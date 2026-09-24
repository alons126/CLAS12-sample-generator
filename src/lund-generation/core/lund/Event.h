//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file Event.h
 * @brief Shared event and particle records.
 *
 * Purpose:
 *   Define the common Event and Particle data passed from uniform generation or physical conversion
 *   to the LUND writer and monitoring code. These records store event content only: source adapters
 *   still own input reading and particle-generation choices, while LundWriter owns text formatting.
 *
 * Workflow:
 *   A uniform sampler or physical adapter constructs one Event -> assigns header metadata -> appends
 *   particles in required output order with one shared interaction vertex -> LundWriter serializes it
 *   -> source-appropriate monitoring reads the successfully written event.
 */

#pragma once
#include <TVector3.h>

#include <cstdint>
#include <vector>

namespace samples {

// Public interface ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Public interface */

// Supported particle identities ----------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Supported particle identities */
/**
 * @namespace constants
 * @brief PDG identifiers supported by maintained LUND event producers.
 *
 * Purpose:
 *   Give generation, conversion, serialization, and monitoring one particle-identity vocabulary
 *   without coupling identity to a duplicate mass table. particleMass() obtains nonzero masses from
 *   the external target source.
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
 * @brief One output particle, owned by its event.
 *
 * Purpose:
 *   Carry the truth-level identity, selected mass convention, three-momentum, and interaction position
 *   required by both LUND serialization and diagnostic filling.
 *
 * Creation and use:
 *   Event sources aggregate-initialize every member and append the value to Event::particles. The Event
 *   owns that copy; LundWriter and monitoring borrow it while processing the parent event.
 */
struct Particle {
    int pid;            ///< Supported PDG identity used verbatim in LUND and for monitoring groups.
    double mass;        ///< LUND mass in GeV/c²; particleMass() normally supplies it from the target source.
    TVector3 momentum;  ///< Truth/generated Cartesian momentum in GeV/c; never resampled by the writer.
    TVector3 vertex;    ///< Interaction position in cm; producer invariant requires one value per event.
};
#pragma endregion

// Event object ----------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Event object */
/**
 * @struct Event
 * @brief Shared record passed from generation to output and diagnostics.
 *
 * Purpose:
 *   Store the particles and LUND header values for one event. Other components format the LUND text,
 *   split output files, track the manifest, and save monitoring data.
 *
 * Creation and workflow:
 *   A fresh value is created for each generated or retained input event, populated completely, passed
 *   by const reference to LundWriter, and then passed to monitoring only after successful serialization.
 *
 * Header semantics:
 *   A and Z describe the target nucleus but do not choose the vertex shape. GENIE stores GST `resid`
 *   in the old target-polarization field; uniform events store zero there. Uniform events use weight 1,
 *   while GENIE uses that field for the QE/MEC/RES/DIS code 1/2/3/4, not a cross-section weight.
 *   LundWriter also writes zero beam polarization, electron beam PID 11, and one interaction.
 *
 * Ordering and invariants:
 *   particles must be nonempty and finite. The scattered/generated electron is first; uniform electron–hadron generation
 *   places its selected hadron second, while physical conversion preserves supported final-state input order.
 */
struct Event {
    std::uint64_t id = 0;             ///< Run-global uniform index or scanned GST entry index. Legacy uniform text
                                      ///< may serialize a per-file index without changing this stored identity.
    int A = 1;                        ///< LUND target mass number; production profiles override the historical default.
    int Z = 1;                        ///< LUND target charge number; independently configured from target geometry.
    double beam_energy = 0;           ///< Configured incident-electron energy in GeV.
    double resonance_id = 0;          ///< Legacy target-polarization field; GST `resid` for physical events.
    double weight = 1;                ///< Uniform constant 1 or physical interaction tag; not a cross-section weight.
    std::vector<Particle> particles;  ///< Output order is preserved; the writer rejects an empty list.
};
#pragma endregion

// particleMass ----------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* particleMass */
/**
 * @brief Return the configured mass convention for one supported output species.
 *
 * @param pid PDG code for electron, proton, neutron, charged pion, or photon. Neutral pions are not
 *            output particles; physical inputs must contain their upstream-generated decay photons.
 * @return Particle mass in GeV/c².
 *
 * @throws std::runtime_error If pid is not part of the supported LUND particle contract.
 *
 * @note Nonzero values come from external targets.h. This lookup does not validate event-generator
 *       status or particle selection; adapters decide which truth particles are retained first.
 */
double particleMass(int pid);
#pragma endregion

#pragma endregion

}  // namespace samples
