//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file Event.h
 * @brief Shared event and particle records.
 *
 * Purpose:
 *   Define the Event and Particle values shared by uniform generation, physical conversion, the LUND
 *   writer, and monitoring. These records hold event data but do not read input or write files.
 *
 * Workflow:
 *   A generator or converter creates an Event -> fills its header values -> adds particles in output
 *   order with one shared vertex -> LundWriter writes it -> monitoring may read the written event.
 */

#pragma once
#include <TVector3.h>

#include <cstdint>
#include <vector>

namespace samples {

// Public interface ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Public interface */

// Supported particle identities -----------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Supported particle identities */
/**
 * @namespace constants
 * @brief PDG identifiers supported by maintained LUND event producers.
 *
 * Purpose:
 *   Keep the supported particle numbers in one place. particleMass() gets nonzero masses from the
 *   external target file, so these constants do not create another mass table.
 */
namespace constants {
constexpr int electron_pdg = 11;    ///< Electron identifier used for the beam and scattered electron.
constexpr int photon_pdg = 22;      ///< Photon identifier retained from physical GST truth.
constexpr int pi_plus_pdg = 211;    ///< Positive charged-pion identifier.
constexpr int pi_minus_pdg = -211;  ///< Negative charged-pion identifier.
constexpr int neutron_pdg = 2112;   ///< Neutron identifier.
constexpr int proton_pdg = 2212;    ///< Proton identifier.
}  // namespace constants
#pragma endregion

// Particle object -------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Particle object */
/**
 * @struct Particle
 * @brief Data for one particle that will be written to LUND.
 *
 * Purpose:
 *   Store the particle number, mass, momentum, and interaction position needed by the writer and
 *   monitoring code.
 *
 * Creation and use:
 *   A generator or converter fills every member and adds the Particle to Event::particles. The Event
 *   owns the stored copy. The writer and monitoring code only read it.
 */
struct Particle {
    int pid;            ///< Supported PDG particle number written to LUND and used by monitoring.
    double mass;        ///< Rest mass in GeV/c², normally returned by particleMass().
    TVector3 momentum;  ///< Generated or input Cartesian momentum in GeV/c; the writer does not change it.
    TVector3 vertex;    ///< Interaction position in cm; every particle in one event uses the same value.
};
#pragma endregion

// Event object ----------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Event object */
/**
 * @struct Event
 * @brief Data for one event passed to the writer and monitoring code.
 *
 * Purpose:
 *   Store the particles and LUND header values for one event. Other components write the text files,
 *   split them, record the manifest, and save monitoring data.
 *
 * Creation and workflow:
 *   The generator or converter creates and fills a new Event. LundWriter reads it without changing it.
 *   Uniform monitoring reads it only after the writer has successfully written it.
 *
 * Header semantics:
 *   A and Z describe the target nucleus but do not choose the vertex shape. Physical conversion stores
 *   GST `resid` in the old target-polarization field; uniform events store zero there. Uniform events
 *   use weight 1. Physical conversion uses that field for process codes 1 through 4, not as a physics
 *   weight. LundWriter also writes zero beam polarization, electron beam PID 11, and one interaction.
 *
 * Ordering and invariants:
 *   particles must not be empty and all stored numbers must be finite. The electron comes first. A
 *   uniform hadron comes second, while physical conversion keeps supported final-state input order.
 */
struct Event {
    std::uint64_t id = 0;             ///< Uniform event index or scanned GST entry index. Uniform LUND text may use a per-file index instead.
    int A = 1;                        ///< Target mass number written in the LUND header.
    int Z = 1;                        ///< Target charge number, configured separately from the vertex geometry.
    double beam_energy = 0;           ///< Incident-electron energy in GeV.
    double resonance_id = 0;          ///< Old target-polarization field; stores GST `resid` for physical events.
    double weight = 1;                ///< Uniform value 1 or physical process code; not a cross-section weight.
    std::vector<Particle> particles;  ///< Particles in output order; the writer rejects an empty list.
};
#pragma endregion

// particleMass ----------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* particleMass */
/**
 * @brief Return the mass used for one supported output particle.
 * @param pid PDG code for electron, proton, neutron, charged pion, or photon. Neutral pions are not
 *            output particles; physical inputs must contain their upstream-generated decay photons.
 * @return Particle mass in GeV/c².
 * @throws std::runtime_error If pid is not supported by the LUND workflow.
 * @note Nonzero masses come from external targets.h. The generator or converter decides which particles
 *       belong in an event before calling this function.
 */
double particleMass(int pid);
#pragma endregion

#pragma endregion

}  // namespace samples
