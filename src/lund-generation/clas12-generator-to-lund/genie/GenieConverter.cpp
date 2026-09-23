//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file GenieConverter.cpp
 * @brief Existing GENIE GST events converted into LUND records.
 *
 * Purpose:
 *   Serve as the nested GENIE adapter: read supported processes and detector-stable final-state
 *   species, retain their momenta, and assign a target vertex. Neutral pions must be decayed by the
 *   upstream GENIE production into photons and are not copied into detector-simulation input.
 *
 * Workflow:
 *   Validate GST schema -> scan and select -> write events -> publish the LUND-generation log.
 */

#include "clas12-generator-to-lund/genie/GenieConverter.h"

#include <TChain.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <TTreeReaderValue.h>

#include <iostream>
#include <stdexcept>

#include "core/geometry/TargetGeometry.h"
#include "core/lund/LundWriter.h"
#include "core/support/environment.h"

namespace env = environment;

namespace samples {

// convertGenie ----------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* convertGenie */
/**
 * @brief Convert supported events from an existing GENIE GST chain.
 *
 * Purpose:
 *   Prepare existing physical events for detector simulation while preserving their supported momenta.
 *
 * Algorithm:
 *   1. Validate required branches and construct typed readers.
 *   2. Scan entries and select QE/MEC/RES/DIS events.
 *   3. Assign a common target vertex and retain detector-stable final-state species, skipping any
 *      residual neutral pion because its two-photon decay must already be present in the GST truth.
 *   4. After each written event, stop when fewer than one configured submission-sized input block
 *      remains; otherwise continue through accepted-event capacity and publish the generation log.
 *
 * @param c Resolved input, beam, metadata, target, mass and output settings.
 *
 * @note No value; schema/read/output failures throw. Accepted partial final files are retained.
 */
void convertGenie(const RunConfig& c) {
    c.validate(false);
    LundWriter::printWorkflowSummary(c, "physical");

#pragma region /* GST input preparation */
    // Validate the input chain before constructing any output products.
    TChain chain("gst");
    if (!chain.Add(c.get("input").c_str()) || chain.GetEntries() == 0) { throw std::runtime_error("No GST entries found for: " + c.get("input")); }
    if (chain.LoadTree(0) < 0) { throw std::runtime_error("Cannot load GST tree"); }
    for (const char* branch : {"qel", "mec", "res", "dis", "resid", "nf", "pdgf", "pxf", "pyf", "pzf", "pxl", "pyl", "pzl"}) {
        if (!chain.GetBranch(branch)) { throw std::runtime_error(std::string("Missing GST branch: ") + branch); }
    }
    // Use typed ROOT array views whose current-entry sizes come from leaf-count metadata; the event
    // loop cross-checks every reported size against nf before indexed access.
    TTreeReader reader(&chain);
    TTreeReaderValue<Bool_t> qel(reader, "qel"), mec(reader, "mec"), res(reader, "res"), dis(reader, "dis");
    TTreeReaderValue<Int_t> resid(reader, "resid"), nf(reader, "nf");
    TTreeReaderValue<Double_t> pxl(reader, "pxl"), pyl(reader, "pyl"), pzl(reader, "pzl");
    TTreeReaderArray<Int_t> pdgf(reader, "pdgf");
    TTreeReaderArray<Double_t> pxf(reader, "pxf"), pyf(reader, "pyf"), pzf(reader, "pzf");
#pragma endregion

    // A nonzero vertex seed is repeatable. ROOT interprets TRandom3(0) as automatic seeding; accepting
    // zero leaves that nonrepeatable behavior available when the user values independence over replay.
    TRandom3 random(c.integer("vertex-seed"));
    TargetGeometry geometry(c.get("target"));
    const double beam = c.number("beam-energy");
    const int A = static_cast<int>(c.integer("A")), Z = static_cast<int>(c.integer("Z"));
    LundWriter writer(c, "physical");
    const auto total_entries = static_cast<std::uint64_t>(chain.GetEntries());
    const auto submission_block = static_cast<std::uint64_t>(c.integer("events-per-file"));
    std::uint64_t scanned = 0;
    bool stopped_at_submission_cutoff = false;

#pragma region /* Event conversion */
    // Scan until output capacity or input exhaustion; counts distinguish scanned and accepted events.
    while (!writer.full() && reader.Next()) {
        if (qel.GetSetupStatus() < 0 || mec.GetSetupStatus() < 0 || res.GetSetupStatus() < 0 || dis.GetSetupStatus() < 0 || resid.GetSetupStatus() < 0 || nf.GetSetupStatus() < 0 ||
            pxl.GetSetupStatus() < 0 || pyl.GetSetupStatus() < 0 || pzl.GetSetupStatus() < 0 || pdgf.GetSetupStatus() < 0 || pxf.GetSetupStatus() < 0 || pyf.GetSetupStatus() < 0 ||
            pzf.GetSetupStatus() < 0) {
            throw std::runtime_error("GST branch type mismatch");
        }
        ++scanned;
        // TTreeReaderArray obtains each current-entry length from ROOT's variable-length leaf metadata.
        // Require the declared nf and every parallel array view to agree before any indexed access. A
        // malformed or truncated entry is rejected rather than trusting one branch as the bound.
        if (*nf < 0 || pdgf.GetSize() != static_cast<std::size_t>(*nf) || pxf.GetSize() != pdgf.GetSize() || pyf.GetSize() != pdgf.GetSize() || pzf.GetSize() != pdgf.GetSize()) {
            throw std::runtime_error("Inconsistent GST final-state array lengths");
        }
        // Retain the historical process-tag convention in the LUND header.
        double code = *qel ? 1 : *mec ? 2 : *res ? 3 : *dis ? 4 : 0;
        if (!code) { continue; }
        Event event;
        event.id = scanned - 1;
        event.A = A;
        event.Z = Z;
        event.beam_energy = beam;
        event.resonance_id = *resid;
        event.weight = code;
        auto vertex = geometry.sample(random);
        event.particles.push_back({constants::electron_pdg, particleMass(constants::electron_pdg), {*pxl, *pyl, *pzl}, vertex});
        // Copy only detector-stable supported species while preserving input order and momenta. A
        // residual PDG 111 is deliberately skipped: this converter cannot reconstruct the missing
        // two-photon decay kinematics, so GENIE production must decay pi0 before writing the GST tree.
        for (std::size_t i = 0; i < pdgf.GetSize(); ++i) {
            const int pid = pdgf[i];
            if (pid == constants::proton_pdg || pid == constants::neutron_pdg || pid == constants::pi_plus_pdg || pid == constants::pi_minus_pdg || pid == constants::photon_pdg) {
                event.particles.push_back({pid, particleMass(pid), {pxf[i], pyf[i], pzf[i]}, vertex});
            }
        }
        writer.write(event);

        // Submission gives every array task one JOB_NEVENTS limit. After writing the current accepted
        // event, stop once the remaining GST input (including that current entry in this comparison)
        // is smaller than one configured block. The cutoff is deliberately based on input entries, not
        // accepted events, and therefore can leave the current output file shorter than JOB_NEVENTS.
        const auto current_entry = scanned - 1;
        if (total_entries - current_entry < submission_block) {
            stopped_at_submission_cutoff = true;
            break;
        }
    }
#pragma endregion

    // Distinguish normal input exhaustion from a schema or later-chain read failure.
    if (!writer.full() && !stopped_at_submission_cutoff && reader.GetEntryStatus() != TTreeReader::kEntryBeyondEnd) {
        throw std::runtime_error("Failed reading GST entries (check branch types and input files)");
    }
    if (!writer.count()) { throw std::runtime_error("No supported QE/MEC/RES/DIS events in input"); }
    writer.finish(scanned);
    LundWriter::printWorkflowSummary(c, "physical", scanned, writer.count(), true);
    std::cout << env::SYSTEM_COLOR << "Scanned " << scanned << ", wrote " << writer.count() << " events to " << env::RESET_COLOR << c.get("output") << '\n';
}
#pragma endregion

}  // namespace samples
