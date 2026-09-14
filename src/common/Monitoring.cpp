#include "common/Monitoring.h"

#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TMath.h>

#include <initializer_list>
#include <map>
#include <stdexcept>
namespace samples {
struct Monitoring::Impl {
    double beam;
    struct Plots {
        std::unique_ptr<TH1D> p, theta, phi, vx, vy, vz;
        std::unique_ptr<TH2D> theta_phi, theta_p, phi_p;
    };
    std::map<int, Plots> plots;
};
Monitoring::Monitoring(double beam) : impl_(std::make_unique<Impl>()) { impl_->beam = beam; }
Monitoring::~Monitoring() = default;
void Monitoring::fill(const Event& event) {
    for (const auto& particle : event.particles) {
        auto [it, inserted] = impl_->plots.try_emplace(particle.pid);
        auto& h = it->second;
        if (inserted) {
            const auto prefix = "pid_" + std::to_string(particle.pid) + "_";
            auto one = [&](const char* name, double lo, double hi) {
                auto result = std::make_unique<TH1D>((prefix + name).c_str(), name, 100, lo, hi);
                result->SetDirectory(nullptr);
                return result;
            };
            auto two = [&](const char* name, double xl, double xh, double yl, double yh) {
                auto result = std::make_unique<TH2D>((prefix + name).c_str(), name, 100, xl, xh, 100, yl, yh);
                result->SetDirectory(nullptr);
                return result;
            };
            h.p = one("p_GeV", 0, impl_->beam * 1.1);
            h.theta = one("theta_deg", 0, 180);
            h.phi = one("phi_deg", -180, 180);
            h.vx = one("vx_cm", -0.3, 0.3);
            h.vy = one("vy_cm", -0.3, 0.3);
            h.vz = one("vz_cm", -7, 1);
            h.theta_phi = two("theta_vs_phi", -180, 180, 0, 180);
            h.theta_p = two("theta_vs_p", 0, impl_->beam * 1.1, 0, 180);
            h.phi_p = two("phi_vs_p", 0, impl_->beam * 1.1, -180, 180);
        }
        double p = particle.momentum.Mag(), theta = particle.momentum.Theta() * TMath::RadToDeg(), phi = particle.momentum.Phi() * TMath::RadToDeg();
        h.p->Fill(p);
        h.theta->Fill(theta);
        h.phi->Fill(phi);
        h.vx->Fill(particle.vertex.X());
        h.vy->Fill(particle.vertex.Y());
        h.vz->Fill(particle.vertex.Z());
        h.theta_phi->Fill(phi, theta);
        h.theta_p->Fill(p, theta);
        h.phi_p->Fill(p, phi);
    }
}
void Monitoring::save(const std::filesystem::path& path) {
    TFile out(path.string().c_str(), "CREATE");
    if (out.IsZombie()) throw std::runtime_error("Cannot create monitoring ROOT file");
    for (const auto& [pid, h] : impl_->plots) {
        for (TH1* plot : std::initializer_list<TH1*>{static_cast<TH1*>(h.p.get()), h.theta.get(), h.phi.get(), h.vx.get(), h.vy.get(), h.vz.get(), static_cast<TH1*>(h.theta_phi.get()),
                                                     static_cast<TH1*>(h.theta_p.get()), static_cast<TH1*>(h.phi_p.get())}) {
            if (plot->Write() <= 0) throw std::runtime_error("Failed to write monitoring histogram");
        }
    }
    out.Close();
}
}  // namespace samples
