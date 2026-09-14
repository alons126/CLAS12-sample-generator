#include "common/LegacyMonitoring.h"

#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TMath.h>
#include <TROOT.h>

#include <stdexcept>
#include <vector>
namespace samples {
struct LegacyMonitoring::Impl {
    struct Entry {
        std::unique_ptr<TH1> histogram;
        std::string x, y;
    };
    std::vector<Entry> entries;
};
namespace {
double value(const std::string& key, const Event& event) {
    int pid = key.back() == 'e' ? 11 : key.back() == 'p' ? 2212 : 2112;
    for (const auto& p : event.particles) {
        if (p.pid != pid) continue;
        auto metric = key.substr(0, key.size() - 2);
        if (metric == "Theta") return p.momentum.Theta() * TMath::RadToDeg();
        if (metric == "Phi") return p.momentum.Phi() * TMath::RadToDeg();
        if (metric == "P") return p.momentum.Mag();
        if (metric == "Vx") return p.vertex.X();
        if (metric == "Vy") return p.vertex.Y();
        if (metric == "Vz") return p.vertex.Z();
    }
    throw std::runtime_error("No histogram quantity " + key);
}
}  // namespace
LegacyMonitoring::LegacyMonitoring(const std::string& channel, double Ebeam) : impl_(std::make_unique<Impl>()) {
    auto add = [&](std::unique_ptr<TH1> h, std::string x, std::string y) {
        h->SetDirectory(nullptr);
        impl_->entries.push_back({std::move(h), std::move(x), std::move(y)});
    };
    if (channel == "Tester_e") {
        add(std::make_unique<TH1D>("Theta_e_Tester_e", "#theta_{e} in Tester_e sample;#theta_{e} [#circ]", 100, 0, 50), "Theta_e", "");
        add(std::make_unique<TH1D>("Phi_e_Tester_e", "#phi_{e} in Tester_e sample;#phi_{e} [#circ]", 100, -180, 180), "Phi_e", "");
        add(std::make_unique<TH1D>("P_e_Tester_e", "P_{e} in Tester_e sample;P_{e} [GeV]", 100, 0, Ebeam * 1.1), "P_e", "");
        add(std::make_unique<TH2D>("Theta_e_VS_Phi_e_Tester_e", "#theta_{e} vs. #phi_{e} in Tester_e sample;#phi_{e} [#circ];#theta_{e} [#circ]", 100, -180, 180, 100, 0, 50), "Phi_e",
            "Theta_e");
        add(std::make_unique<TH2D>("Theta_e_VS_P_e_Tester_e", "#theta_{e} vs. P_{e} in Tester_e sample;P_{e} [GeV];#theta_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50), "P_e", "Theta_e");
        add(std::make_unique<TH2D>("Phi_e_VS_P_e_Tester_e", "#phi_{e} vs. P_{e} in Tester_e sample;P_{e} [GeV];#phi_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180), "P_e", "Phi_e");
    }
    if (channel == "1e") {
        add(std::make_unique<TH1D>("Theta_e_1e", "#theta_{e} in (e,e') sample;#theta_{e} [#circ]", 100, 0, 50), "Theta_e", "");
        add(std::make_unique<TH1D>("Phi_e_1e", "#phi_{e} in (e,e') sample;#phi_{e} [#circ]", 100, -180, 180), "Phi_e", "");
        add(std::make_unique<TH1D>("P_e_1e", "P_{e} in (e,e') sample;P_{e} [GeV]", 100, 0, Ebeam * 1.1), "P_e", "");
        add(std::make_unique<TH1D>("Vx_e_1e", "V_{e,x} of e in (e,e') sample;V_{e,x} [cm]", 100, -5, 5), "Vx_e", "");
        add(std::make_unique<TH1D>("Vy_e_1e", "V_{e,y} of e in (e,e') sample;V_{e,y} [cm]", 100, -5, 5), "Vy_e", "");
        add(std::make_unique<TH1D>("Vz_e_1e", "V_{e,z} of e in (e,e') sample;V_{e,z} [cm]", 100, -5, 5), "Vz_e", "");
        add(std::make_unique<TH2D>("Theta_e_VS_Phi_e_1e", "#theta_{e} vs. #phi_{e} in (e,e') sample;#phi_{e} [#circ];#theta_{e} [#circ]", 100, -180, 180, 100, 0, 50), "Phi_e", "Theta_e");
        add(std::make_unique<TH2D>("Theta_e_VS_P_e_1e", "#theta_{e} vs. P_{e} in (e,e') sample;P_{e} [GeV];#theta_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50), "P_e", "Theta_e");
        add(std::make_unique<TH2D>("Phi_e_VS_P_e_1e", "#phi_{e} vs. P_{e} in (e,e') sample;P_{e} [GeV];#phi_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180), "P_e", "Phi_e");
    }
    if (channel == "ep") {
        add(std::make_unique<TH1D>("Theta_e_ep", "#theta_{e} in (e,e'p) sample;#theta_{e} [#circ]", 100, 0, 50), "Theta_e", "");
        add(std::make_unique<TH1D>("Phi_e_ep", "#phi_{e} in (e,e'p) sample;#phi_{e} [#circ]", 100, -180, 180), "Phi_e", "");
        add(std::make_unique<TH1D>("P_e_ep", "P_{e} in (e,e'p) sample;P_{e} [GeV]", 100, 0, Ebeam * 1.1), "P_e", "");
        add(std::make_unique<TH1D>("Vx_e_ep", "V_{e,x} of e in (e,e'p) sample;V_{e,x} [cm]", 100, -5, 5), "Vx_e", "");
        add(std::make_unique<TH1D>("Vy_e_ep", "V_{e,y} of e in (e,e'p) sample;V_{e,y} [cm]", 100, -5, 5), "Vy_e", "");
        add(std::make_unique<TH1D>("Vz_e_ep", "V_{e,z} of e in (e,e'p) sample;V_{e,z} [cm]", 100, -5, 5), "Vz_e", "");
        add(std::make_unique<TH2D>("Theta_e_VS_Phi_e_ep", "#theta_{e} vs. #phi_{e} in (e,e'p) sample;#phi_{e} [#circ];#theta_{e} [#circ]", 100, -180, 180, 100, 0, 50), "Phi_e", "Theta_e");
        add(std::make_unique<TH2D>("Theta_e_VS_P_e_ep", "#theta_{e} vs. P_{e} in (e,e'p) sample;P_{e} [GeV];#theta_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50), "P_e", "Theta_e");
        add(std::make_unique<TH2D>("Phi_e_VS_P_e_ep", "#phi_{e} vs. P_{e} in (e,e'p) sample;P_{e} [GeV];#phi_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180), "P_e", "Phi_e");
        add(std::make_unique<TH1D>("Theta_p_ep", "#theta_{p} in (e,e'p) sample;#theta_{p} [#circ]", 100, 0, 50), "Theta_p", "");
        add(std::make_unique<TH1D>("Phi_p_ep", "#phi_{p} in (e,e'p) sample;#phi_{p} [#circ]", 100, -180, 180), "Phi_p", "");
        add(std::make_unique<TH1D>("P_p_ep", "P_{p} in (e,e'p) sample;P_{p} [GeV]", 100, 0, Ebeam * 1.1), "P_p", "");
        add(std::make_unique<TH1D>("Vx_p_ep", "V_{p,x} of p in (e,e'p) sample;V_{p,x} [cm]", 100, -5, 5), "Vx_p", "");
        add(std::make_unique<TH1D>("Vy_p_ep", "V_{p,y} of p in (e,e'p) sample;V_{p,y} [cm]", 100, -5, 5), "Vy_p", "");
        add(std::make_unique<TH1D>("Vz_p_ep", "V_{p,z} of p in (e,e'p) sample;V_{p,z} [cm]", 100, -5, 5), "Vz_p", "");
        add(std::make_unique<TH2D>("Theta_p_VS_Phi_p_ep", "#theta_{p} vs. #phi_{p} in (e,e'p) sample;#phi_{p} [#circ];#theta_{p} [#circ]", 100, -180, 180, 100, 0, 50), "Phi_p", "Theta_p");
        add(std::make_unique<TH2D>("Theta_p_VS_P_p_ep", "#theta_{p} vs. P_{p} in (e,e'p) sample;P_{p} [GeV];#theta_{p} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50), "P_p", "Theta_p");
        add(std::make_unique<TH2D>("Phi_p_VS_P_p_ep", "#phi_{p} vs. P_{p} in (e,e'p) sample;P_{p} [GeV];#phi_{p} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180), "P_p", "Phi_p");
        add(std::make_unique<TH2D>("P_e_VS_P_p_ep", "P_{e} vs. P_{p} in (e,e'p) sample;P_{p} [GeV];P_{e} [GeV]", 100, 0, Ebeam * 1.1, 100, 0, Ebeam * 1.1), "P_p", "P_e");
        add(std::make_unique<TH2D>("P_e_VS_Theta_p_ep", "P_{e} vs. #theta_{p} in (e,e'p) sample;#theta_{p} [#circ];P_{e} [GeV]", 100, 0, 50, 100, 0, Ebeam * 1.1), "Theta_p", "P_e");
        add(std::make_unique<TH2D>("P_e_VS_Phi_p_ep", "P_{e} vs. #phi_{p} in (e,e'p) sample;#phi_{p} [#circ];P_{e} [GeV]", 100, -180, 180, 100, 0, Ebeam * 1.1), "Phi_p", "P_e");
        add(std::make_unique<TH2D>("Theta_e_VS_P_p_ep", "#theta_{e} vs. P_{p} in (e,e'p) sample;P_{p} [GeV];#theta_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50), "P_p", "Theta_e");
        add(std::make_unique<TH2D>("Theta_e_VS_Theta_p_ep", "#theta_{e} vs. #theta_{p} in (e,e'p) sample;#theta_{p} [#circ];#theta_{e} [#circ]", 100, 0, 50, 100, 0, 50), "Theta_p",
            "Theta_e");
        add(std::make_unique<TH2D>("Theta_e_VS_Phi_p_ep", "#theta_{e} vs. #phi_{p} in (e,e'p) sample;#phi_{p} [#circ];#theta_{e} [#circ]", 100, -180, 180, 100, 0, 50), "Phi_p", "Theta_e");
        add(std::make_unique<TH2D>("Phi_e_VS_P_p_ep", "#phi_{e} vs. P_{p} in (e,e'p) sample;P_{p} [GeV];#phi_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180), "P_p", "Phi_e");
        add(std::make_unique<TH2D>("Phi_e_VS_Theta_p_ep", "#phi_{e} vs. #theta_{p} in (e,e'p) sample;#theta_{p} [#circ];#phi_{e} [#circ]", 100, 0, 50, 100, -180, 180), "Theta_p", "Phi_e");
        add(std::make_unique<TH2D>("Phi_e_VS_Phi_p_ep", "#phi_{e} vs. #phi_{p} in (e,e'p) sample;#phi_{p} [#circ];#phi_{e} [#circ]", 100, -200, 200, 100, -200, 200), "Phi_p", "Phi_e");
    }
    if (channel == "en") {
        add(std::make_unique<TH1D>("Theta_e_en", "#theta_{e} in (e,e'n) sample;#theta_{e} [#circ]", 100, 0, 50), "Theta_e", "");
        add(std::make_unique<TH1D>("Phi_e_en", "#phi_{e} in (e,e'n) sample;#phi_{e} [#circ]", 100, -180, 180), "Phi_e", "");
        add(std::make_unique<TH1D>("P_e_en", "P_{e} in (e,e'n) sample;P_{e} [GeV]", 100, 0, Ebeam * 1.1), "P_e", "");
        add(std::make_unique<TH1D>("Vx_e_en", "V_{e,x} of e in (e,e'n) sample;V_{e,x} [cm]", 100, -5, 5), "Vx_e", "");
        add(std::make_unique<TH1D>("Vy_e_en", "V_{e,y} of e in (e,e'n) sample;V_{e,y} [cm]", 100, -5, 5), "Vy_e", "");
        add(std::make_unique<TH1D>("Vz_e_en", "V_{e,z} of e in (e,e'n) sample;V_{e,z} [cm]", 100, -5, 5), "Vz_e", "");
        add(std::make_unique<TH2D>("Theta_e_VS_Phi_e_en", "#theta_{e} vs. #phi_{e} in (e,e'n) sample;#phi_{e} [#circ];#theta_{e} [#circ]", 100, -180, 180, 100, 0, 50), "Phi_e", "Theta_e");
        add(std::make_unique<TH2D>("Theta_e_VS_P_e_en", "#theta_{e} vs. P_{e} in (e,e'n) sample;P_{e} [GeV];#theta_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50), "P_e", "Theta_e");
        add(std::make_unique<TH2D>("Phi_e_VS_P_e_en", "#phi_{e} vs. P_{e} in (e,e'n) sample;P_{e} [GeV];#phi_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180), "P_e", "Phi_e");
        add(std::make_unique<TH1D>("Theta_n_en", "#theta_{n} in (e,e'n) sample;#theta_{n} [#circ]", 100, 0, 50), "Theta_n", "");
        add(std::make_unique<TH1D>("Phi_n_en", "#phi_{n} in (e,e'n) sample;#phi_{n} [#circ]", 100, -180, 180), "Phi_n", "");
        add(std::make_unique<TH1D>("P_n_en", "P_{n} in (e,e'n) sample;P_{n} [GeV]", 100, 0, Ebeam * 1.1), "P_n", "");
        add(std::make_unique<TH1D>("Vx_n_en", "V_{n,x} of n in (e,e'n) sample;V_{n,x} [cm]", 100, -5, 5), "Vx_n", "");
        add(std::make_unique<TH1D>("Vy_n_en", "V_{n,y} of n in (e,e'n) sample;V_{n,y} [cm]", 100, -5, 5), "Vy_n", "");
        add(std::make_unique<TH1D>("Vz_n_en", "V_{n,z} of n in (e,e'n) sample;V_{n,z} [cm]", 100, -5, 5), "Vz_n", "");
        add(std::make_unique<TH2D>("Theta_n_VS_Phi_n_en", "#theta_{n} vs. #phi_{n} in (e,e'n) sample;#phi_{n} [#circ];#theta_{n} [#circ]", 100, -180, 180, 100, 0, 50), "Phi_n", "Theta_n");
        add(std::make_unique<TH2D>("Theta_n_VS_P_n_en", "#theta_{n} vs. P_{n} in (e,e'n) sample;P_{n} [GeV];#theta_{n} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50), "P_n", "Theta_n");
        add(std::make_unique<TH2D>("Phi_n_VS_P_n_en", "#phi_{n} vs. P_{n} in (e,e'n) sample;P_{n} [GeV];#phi_{n} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180), "P_n", "Phi_n");
        add(std::make_unique<TH2D>("P_e_VS_P_n_en", "P_{e} vs. P_{n} in (e,e'n) sample;P_{n} [GeV];P_{e} [GeV]", 100, 0, Ebeam * 1.1, 100, 0, Ebeam * 1.1), "P_n", "P_e");
        add(std::make_unique<TH2D>("P_e_VS_Theta_n_en", "P_{e} vs. #theta_{n} in (e,e'n) sample;#theta_{n} [#circ];P_{e} [GeV]", 100, 0, 50, 100, 0, Ebeam * 1.1), "Theta_n", "P_e");
        add(std::make_unique<TH2D>("P_e_VS_Phi_n_en", "P_{e} vs. #phi_{n} in (e,e'n) sample;#phi_{n} [#circ];P_{e} [GeV]", 100, -180, 180, 100, 0, Ebeam * 1.1), "Phi_n", "P_e");
        add(std::make_unique<TH2D>("Theta_e_VS_P_n_en", "#theta_{e} vs. P_{n} in (e,e'n) sample;P_{n} [GeV];#theta_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50), "P_n", "Theta_e");
        add(std::make_unique<TH2D>("Theta_e_VS_Theta_n_en", "#theta_{e} vs. #theta_{n} in (e,e'n) sample;#theta_{n} [#circ];#theta_{e} [#circ]", 100, 0, 50, 100, 0, 50), "Theta_n",
            "Theta_e");
        add(std::make_unique<TH2D>("Theta_e_VS_Phi_n_en", "#theta_{e} vs. #phi_{n} in (e,e'n) sample;#phi_{n} [#circ];#theta_{e} [#circ]", 100, -180, 180, 100, 0, 50), "Phi_n", "Theta_e");
        add(std::make_unique<TH2D>("Phi_e_VS_P_n_en", "#phi_{e} vs. P_{n} in (e,e'n) sample;P_{n} [GeV];#phi_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180), "P_n", "Phi_e");
        add(std::make_unique<TH2D>("Phi_e_VS_Theta_n_en", "#phi_{e} vs. #theta_{n} in (e,e'n) sample;#theta_{n} [#circ];#phi_{e} [#circ]", 100, 0, 50, 100, -180, 180), "Theta_n", "Phi_e");
        add(std::make_unique<TH2D>("Phi_e_VS_Phi_n_en", "#phi_{e} vs. #phi_{n} in (e,e'n) sample;#phi_{n} [#circ];#phi_{e} [#circ]", 100, -200, 200, 100, -200, 200), "Phi_n", "Phi_e");
    }
}
LegacyMonitoring::~LegacyMonitoring() = default;
void LegacyMonitoring::fill(const Event& event) {
    for (auto& entry : impl_->entries) {
        if (entry.y.empty())
            entry.histogram->Fill(value(entry.x, event));
        else
            static_cast<TH2*>(entry.histogram.get())->Fill(value(entry.x, event), value(entry.y, event));
    }
}
void LegacyMonitoring::save(const std::filesystem::path& path, bool render) {
    TFile out(path.string().c_str(), "CREATE");
    if (out.IsZombie()) throw std::runtime_error("Cannot create legacy monitoring file");
    for (auto& entry : impl_->entries) {
        entry.histogram->Sumw2();
        if (entry.histogram->Write() <= 0) throw std::runtime_error("Cannot write legacy histogram");
    }
    out.Close();
    if (render) {
        gROOT->SetBatch(true);
        const auto directory = path.parent_path() / "monitoring_plots";
        std::filesystem::create_directory(directory);
        const auto pdf = (directory / "uniform.pdf").string();
        TCanvas canvas("sample_monitoring", "Uniform sample monitoring", 1000, 750);
        canvas.Print((pdf + "[").c_str());
        for (auto& entry : impl_->entries) {
            canvas.Clear();
            canvas.SetGrid();
            entry.histogram->Draw(entry.y.empty() ? "hist" : "colz");
            canvas.Print(pdf.c_str());
            canvas.Print((directory / (std::string(entry.histogram->GetName()) + ".png")).string().c_str());
        }
        canvas.Print((pdf + "]").c_str());
    }
}
}  // namespace samples
