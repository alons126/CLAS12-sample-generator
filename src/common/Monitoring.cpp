//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file Monitoring.cpp
 * @brief Implements shared per-particle ROOT diagnostics for LUND creation.
 *
 * Purpose:
 *   Record a source-independent view of the particles that were accepted by a
 *   LUND-creation workflow. Uniform and physical adapters can add specialized
 *   monitoring elsewhere while using this implementation for common momentum,
 *   angle, vertex and correlation plots.
 *
 * Workflow:
 *   1. Monitoring constructs an empty private implementation for one run.
 *   2. fill() creates a detached histogram family when it first sees a PDG code.
 *   3. Each particle contributes its kinematics and event vertex to that family.
 *   4. save() creates a ROOT file and writes all accumulated histogram objects.
 *
 * Units and assumptions:
 *   Momentum magnitudes are in GeV/c, beam energy is in GeV, angles are stored
 *   in degrees, and vertex coordinates are in centimeters. This module assumes
 *   that configuration validation has already supplied a positive beam energy.
 *
 * Failure behavior:
 *   Histogram allocation follows normal C++/ROOT exception behavior. save()
 *   throws std::runtime_error when ROOT cannot create the requested file or
 *   reports that a histogram was not written.
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

// Monitoring::Impl object -----------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Monitoring::Impl object */

/**
 * @struct Monitoring::Impl
 * @brief Private run-local storage for common diagnostic histograms.
 *
 * Purpose:
 *   Keep ROOT types and histogram bookkeeping out of Monitoring's public header.
 *
 * Lifecycle and ownership:
 *   Monitoring creates and exclusively owns one Impl. The constructor records
 *   the beam scale, fill() lazily inserts Plots records, save() reads them, and
 *   destruction releases every detached histogram through std::unique_ptr.
 *
 * Invariants:
 *   Each key in plots is a particle PDG code and owns exactly one complete
 *   histogram family after its first fill. beam is expressed in GeV and is used
 *   only to define the upper edge of momentum-related axes.
 */
struct Monitoring::Impl {
    double beam;  ///< Beam energy in GeV, used to choose momentum-axis ranges.

    // Plots object ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Plots object */

    /**
     * @struct Plots
     * @brief Owned histogram family for one PDG species.
     *
     * Creation and use:
     *   fill() initializes every member together when the associated species is
     *   first observed, then reuses the same objects for later particles.
     *
     * Ownership and units:
     *   SetDirectory(nullptr) detaches each histogram from ROOT directory
     *   ownership, so these unique pointers remain their sole owners. Momentum
     *   is in GeV/c, theta and phi are in degrees, and vertices are in cm.
     *
     * Naming convention:
     *   Two-dimensional member names use y_vs_x ordering. For example,
     *   theta_phi is filled with x = phi and y = theta.
     */
    struct Plots {
        std::unique_ptr<TH1D> p, theta, phi, vx, vy, vz;  ///< One-dimensional particle quantities.
        std::unique_ptr<TH2D> theta_phi, theta_p, phi_p;  ///< Two-dimensional correlations named y_vs_x.
    };

#pragma endregion

    std::map<int, Plots> plots;  ///< PDG-keyed families, allocated only for observed species.
};

#pragma endregion

// Monitoring::Monitoring ------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Monitoring::Monitoring */

/**
 * @brief Initialize run-local diagnostic storage.
 *
 * Algorithm:
 *   Allocate the private implementation and retain the configured beam energy.
 *   No ROOT histogram is allocated until a particle is passed to fill().
 *
 * @param beam Beam energy in GeV.
 *
 * @post impl_ owns an empty PDG-to-histogram map.
 */
Monitoring::Monitoring(double beam) : impl_(std::make_unique<Impl>()) { impl_->beam = beam; }

#pragma endregion

// Monitoring destruction ------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Histogram destruction */

/**
 * @brief Release the run-owned implementation and all detached ROOT objects.
 *
 * Defining the destructor here allows Monitoring.h to keep Impl incomplete.
 */
Monitoring::~Monitoring() = default;

#pragma endregion

// Monitoring::fill ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Monitoring::fill */

/**
 * @brief Record every written particle in common diagnostics.
 *
 * Algorithm:
 *   1. Find or insert the histogram family keyed by the particle's PDG code.
 *   2. For a new species, allocate all one- and two-dimensional histograms and
 *      detach them from ROOT directory ownership.
 *   3. Convert ROOT's angular values from radians to degrees.
 *   4. Fill momentum, angular, vertex and correlation histograms.
 *
 * @param event Successfully written event to observe. The function borrows it
 *              for this call and does not modify or retain it.
 *
 * @note Empty events contribute no entries. Histogram contents accumulate for
 *       the lifetime of this Monitoring object.
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
        const double p = particle.momentum.Mag();
        const double theta = particle.momentum.Theta() * TMath::RadToDeg();
        const double phi = particle.momentum.Phi() * TMath::RadToDeg();

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

// Monitoring::save ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Monitoring::save */

/**
 * @brief Write the common diagnostic ROOT file.
 *
 * Algorithm:
 *   1. Open the requested path in ROOT CREATE mode so an existing result is not
 *      silently overwritten.
 *   2. Visit PDG families in map order and write every owned histogram.
 *   3. Treat a non-positive ROOT Write() result as a failure.
 *   4. Close the output file after all objects have been persisted.
 *
 * @param path Destination ROOT-file path. Ownership of the path remains with
 *             the caller; this function creates the file itself.
 *
 * @throws std::runtime_error If ROOT cannot create the destination or cannot
 *                            write any histogram.
 */
void Monitoring::save(const std::filesystem::path& path) {
    TFile out(path.string().c_str(), "CREATE");

    if (out.IsZombie()) { throw std::runtime_error("Cannot create monitoring ROOT file"); }

    for (const auto& [pid, h] : impl_->plots) {
        for (TH1* plot : std::initializer_list<TH1*>{static_cast<TH1*>(h.p.get()), h.theta.get(), h.phi.get(), h.vx.get(), h.vy.get(), h.vz.get(), static_cast<TH1*>(h.theta_phi.get()),
                                                     static_cast<TH1*>(h.theta_p.get()), static_cast<TH1*>(h.phi_p.get())}) {
            if (plot->Write() <= 0) { throw std::runtime_error("Failed to write monitoring histogram"); }
        }
    }

    out.Close();
}

#pragma endregion

}  // namespace samples
