#include "genie/GenieConverter.h"

#include <TCanvas.h>
#include <TChain.h>
#include <TFile.h>
#include <TH2D.h>
#include <TMath.h>
#include <TROOT.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <TTreeReaderValue.h>

#include <algorithm>
#include <cmath>
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
    const bool legacy_mass = c.get("mass-convention") == "legacy";
    Monitoring monitoring(beam);
    TH2D legacy_electron("theta_e_VS_phi_e", "#theta_{e} vs. #phi_{e};#phi_{e} [#circ];#theta_{e}", 100, -180., 180., 100, 0., 50.);
    legacy_electron.SetDirectory(nullptr);
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
        // The archived diagnostic includes all scanned events, before process selection.
        const double p = std::sqrt(*pxl * *pxl + *pyl * *pyl + *pzl * *pzl);
        const double theta = p > 0 ? std::acos(std::clamp(*pzl / p, -1.0, 1.0)) * TMath::RadToDeg() : 0;
        legacy_electron.Fill(std::atan2(*pyl, *pxl) * TMath::RadToDeg(), theta);
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
        event.particles.push_back({11, particleMass(11, legacy_mass), {*pxl, *pyl, *pzl}, vertex});
        for (std::size_t i = 0; i < pdgf.GetSize(); ++i) {
            const int pid = pdgf[i];
            if (pid == 2212 || pid == 2112 || pid == 211 || pid == -211 || pid == 111 || pid == 22)
                event.particles.push_back({pid, particleMass(pid, legacy_mass), {pxf[i], pyf[i], pzf[i]}, vertex});
        }
        writer.write(event);
        monitoring.fill(event);
    }
    if (!writer.full() && reader.GetEntryStatus() != TTreeReader::kEntryBeyondEnd) throw std::runtime_error("Failed reading GST entries (check branch types and input files)");
    if (!writer.count()) throw std::runtime_error("No supported QE/MEC/RES/DIS events in input");
    monitoring.save(std::filesystem::path(c.get("output")) / "monitoring.root");
    const auto output = std::filesystem::path(c.get("output"));
    TFile legacy_file((output / "legacy_histograms.root").string().c_str(), "CREATE");
    if (legacy_file.IsZombie() || legacy_electron.Write() <= 0) throw std::runtime_error("Cannot write legacy GENIE diagnostic");
    legacy_file.Close();
    if (c.get("render-plots") == "true") {
        gROOT->SetBatch(true);
        std::filesystem::create_directory(output / "monitoring_plots");
        TCanvas canvas("genie_monitoring", "GENIE electron monitoring", 800, 600);
        legacy_electron.Draw("colz");
        canvas.Print((output / "monitoring_plots/genie.pdf").string().c_str());
        canvas.Print((output / "monitoring_plots/theta_e_VS_phi_e.png").string().c_str());
    }
    writer.finish(scanned);
    std::cout << "Scanned " << scanned << ", wrote " << writer.count() << " events to " << c.get("output") << '\n';
}
}  // namespace samples
