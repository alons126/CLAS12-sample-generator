#include "genie/GenieConverter.h"

#include <TChain.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <TTreeReaderValue.h>

#include <iostream>
#include <stdexcept>

#include "common/LundWriter.h"
#include "common/Monitoring.h"
#include "common/TargetGeometry.h"
namespace samples {
void convertGenie(const RunConfig& c) {
    c.validate(true);
    TChain chain("gst");
    if (!chain.Add(c.get("input").c_str()) || chain.GetEntries() == 0) throw std::runtime_error("No GST entries found for: " + c.get("input"));
    if (chain.LoadTree(0) < 0) throw std::runtime_error("Cannot load GST tree");
    for (const char* branch : {"qel", "mec", "res", "dis", "resid", "nf", "pdgf", "pxf", "pyf", "pzf", "pxl", "pyl", "pzl"})
        if (!chain.GetBranch(branch)) throw std::runtime_error(std::string("Missing GST branch: ") + branch);
    TTreeReader reader(&chain);
    TTreeReaderValue<Bool_t> qel(reader, "qel"), mec(reader, "mec"), res(reader, "res"), dis(reader, "dis");
    TTreeReaderValue<Int_t> resid(reader, "resid"), nf(reader, "nf");
    TTreeReaderValue<Double_t> pxl(reader, "pxl"), pyl(reader, "pyl"), pzl(reader, "pzl");
    TTreeReaderArray<Int_t> pdgf(reader, "pdgf");
    TTreeReaderArray<Double_t> pxf(reader, "pxf"), pyf(reader, "pyf"), pzf(reader, "pzf");
    TRandom3 random(c.integer("vertex-seed"));
    TargetGeometry geometry(c.get("target"));
    const double beam = c.number("beam-energy");
    const int A = static_cast<int>(c.integer("A")), Z = static_cast<int>(c.integer("Z"));
    Monitoring monitoring(beam);
    LundWriter writer(c, "genie");
    std::uint64_t scanned = 0;
    while (!writer.full() && reader.Next()) {
        if (qel.GetSetupStatus() < 0 || mec.GetSetupStatus() < 0 || res.GetSetupStatus() < 0 || dis.GetSetupStatus() < 0 || resid.GetSetupStatus() < 0 || nf.GetSetupStatus() < 0 ||
            pxl.GetSetupStatus() < 0 || pyl.GetSetupStatus() < 0 || pzl.GetSetupStatus() < 0 || pdgf.GetSetupStatus() < 0 || pxf.GetSetupStatus() < 0 || pyf.GetSetupStatus() < 0 ||
            pzf.GetSetupStatus() < 0)
            throw std::runtime_error("GST branch type mismatch");
        ++scanned;
        if (*nf < 0 || pdgf.GetSize() != static_cast<std::size_t>(*nf) || pxf.GetSize() != pdgf.GetSize() || pyf.GetSize() != pdgf.GetSize() || pzf.GetSize() != pdgf.GetSize())
            throw std::runtime_error("Inconsistent GST final-state array lengths");
        double code = *qel ? 1 : *mec ? 2 : *res ? 3 : *dis ? 4 : 0;
        if (!code) continue;
        Event event;
        event.id = scanned - 1;
        event.A = A;
        event.Z = Z;
        event.beam_energy = beam;
        event.resonance_id = *resid;
        event.weight = code;
        auto vertex = geometry.sample(random);
        event.particles.push_back({11, particleMass(11), {*pxl, *pyl, *pzl}, vertex});
        for (std::size_t i = 0; i < pdgf.GetSize(); ++i) {
            const int pid = pdgf[i];
            if (pid == 2212 || pid == 2112 || pid == 211 || pid == -211 || pid == 111 || pid == 22) event.particles.push_back({pid, particleMass(pid), {pxf[i], pyf[i], pzf[i]}, vertex});
        }
        writer.write(event);
        monitoring.fill(event);
    }
    if (!writer.full() && reader.GetEntryStatus() != TTreeReader::kEntryBeyondEnd) throw std::runtime_error("Failed reading GST entries (check branch types and input files)");
    if (!writer.count()) throw std::runtime_error("No supported QE/MEC/RES/DIS events in input");
    monitoring.save(std::filesystem::path(c.get("output")) / "monitoring.root");
    writer.finish(scanned);
    std::cout << "Scanned " << scanned << ", wrote " << writer.count() << " events to " << c.get("output") << '\n';
}
}  // namespace samples
