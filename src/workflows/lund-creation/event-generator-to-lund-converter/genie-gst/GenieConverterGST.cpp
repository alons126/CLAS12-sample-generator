/**
 * @file GenieConverterGST.cpp
 * @brief Reads GENIE GST data and writes the supported events as LUND records.
 *
 * Purpose:
 *   A GENIE GST ROOT tree is a table in which each row describes one event before detector simulation.
 *   This file copies the supported events and particles into LUND. It never recalculates their momenta.
 *   It only chooses one vertex position for each written event.
 *
 * Execution flow:
 *   Open the matching GST files as one ordered input -> check every required field -> scan entries in
 *   order -> keep QE, MEC, RES, and DIS events -> copy their supported particles -> stop before a later
 *   output file when too few input entries remain -> close the files and write the run log.
 *
 * Inputs:
 *   RunConfig supplies the GST file path or filename pattern, target shape, separate A and Z values, beam
 *   energy, seed for choosing vertex positions, event limits, file size, and output labels. The GST tree
 *   supplies the interaction type, resonance number, electron momentum, particle IDs, and particle momenta.
 *
 * Outputs:
 *   LundWriter creates split LUND text files and `lund-creation-log.json` in the run directory. Progress
 *   shows how many GST entries were scanned and how many LUND events were written. The physical LUND
 *   converter does not create ROOT monitoring plots.
 *
 * Input rules:
 *   In each GST entry, `nf` is the number of final-state particles. The particle-ID array and the three
 *   momentum arrays must each contain exactly `nf` values. The supported interaction flags are QE
 *   (quasi-elastic), MEC (meson-exchange current), RES (resonance production), and DIS (deep-inelastic
 *   scattering).
 *
 * Failure:
 *   Empty input, missing fields, wrong field types, different array lengths, an input with no supported
 *   events, ROOT read failures, and output failures stop the run with an exception.
 */

#include "event-generator-to-lund-converter/genie-gst/GenieConverterGST.h"

#include <TChain.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <TTreeReaderValue.h>

#include <iostream>
#include <stdexcept>

#include "core/geometry/TargetGeometry.h"
#include "core/lund/LundWriter.h"
#include "core/presentation/ProgressReporter.h"
#include "support/environment.h"

namespace env = environment;

namespace samples {

// convertGenieGST -------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* convertGenieGST */
void convertGenieGST(const RunConfig& c) {
    c.validateForSource(LundSource::Physical);
    LundWriter::printWorkflowSummary(c, "physical");

#pragma region /* GST input preparation */
    std::cout << "\n" << env::SYSTEM_COLOR << "Loading and validating GENIE GST input..." << env::RESET_COLOR << "\n";

    // TChain makes all matching files look like one ordered tree named `gst`. Check it before the output directory can be replaced.
    TChain chain("gst");
    if (!chain.Add(c.getText("input").c_str()) || (chain.GetEntries() == 0)) { throw std::runtime_error("No GST entries found for: " + c.getText("input")); }
    if (chain.LoadTree(0) < 0) { throw std::runtime_error("Cannot load GST tree"); }

    // These are the GST fields needed below. The four interaction flags choose which entries are written.
    // `resid` is the resonance number, `nf` counts final particles, and pxl/pyl/pzl hold the electron momentum.
    for (const char* branch : {"qel", "mec", "res", "dis", "resid", "nf", "pdgf", "pxf", "pyf", "pzf", "pxl", "pyl", "pzl"}) {
        if (!chain.GetBranch(branch)) { throw std::runtime_error(std::string("Missing GST branch: ") + branch); }
    }

    // TTreeReader advances one event at a time. Its array readers automatically use the current event's array lengths.
    TTreeReader reader(&chain);
    TTreeReaderValue<Bool_t> qel(reader, "qel"), mec(reader, "mec"), res(reader, "res"), dis(reader, "dis");
    TTreeReaderValue<Int_t> resid(reader, "resid"), nf(reader, "nf");
    TTreeReaderValue<Double_t> pxl(reader, "pxl"), pyl(reader, "pyl"), pzl(reader, "pzl");
    TTreeReaderArray<Int_t> pdgf(reader, "pdgf");
    TTreeReaderArray<Double_t> pxf(reader, "pxf"), pyf(reader, "pyf"), pzf(reader, "pzf");
#pragma endregion

#pragma region /* Resolved run state */
    std::cout << "\n" << env::SYSTEM_COLOR << "Preparing target geometry and LUND output..." << env::RESET_COLOR << "\n";

    // The random number generator chooses vertex positions. A nonzero seed repeats the same positions on
    // another run. ROOT gives seed 0 a new automatic value, so a run configured with 0 cannot be repeated from that value alone.
    TRandom3 random(c.getNonnegativeInteger("vertex-seed"));

    // Target geometry controls where the event occurs. A and Z are separate numbers written in the LUND event header.
    // Create the writer only after the GST input checks pass, because the writer may replace the run directory.
    TargetGeometry geometry(c.getText("target-geometry"));
    const double beam = c.getDouble("beam-energy");
    const int A = static_cast<int>(c.getNonnegativeInteger("A")), Z = static_cast<int>(c.getNonnegativeInteger("Z"));
    LundWriter writer(c, "physical");

    // For example, a value of 10,000 writes at most 10,000 accepted events per file. Before opening a later file,
    // the code also requires at least 10,000 GST input entries to remain.
    const auto total_entries = static_cast<std::uint64_t>(chain.GetEntries());
    const auto submission_block = static_cast<std::uint64_t>(c.getNonnegativeInteger("events-per-file"));
    std::uint64_t scanned = 0;
    bool stopped_at_submission_cutoff = false;

#pragma endregion

#pragma region /* Event conversion */
    std::cout << "\n" << env::SYSTEM_COLOR << "Converting GENIE GST events to LUND..." << env::RESET_COLOR << "\n";

    const auto requested_events = static_cast<std::uint64_t>(c.getNonnegativeInteger("events"));
    ProgressReporter progress("Converting GST", requested_events, "Written", "Scanned");
    progress.update(0, 0, total_entries);

    // Read entries in their original order until enough events are written, ROOT reaches the end, or the
    // input-tail rule prevents a second or later output file from starting.
    while (!writer.hasReachedRunEventLimit() && reader.Next()) {
        // ROOT reports a negative setup status when a GST field has the wrong stored type. Check every input file, not only the first one.
        if ((qel.GetSetupStatus() < 0) || (mec.GetSetupStatus() < 0) || (res.GetSetupStatus() < 0) || (dis.GetSetupStatus() < 0) || (resid.GetSetupStatus() < 0) ||
            (nf.GetSetupStatus() < 0) || (pxl.GetSetupStatus() < 0) || (pyl.GetSetupStatus() < 0) || (pzl.GetSetupStatus() < 0) || (pdgf.GetSetupStatus() < 0) ||
            (pxf.GetSetupStatus() < 0) || (pyf.GetSetupStatus() < 0) || (pzf.GetSetupStatus() < 0)) {
            throw std::runtime_error("GST branch type mismatch");
        }

        // Count every successfully read GST entry, including entries that will be skipped. `scanned - 1` is its zero-based input number.
        ++scanned;

        // Each particle uses values at the same position in four separate arrays. Check their lengths first so an index cannot read past an array.
        if ((*nf < 0) || (pdgf.GetSize() != static_cast<std::size_t>(*nf)) || (pxf.GetSize() != pdgf.GetSize()) || (pyf.GetSize() != pdgf.GetSize()) || (pzf.GetSize() != pdgf.GetSize())) {
            throw std::runtime_error("Inconsistent GST final-state array lengths");
        }

        progress.update(writer.getWrittenEventCount(), scanned, total_entries);

        // Store QE, MEC, RES, and DIS as codes 1, 2, 3, and 4. Skip other interactions before choosing a vertex position.
        // If invalid input sets more than one flag, the first true flag in this expression wins.
        double code = *qel ? 1 : *mec ? 2 : *res ? 3 : *dis ? 4 : 0;
        if (!code) { continue; }

        // Always allow the first output file. Before opening a later file, count the current GST entry plus
        // every entry after it. Stop if that count is smaller than `events-per-file`. This counts input
        // entries, not accepted events, and the check is not repeated after the new file starts.
        const auto current_entry = scanned - 1;
        const auto inclusive_entries_remaining = total_entries - current_entry;
        const bool starting_followup_file = ((writer.getWrittenEventCount() > 0) && (writer.getWrittenEventCount() % submission_block == 0));
        if (starting_followup_file && (inclusive_entries_remaining < submission_block)) {
            stopped_at_submission_cutoff = true;
            break;
        }

        // Fill the LUND event header. The established file layout stores the GST resonance number in the
        // target-polarization field and stores the interaction code in the weight field.
        Event event;
        event.id = scanned - 1;
        event.A = A;
        event.Z = Z;
        event.beam_energy = beam;
        event.resonance_id = *resid;
        event.weight = code;

        // Choose one vertex position. Put the scattered electron first and give the same Vx, Vy, and Vz coordinates to every particle in the event.
        auto vertex = geometry.sampleVertexPosition(random);
        event.particles.push_back({constants::electron_pdg, getParticleMass(constants::electron_pdg), {*pxl, *pyl, *pzl}, vertex});

        // Copy supported particles in their GST order. This includes photons from an upstream neutral-pion
        // decay. A remaining neutral pion (PDG 111), like any unsupported particle, is skipped; no missing decay is invented here.
        for (std::size_t i = 0; i < pdgf.GetSize(); ++i) {
            const int pid = pdgf[i];
            if ((pid == constants::proton_pdg) || (pid == constants::neutron_pdg) || (pid == constants::pi_plus_pdg) || (pid == constants::pi_minus_pdg) || (pid == constants::photon_pdg)) {
                event.particles.push_back({pid, getParticleMass(pid), {pxf[i], pyf[i], pzf[i]}, vertex});
            }
        }

        // LundWriter calculates each energy from the copied momentum and particle mass. It writes the event,
        // opens a new file at the configured size, and increases its count only after the write succeeds.
        writer.writeEvent(event);
        progress.update(writer.getWrittenEventCount(), scanned, total_entries);
    }
#pragma endregion

#pragma region /* Completion and publication */
    // If neither planned stop condition ended the loop, ROOT must report that it reached the normal end of the input.
    if (!writer.hasReachedRunEventLimit() && !stopped_at_submission_cutoff && (reader.GetEntryStatus() != TTreeReader::kEntryBeyondEnd)) {
        throw std::runtime_error("Failed reading GST entries (check branch types and input files)");
    }

    // A run containing no LUND events is not useful, so do not publish it as complete.
    if (!writer.getWrittenEventCount()) { throw std::runtime_error("No supported QE/MEC/RES/DIS events in input"); }

    const std::string stop_reason = writer.hasReachedRunEventLimit() ? "requested event capacity reached"
                                    : stopped_at_submission_cutoff   ? "submission-tail cutoff reached"
                                                                     : "input exhausted";
    progress.finish(writer.getWrittenEventCount(), scanned, total_entries, stop_reason);

    std::cout << "\n" << env::SYSTEM_COLOR << "Finalizing LUND output..." << env::RESET_COLOR << "\n";

    // Close the last LUND file, write the JSON run log, and print the final scanned and written counts.
    writer.finalizeRun(scanned);
    LundWriter::printWorkflowSummary(c, "physical", scanned, writer.getWrittenEventCount(), true);

#pragma endregion
}
#pragma endregion

}  // namespace samples
