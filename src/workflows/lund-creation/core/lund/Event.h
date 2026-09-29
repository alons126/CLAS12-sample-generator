/**
 * @file Event.h
 * @brief Shared event and particle records.
 *
 * Purpose:
 *   Define the Event and Particle values shared by the uniform LUND creator, physical LUND converter,
 *   LUND writer, and monitoring. These records hold event data but do not read input or write files.
 *
 * Execution flow:
 *   The uniform LUND creator or physical LUND converter creates an Event -> fills its header values ->
 *   adds particles in output order with one shared vertex position -> LundWriter writes it -> monitoring may read
 *   the written event.
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
 * @brief PDG identifiers supported by the LUND event producers.
 *
 * Purpose:
 *   Keep the supported particle numbers in one place. getParticleMass() gets nonzero masses from the
 *   external target file, so these constants do not create another mass table.
 */
namespace constants {
constexpr int electron_pdg = 11;    ///< Electron identifier used for the beam and scattered electron.
constexpr int proton_pdg = 2212;    ///< Proton identifier.
constexpr int neutron_pdg = 2112;   ///< Neutron identifier.
constexpr int pi_plus_pdg = 211;    ///< Positive charged-pion identifier.
constexpr int pi_minus_pdg = -211;  ///< Negative charged-pion identifier.
constexpr int photon_pdg = 22;      ///< Photon identifier retained from physical GST truth.
}  // namespace constants
#pragma endregion

// Particle object -------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Particle object */
/**
 * @struct Particle
 * @brief Data for one particle that will be written to LUND.
 *
 * Purpose:
 *   Store the particle number, mass, momentum, and LUND vertex position needed by the writer and
 *   monitoring code.
 *
 * Creation and use:
 *   The uniform LUND creator or physical LUND converter fills every member and adds the Particle to
 *   Event::particles. The Event owns the stored copy. The writer and monitoring code only read it.
 */
struct Particle {
    int pid;            ///< Supported PDG particle number written to LUND and used by monitoring.
    double mass;        ///< Rest mass in GeV/c², normally returned by getParticleMass().
    TVector3 momentum;  ///< Generated or input Cartesian momentum in GeV/c; the writer does not change it.
    TVector3 vertex;    ///< LUND vertex position in cm; every particle in one event uses the same coordinates.
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
 *   The uniform LUND creator or physical LUND converter creates and fills a new Event. LundWriter reads
 *   it without changing it. Uniform monitoring reads it only after the writer has successfully written it.
 *
 * Header semantics:
 *   A and Z describe the target nucleus but do not choose the vertex geometry (liquid, 1-foil, 4-foil,
 *   etc.). Physical conversion stores GST `resid` in header field 4; uniform events store zero there.
 *   Uniform events use weight 1. Physical conversion uses that field for process codes 1, 2, 3, and 4
 *   for the QE, MEC, RES, and DIS reaction mechanisms, respectively. LundWriter also writes zero beam
 *   polarization, electron beam PID 11, and one interaction.
 *
 * Ordering and invariants:
 *   particles must not be empty and all stored numbers must be finite. The electron comes first in the LUND format.
 */
struct Event {
    std::uint64_t id = 0;             ///< Run-global uniform event index or scanned GST input-entry index written unchanged to LUND.
    int A = 1;                        ///< Target mass number written in the LUND header.
    int Z = 1;                        ///< Target charge number, configured separately from the vertex geometry.
    double beam_energy = 0;           ///< Incident-electron energy in GeV.
    double resonance_id = 0;          ///< Header field 4; stores GST `resid` for physical events.
    double weight = 1;                ///< Uniform value 1 or physical process code.
    std::vector<Particle> particles;  ///< Particles in output order; the writer rejects an empty list.
};
#pragma endregion

// getParticleMass -------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* getParticleMass */
/**
 * @brief Return the mass used for one supported output particle.
 * @param pid PDG code for electron, proton, neutron, charged pion, or photon. Neutral pions are not
 *            output particles; physical inputs must contain their upstream-generated decay photons.
 * @return Particle mass in GeV/c².
 * @throws std::runtime_error If pid is not supported by the LUND workflow.
 * @note Nonzero masses come from external targets.h. The generator or converter decides which particles
 *       belong in an event before calling this function.
 */
double getParticleMass(int pid);
#pragma endregion

#pragma endregion

}  // namespace samples
