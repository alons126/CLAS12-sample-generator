//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file GenieConverterGST.cpp
 * @brief Copies supported GENIE GST events to LUND records.
 *
 * Purpose:
 *   Read supported GENIE processes and final-state particles, keep their input momenta, and give every
 *   particle in an event the same sampled target vertex.
 *
 * Workflow:
 *   Open one or more GST ROOT files as a TChain -> check the required branches -> scan entries in order
 *   -> keep QE/MEC/RES/DIS events -> build each event from the electron and supported final particles
 *   -> apply the input-tail cutoff before a later output file -> publish the completed run log.
 *
 * Inputs:
 *   RunConfig supplies the GST input path or glob, target geometry, separate A and Z values, beam
 *   energy, vertex RNG seed, event limit, split size, and output metadata. The GST tree supplies
 *   the interaction flags, resonance identifier, scattered-electron momentum and counted final-state
 *   PDG/momentum arrays.
 *
 * Outputs:
 *   LundWriter creates split LUND text files and the completion manifest in the final run directory.
 *   Physical conversion creates no monitoring histograms.
 *
 * Expected input:
 *   GST final-state arrays use nf as their per-entry leaf count. Only QE, MEC, RES and DIS reactions
 *   are supported; adding another reaction requires updating this converter.
 *
 * Failure:
 *   Empty input, missing or wrong branch types, different array lengths, unsupported-only input,
 *   ROOT read failures and output failures terminate conversion with an exception.
 */

#include "event-generator-to-lund-converter/genie-gst/GenieConverterGST.h"

#include <TChain.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <TTreeReaderValue.h>

#include <stdexcept>

#include "core/geometry/TargetGeometry.h"
#include "core/lund/LundWriter.h"

namespace samples {

// convertGenieGST -------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* convertGenieGST */
void convertGenieGST(const RunConfig& c) {
    c.validate(false);
    LundWriter::printWorkflowSummary(c, "physical");

#pragma region /* GST input preparation */
    // TChain reads matching files as one ordered `gst` tree. Check input before output can be replaced.
    TChain chain("gst");
    if (!chain.Add(c.get("input").c_str()) || chain.GetEntries() == 0) { throw std::runtime_error("No GST entries found for: " + c.get("input")); }
    if (chain.LoadTree(0) < 0) { throw std::runtime_error("Cannot load GST tree"); }

    // Interaction flags decide whether an entry is kept and which code is written. resid goes in the
    // old target-polarization field. nf counts final particles; pxl/pyl/pzl describe the first electron.
    for (const char* branch : {"qel", "mec", "res", "dis", "resid", "nf", "pdgf", "pxf", "pyf", "pzf", "pxl", "pyl", "pzl"}) {
        if (!chain.GetBranch(branch)) { throw std::runtime_error(std::string("Missing GST branch: ") + branch); }
    }

    // TTreeReader moves through the chain. Array readers use the current `nf`, so no fixed buffer is needed.
    TTreeReader reader(&chain);
    TTreeReaderValue<Bool_t> qel(reader, "qel"), mec(reader, "mec"), res(reader, "res"), dis(reader, "dis");
    TTreeReaderValue<Int_t> resid(reader, "resid"), nf(reader, "nf");
    TTreeReaderValue<Double_t> pxl(reader, "pxl"), pyl(reader, "pyl"), pzl(reader, "pzl");
    TTreeReaderArray<Int_t> pdgf(reader, "pdgf");
    TTreeReaderArray<Double_t> pxf(reader, "pxf"), pyf(reader, "pyf"), pzf(reader, "pzf");
#pragma endregion

#pragma region /* Resolved run state */

    // A nonzero seed is repeatable. ROOT gives TRandom3(0) a new automatic seed, so zero is not replayable.
    // This RNG samples only vertices; input momenta are copied unchanged.
    TRandom3 random(c.integer("vertex-seed"));

    // Geometry controls only the vertex position. A and Z are separate LUND header values. Construct the
    // writer only after the GST checks pass.
    TargetGeometry geometry(c.get("target"));
    const double beam = c.number("beam-energy");
    const int A = static_cast<int>(c.integer("A")), Z = static_cast<int>(c.integer("Z"));
    LundWriter writer(c, "physical");

    // submission_block is the split size and the minimum input left before starting another file.
    const auto total_entries = static_cast<std::uint64_t>(chain.GetEntries());
    const auto submission_block = static_cast<std::uint64_t>(c.integer("events-per-file"));
    std::uint64_t scanned = 0;
    bool stopped_at_submission_cutoff = false;

#pragma endregion

#pragma region /* Event conversion */
    // Read GST entries in order until the writer is full, ROOT reaches the end, or the tail cutoff stops
    // a later file from starting.
    while (!writer.full() && reader.Next()) {
        // Check reader types for every entry, including entries in later files.
        if (qel.GetSetupStatus() < 0 || mec.GetSetupStatus() < 0 || res.GetSetupStatus() < 0 || dis.GetSetupStatus() < 0 || resid.GetSetupStatus() < 0 || nf.GetSetupStatus() < 0 ||
            pxl.GetSetupStatus() < 0 || pyl.GetSetupStatus() < 0 || pzl.GetSetupStatus() < 0 || pdgf.GetSetupStatus() < 0 || pxf.GetSetupStatus() < 0 || pyf.GetSetupStatus() < 0 ||
            pzf.GetSetupStatus() < 0) {
            throw std::runtime_error("GST branch type mismatch");
        }

        // Count the successful read. scanned - 1 is its zero-based input entry number.
        ++scanned;

        // Require `nf` and every parallel particle array to have the same length before using an index.
        if (*nf < 0 || pdgf.GetSize() != static_cast<std::size_t>(*nf) || pxf.GetSize() != pdgf.GetSize() || pyf.GetSize() != pdgf.GetSize() || pzf.GetSize() != pdgf.GetSize()) {
            throw std::runtime_error("Inconsistent GST final-state array lengths");
        }

        // Map QE, MEC, RES, and DIS to codes 1 through 4. Skip other reactions before sampling a vertex.
        // If bad input sets several flags, the written order here gives the first one priority.
        double code = *qel ? 1 : *mec ? 2 : *res ? 3 : *dis ? 4 : 0;
        if (!code) { continue; }

        // Always allow the first file. Before a later file starts, require one full inclusive block of
        // input entries. Do not repeat this check after that file has started.
        const auto current_entry = scanned - 1;
        const auto inclusive_entries_remaining = total_entries - current_entry;
        const bool starting_followup_file = writer.count() > 0 && writer.count() % submission_block == 0;
        if (starting_followup_file && inclusive_entries_remaining < submission_block) {
            stopped_at_submission_cutoff = true;
            break;
        }

        // Fill the LUND header from the GST entry and final settings. resonance_id uses the old
        // target-polarization field, and weight stores the interaction code.
        Event event;
        event.id = scanned - 1;
        event.A = A;
        event.Z = Z;
        event.beam_energy = beam;
        event.resonance_id = *resid;
        event.weight = code;

        // Sample one vertex, write the electron first, and give every kept particle the same position.
        auto vertex = geometry.sample(random);
        event.particles.push_back({constants::electron_pdg, particleMass(constants::electron_pdg), {*pxl, *pyl, *pzl}, vertex});

        // Copy supported particles in input order. Skip others without changing or inventing momentum.
        for (std::size_t i = 0; i < pdgf.GetSize(); ++i) {
            const int pid = pdgf[i];
            if (pid == constants::proton_pdg || pid == constants::neutron_pdg || pid == constants::pi_plus_pdg || pid == constants::pi_minus_pdg || pid == constants::photon_pdg) {
                event.particles.push_back({pid, particleMass(pid), {pxf[i], pyf[i], pzf[i]}, vertex});
            }
        }

        // LundWriter calculates particle energies, writes the event, starts a new file at the configured
        // split size, and increases its event count only after success.
        writer.write(event);
    }
#pragma endregion

#pragma region /* Completion and publication */

    // When neither limit stopped the loop, require ROOT to have reached the normal end of input.
    if (!writer.full() && !stopped_at_submission_cutoff && reader.GetEntryStatus() != TTreeReader::kEntryBeyondEnd) {
        throw std::runtime_error("Failed reading GST entries (check branch types and input files)");
    }

    // Do not publish a completed run when no supported event was written.
    if (!writer.count()) { throw std::runtime_error("No supported QE/MEC/RES/DIS events in input"); }

    // Close output, publish the run log, and print final counts.
    writer.finish(scanned);
    LundWriter::printWorkflowSummary(c, "physical", scanned, writer.count(), true);

#pragma endregion
}
#pragma endregion

}  // namespace samples
