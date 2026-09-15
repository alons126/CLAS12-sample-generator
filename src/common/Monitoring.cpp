//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file Monitoring.cpp
 * @brief Common momentum, angle and vertex diagnostics.
 *
 * Purpose:
 *   Allocate per-PDG ROOT histograms on first use and persist their numerical contents.
 *
 * Workflow:
 *   Create detached histograms -> fill particle quantities -> write a new ROOT file.
 */

#include "common/Monitoring.h"

#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TMath.h>

#include <initializer_list>
#include <map>
#include <stdexcept>
namespace samples {
// Monitoring::Impl object ------------------------------------------------
#pragma region /* Monitoring::Impl object */
/**
 * @struct Monitoring::Impl
 * @brief Private run-local storage for common diagnostic histograms.
 *
 * Purpose: keep ROOT implementation details out of the public Monitoring header.
 * Lifecycle: constructor stores beam; fill lazily inserts a Plots record per PDG; save reads
 * all records. Monitoring owns this object and releases the detached histograms at destruction.
 */
struct Monitoring::Impl {
    double beam;  ///< Beam energy in GeV, used to choose momentum-axis ranges.
    // Plots object ------------------------------------------------
#pragma region /* Plots object */
    /**
     * @struct Plots
     * @brief Owned histogram family for one PDG species.
     *
     * Usage: fill allocates every pointer together when the species first appears.
     * Histograms are detached from ROOT directories; unique_ptr controls their lifetime.
     * Momentum is in GeV, theta/phi in degrees, and vertex coordinates in cm.
     */
    struct Plots {
        std::unique_ptr<TH1D> p, theta, phi, vx, vy, vz;  ///< One-dimensional particle quantities.
        // Names are y_vs_x: e.g. theta_phi is filled with (phi, theta).
        std::unique_ptr<TH2D> theta_phi, theta_p, phi_p;
    };
#pragma endregion
    std::map<int, Plots> plots;  ///< PDG-keyed families, allocated only for observed species.
};
#pragma endregion
// Monitoring::Monitoring ----------------------------------------------------------------------

#pragma region /* Monitoring::Monitoring */
/**
 * @brief Initialize run-local diagnostic storage.
 *
 * Algorithm:
 *   Allocate an implementation object and store the beam scale for histogram axes.
 *
 * @param beam Beam energy in GeV.
 *
 * @note Constructor; histograms are allocated when each PDG is first observed.
 */
Monitoring::Monitoring(double beam) : impl_(std::make_unique<Impl>()) { impl_->beam = beam; }
#pragma endregion

// Monitoring destruction ------------------------------------------------------
#pragma region /* Histogram destruction */
/** @brief Release the run-owned diagnostic implementation and its detached ROOT objects. */
Monitoring::~Monitoring() = default;
#pragma endregion

// Monitoring::fill ----------------------------------------------------------------------

#pragma region /* Monitoring::fill */
/**
 * @brief Record every written particle in common diagnostics.
 *
 * Algorithm:
 *   Create detached histograms for new PDGs, then fill momenta, angles, vertices and correlations.
 *
 * @param event Successfully written event.
 *
 * @note No value; histogram contents accumulate within this run.
 */
void Monitoring::fill(const Event& event) {
    for (const auto& particle : event.particles) {
        auto [it, inserted] = impl_->plots.try_emplace(particle.pid);
        auto& h = it->second;
        // Allocate detached ROOT histograms only when this PDG first appears.
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
        // Convert angles to degrees and retain momentum/vertex units from the event.
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
#pragma endregion

// Monitoring::save ----------------------------------------------------------------------

#pragma region /* Monitoring::save */
/**
 * @brief Write the common diagnostic ROOT file.
 *
 * Algorithm:
 *   Create a new file, write every owned histogram, check write status, and close.
 *
 * @param path Destination ROOT file.
 *
 * @note No value; creation or histogram write failures throw.
 */
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
#pragma endregion

}  // namespace samples
