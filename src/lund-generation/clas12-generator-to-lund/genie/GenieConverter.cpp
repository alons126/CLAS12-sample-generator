//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file GenieConverter.cpp
 * @brief Existing GENIE GST events converted into LUND records.
 *
 * Purpose:
 *   Serve as the nested GENIE adapter: read supported processes and final-state species,
 *   retain their momenta, and assign a target vertex.
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
#include "core/support/constants.h"
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
 *   3. Assign a common target vertex and retain supported final-state species.
 *   4. Write through capacity or end of input and publish the generation log.
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
    // Use typed, dynamically sized readers rather than fixed final-state buffers.
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
    std::uint64_t scanned = 0;

#pragma region /* Event conversion */
    // Scan until output capacity or input exhaustion; counts distinguish scanned and accepted events.
    while (!writer.full() && reader.Next()) {
        if (qel.GetSetupStatus() < 0 || mec.GetSetupStatus() < 0 || res.GetSetupStatus() < 0 || dis.GetSetupStatus() < 0 || resid.GetSetupStatus() < 0 || nf.GetSetupStatus() < 0 ||
            pxl.GetSetupStatus() < 0 || pyl.GetSetupStatus() < 0 || pzl.GetSetupStatus() < 0 || pdgf.GetSetupStatus() < 0 || pxf.GetSetupStatus() < 0 || pyf.GetSetupStatus() < 0 ||
            pzf.GetSetupStatus() < 0) {
            throw std::runtime_error("GST branch type mismatch");
        }
        ++scanned;
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
        // Copy only supported final-state species while preserving the input momenta.
        for (std::size_t i = 0; i < pdgf.GetSize(); ++i) {
            const int pid = pdgf[i];
            if (pid == constants::proton_pdg || pid == constants::neutron_pdg || pid == constants::pi_plus_pdg || pid == constants::pi_minus_pdg || pid == constants::pi_zero_pdg ||
                pid == constants::photon_pdg) {
                event.particles.push_back({pid, particleMass(pid), {pxf[i], pyf[i], pzf[i]}, vertex});
            }
        }
        writer.write(event);
    }
#pragma endregion

    // Distinguish normal input exhaustion from a schema or later-chain read failure.
    if (!writer.full() && reader.GetEntryStatus() != TTreeReader::kEntryBeyondEnd) { throw std::runtime_error("Failed reading GST entries (check branch types and input files)"); }
    if (!writer.count()) { throw std::runtime_error("No supported QE/MEC/RES/DIS events in input"); }
    writer.finish(scanned);
    LundWriter::printWorkflowSummary(c, "physical", scanned, writer.count(), true);
    std::cout << env::SYSTEM_COLOR << "Scanned " << scanned << ", wrote " << writer.count() << " events to " << env::RESET_COLOR << c.get("output") << '\n';
}
#pragma endregion

}  // namespace samples
