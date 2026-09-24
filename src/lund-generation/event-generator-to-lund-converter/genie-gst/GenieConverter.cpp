//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file GenieConverter.cpp
 * @brief Existing GENIE GST events converted into LUND records.
 *
 * Purpose:
 *   Serve as the nested GENIE adapter: read supported processes and detector-stable final-state
 *   species, retain their momenta, and assign a target vertex.
 *
 * Workflow:
 *   Resolve one or more GST ROOT files into a TChain -> validate the required scalar and variable-
 *   length branches -> scan entries in chain order -> retain QE/MEC/RES/DIS interactions -> construct
 *   one LUND event from the scattered electron and supported final-state particles -> apply the
 *   submission-block tail cutoff -> publish the completed LUND-generation log.
 *
 * Inputs:
 *   RunConfig supplies the GST input path or glob, target geometry, independent nuclear A/Z metadata,
 *   beam energy, vertex RNG seed, event capacity, split size and output metadata. The GST tree supplies
 *   the interaction flags, resonance identifier, scattered-electron momentum and counted final-state
 *   PDG/momentum arrays.
 *
 * Outputs:
 *   The shared LundWriter creates split LUND text files and the completion manifest in the resolved run
 *   directory. This physical adapter creates no monitoring histograms.
 *
 * Assumptions:
 *   GST final-state arrays use nf as their per-entry leaf count. Only QE, MEC, RES and DIS reactions
 *   are supported; adding another reaction requires updating this adapter.
 *
 * Failure:
 *   Empty input, missing or mistyped branches, inconsistent per-entry arrays, unsupported-only input,
 *   ROOT read failures and output failures terminate conversion with an exception.
 */

#include "event-generator-to-lund-converter/genie-gst/GenieConverter.h"

#include <TChain.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <TTreeReaderValue.h>

#include <stdexcept>

#include "core/geometry/TargetGeometry.h"
#include "core/lund/LundWriter.h"

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
 *   3. Assign a common target vertex and retain supported detector-stable final-state species in input order.
 *   4. Before starting a follow-up output file, require at least one configured submission-sized block of
 *      input entries to remain; otherwise continue through accepted-event capacity and publish the log.
 *
 * @param c Resolved input, beam, metadata, target and output settings.
 *
 * @note The configured target selects only the vertex distribution; A and Z are copied independently
 *       into each LUND header. One sampled vertex is shared by every particle in an accepted event.
 * @note No value is returned. Schema, read and output failures throw. The submission cutoff is evaluated
 *       only before a follow-up file starts, so it never interrupts a file that is already being written.
 */
void convertGenie(const RunConfig& c) {
    c.validate(false);
    LundWriter::printWorkflowSummary(c, "physical");

#pragma region /* GST input preparation */
    // TChain accepts either one GST file or the resolved input glob and exposes matching files as one
    // ordered `gst` tree. Require at least one entry, then load the first underlying tree so its branch
    // metadata is available for validation. These checks happen before LundWriter is constructed, so an
    // empty input or missing required branch cannot replace an existing resolved run directory.
    TChain chain("gst");
    if (!chain.Add(c.get("input").c_str()) || chain.GetEntries() == 0) { throw std::runtime_error("No GST entries found for: " + c.get("input")); }
    if (chain.LoadTree(0) < 0) { throw std::runtime_error("Cannot load GST tree"); }

    // Interaction flags determine whether an entry is retained and which historical code is written in
    // the LUND header. resid is copied to the target-polarization header position. nf counts the parallel
    // final-state arrays; pxl/pyl/pzl describe the scattered electron, which is always written first.
    for (const char* branch : {"qel", "mec", "res", "dis", "resid", "nf", "pdgf", "pxf", "pyf", "pzf", "pxl", "pyl", "pzl"}) {
        if (!chain.GetBranch(branch)) { throw std::runtime_error(std::string("Missing GST branch: ") + branch); }
    }

    // TTreeReader owns traversal state across every tree in the chain. Value readers expose one scalar
    // from the current entry. Array readers are dynamically sized ROOT views: on every reader.Next(),
    // ROOT derives each view's exact current-entry length from that branch's nf leaf-count metadata. This
    // adapter allocates no fixed particle buffer; the event loop still cross-checks every view before
    // using an index, so corrupt or mutually inconsistent lengths fail instead of causing an out-of-
    // bounds read.
    TTreeReader reader(&chain);
    TTreeReaderValue<Bool_t> qel(reader, "qel"), mec(reader, "mec"), res(reader, "res"), dis(reader, "dis");
    TTreeReaderValue<Int_t> resid(reader, "resid"), nf(reader, "nf");
    TTreeReaderValue<Double_t> pxl(reader, "pxl"), pyl(reader, "pyl"), pzl(reader, "pzl");
    TTreeReaderArray<Int_t> pdgf(reader, "pdgf");
    TTreeReaderArray<Double_t> pxf(reader, "pxf"), pyf(reader, "pyf"), pzf(reader, "pzf");
#pragma endregion

#pragma region /* Resolved run state */

    // A nonzero vertex seed is repeatable. ROOT interprets TRandom3(0) as automatic seeding; accepting
    // zero leaves that nonrepeatable behavior available when the user values independence over replay.
    // This stream is used only for target vertices; GST particle momenta are copied without resampling.
    TRandom3 random(c.integer("vertex-seed"));

    // Geometry controls spatial sampling only. Nuclear A and Z remain explicit LUND header metadata and
    // are not inferred from the geometry key. LundWriter is constructed only after GST validation; it
    // then safely prepares the resolved run directory and owns splitting, formatting and the manifest.
    TargetGeometry geometry(c.get("target"));
    const double beam = c.number("beam-energy");
    const int A = static_cast<int>(c.integer("A")), Z = static_cast<int>(c.integer("Z"));
    LundWriter writer(c, "physical");

    // total_entries is the fixed number of input entries across the complete chain. submission_block is
    // both the LUND split size and the input-tail cutoff aligned with one submission task's JOB_NEVENTS.
    // scanned counts successful reader.Next() calls, including retained and rejected interactions.
    const auto total_entries = static_cast<std::uint64_t>(chain.GetEntries());
    const auto submission_block = static_cast<std::uint64_t>(c.integer("events-per-file"));
    std::uint64_t scanned = 0;
    bool stopped_at_submission_cutoff = false;

#pragma endregion

#pragma region /* Event conversion */
    // reader.Next() advances through the chained GST entries in order and refreshes every scalar and
    // array view. Stop before reading another entry when LundWriter has reached the configured accepted-
    // event capacity; otherwise continue until ROOT reports end of input or the submission cutoff fires.
    while (!writer.full() && reader.Next()) {
        // Branch existence alone does not prove that its stored ROOT type matches the typed reader. The
        // setup status becomes authoritative after an entry is loaded and is checked for every reader,
        // including after TChain crosses into another input file with a potentially different schema.
        if (qel.GetSetupStatus() < 0 || mec.GetSetupStatus() < 0 || res.GetSetupStatus() < 0 || dis.GetSetupStatus() < 0 || resid.GetSetupStatus() < 0 || nf.GetSetupStatus() < 0 ||
            pxl.GetSetupStatus() < 0 || pyl.GetSetupStatus() < 0 || pzl.GetSetupStatus() < 0 || pdgf.GetSetupStatus() < 0 || pxf.GetSetupStatus() < 0 || pyf.GetSetupStatus() < 0 ||
            pzf.GetSetupStatus() < 0) {
            throw std::runtime_error("GST branch type mismatch");
        }

        // Increment after a successful read. Consequently scanned is a one-based count, while
        // scanned - 1 is the zero-based TChain entry number preserved as the LUND event identifier.
        ++scanned;

        // TTreeReaderArray obtains each current-entry length from ROOT's variable-length leaf metadata.
        // Require the declared nf and every parallel array view to agree before any indexed access. A
        // malformed or truncated entry is rejected rather than trusting one branch as the bound.
        if (*nf < 0 || pdgf.GetSize() != static_cast<std::size_t>(*nf) || pxf.GetSize() != pdgf.GetSize() || pyf.GetSize() != pdgf.GetSize() || pzf.GetSize() != pdgf.GetSize()) {
            throw std::runtime_error("Inconsistent GST final-state array lengths");
        }

        // Map the supported interaction flags to the LUND interaction code: QE=1, MEC=2, RES=3 and
        // DIS=4. This converter requires one of these four reactions. Supporting another reaction requires
        // adding its GST flag branch, LUND code mapping, validation and tests here. GST is expected to make
        // the supported flags mutually exclusive; the expression has the stated priority if malformed
        // input sets more than one. Code zero skips every other reaction before vertex sampling or output,
        // so skipped entries consume neither RNG draws nor writer capacity.
        double code = *qel ? 1 : *mec ? 2 : *res ? 3 : *dis ? 4 : 0;
        if (!code) { continue; }

        // Use the submission block only to decide whether a follow-up LUND file may start. The first file
        // is always allowed, including for inputs shorter than one block. At each later file boundary,
        // require at least one inclusive block of GST entries beginning with the current accepted entry.
        // Once a file starts, do not reapply this test inside it: doing so would interrupt an exact final
        // block after its second event because the inclusive remaining count has fallen below the block.
        const auto current_entry = scanned - 1;
        const auto inclusive_entries_remaining = total_entries - current_entry;
        const bool starting_followup_file = writer.count() > 0 && writer.count() % submission_block == 0;
        if (starting_followup_file && inclusive_entries_remaining < submission_block) {
            stopped_at_submission_cutoff = true;
            break;
        }

        // Build the LUND header state directly from the accepted GST entry and resolved configuration.
        // resonance_id is serialized in the established target-polarization position, while weight
        // carries the interaction code. A/Z and beam energy remain constant across the run.
        Event event;
        event.id = scanned - 1;
        event.A = A;
        event.Z = Z;
        event.beam_energy = beam;
        event.resonance_id = *resid;
        event.weight = code;

        // Sample exactly one accepted-event vertex and assign it first to the scattered electron. Every
        // retained final-state particle below receives the same position, preserving one interaction
        // point per event. The electron remains the first serialized particle by construction.
        auto vertex = geometry.sample(random);
        event.particles.push_back({constants::electron_pdg, particleMass(constants::electron_pdg), {*pxl, *pyl, *pzl}, vertex});

        // Copy only detector-stable supported species while preserving input order and momenta.
        // Unsupported identities are omitted; none are substituted or given invented kinematics.
        // particleMass obtains the maintained targets.h value for every retained identity.
        for (std::size_t i = 0; i < pdgf.GetSize(); ++i) {
            const int pid = pdgf[i];
            if (pid == constants::proton_pdg || pid == constants::neutron_pdg || pid == constants::pi_plus_pdg || pid == constants::pi_minus_pdg || pid == constants::photon_pdg) {
                event.particles.push_back({pid, particleMass(pid), {pxf[i], pyf[i], pzf[i]}, vertex});
            }
        }

        // LundWriter derives mass-shell energies, serializes the complete event, rotates files at the
        // configured split size and increments its accepted-event count only after successful output.
        writer.write(event);
    }
#pragma endregion

#pragma region /* Completion and publication */

    // A false reader.Next() is successful only when ROOT reached the normal end of the chain. Capacity
    // and submission-cutoff exits do not call reader.Next() again, so their current reader status is not
    // an error. Any other status indicates a read or schema failure, including one encountered only in a
    // later chained file.
    if (!writer.full() && !stopped_at_submission_cutoff && reader.GetEntryStatus() != TTreeReader::kEntryBeyondEnd) {
        throw std::runtime_error("Failed reading GST entries (check branch types and input files)");
    }

    // An input containing no QE/MEC/RES/DIS interaction is not a successful empty production run. Fail
    // before publishing the completion manifest so downstream submission cannot treat it as consumable.
    if (!writer.count()) { throw std::runtime_error("No supported QE/MEC/RES/DIS events in input"); }

    // finish() closes and verifies the last LUND stream, records scanned/written counts and split-file
    // metadata, then atomically publishes lund-gen-log.json. The summary reports those final counters for
    // operators without modifying generated content.
    writer.finish(scanned);
    LundWriter::printWorkflowSummary(c, "physical", scanned, writer.count(), true);

#pragma endregion
}
#pragma endregion

}  // namespace samples
