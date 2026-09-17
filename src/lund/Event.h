//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file Event.h
 * @brief Shared event and particle records.
 *
 * Purpose:
 *   Provide one small in-memory contract between source-specific event construction and common LUND/
 *   monitoring consumers, without embedding ROOT-tree ownership, sampling policy, or serialization
 *   formatting in the records themselves.
 *
 * Workflow:
 *   A uniform sampler or physical adapter constructs one Event -> assigns header metadata -> appends
 *   particles in required output order with one shared interaction vertex -> LundWriter serializes it
 *   -> source-appropriate monitoring reads the successfully written event.
 *
 * Units and conventions:
 *   Particle mass is GeV/c², momentum is GeV/c, derived and beam energies are GeV, and vertices are cm.
 *   The writer uses c=1 for E=sqrt(m²+p²) and supplies fixed LUND status/parent fields not represented
 *   by these source-facing records.
 */

#pragma once
#include <TVector3.h>

#include <cstdint>
#include <vector>

namespace samples {

// Public interface ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Public interface */

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
 *
 * Ownership:
 *   TVector3 members are values and carry no input ROOT-file ownership. Particle has no independent
 *   lifetime requirements beyond the containing vector.
 *
 * Units and invariants:
 *   mass is GeV/c², Cartesian momentum is GeV/c, and vertex coordinates are cm. Producers supply finite
 *   values and assign the same vertex to every particle in one Event. Energy is deliberately absent;
 *   LundWriter derives the mass-shell value and rejects non-finite data.
 */
struct Particle {
    int pid;  ///< Supported PDG identity used verbatim in LUND and for monitoring groups.

    double mass;  ///< Central rounded LUND mass in GeV/c²; particleMass() normally supplies it.

    TVector3 momentum;  ///< Truth/generated Cartesian momentum in GeV/c; never resampled by the writer.

    TVector3 vertex;  ///< Interaction position in cm; producer invariant requires one value per event.
};
#pragma endregion

// Event object ----------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Event object */
/**
 * @struct Event
 * @brief Shared record passed from generation to output and diagnostics.
 *
 * Purpose:
 *   Represent the source-defined content and LUND header metadata of one writable event while leaving
 *   text precision, file splitting, manifest state, and monitoring storage to common consumers.
 *
 * Creation and workflow:
 *   A fresh value is created for each generated or retained input event, populated completely, passed
 *   by const reference to LundWriter, and then passed to monitoring only after successful serialization.
 *
 * Ownership and lifetime:
 *   The vector owns Particle values in insertion order. Writers and monitors borrow the Event only for
 *   the duration of a call; the record owns no file, RNG, target-geometry, or configuration state.
 *
 * Header semantics:
 *   A/Z are explicit nuclear metadata and never select vertex geometry. resonance_id occupies the
 *   historical target-polarization header position (GENIE supplies GST `resid`; uniform leaves zero).
 *   weight is 1 for uniform output and carries the GENIE QE/MEC/RES/DIS process tag 1/2/3/4; it is not
 *   a physical cross-section weight. LundWriter supplies zero beam polarization, beam PID 11, and one
 *   interaction.
 *
 * Ordering and invariants:
 *   particles must be nonempty and finite. The scattered/generated electron is first; uniform electron–hadron generation
 *   places its selected hadron second, while physical conversion preserves supported final-state input order.
 */
struct Event {
    std::uint64_t id = 0;  ///< Run-global uniform index or scanned GST entry index. Legacy uniform text
                           ///< may serialize a per-file index without changing this stored identity.

    int A = 1;  ///< LUND target mass number; production profiles override the historical default.
    int Z = 1;  ///< LUND target charge number; independently configured from target geometry.

    double beam_energy = 0;  ///< Configured incident-electron energy in GeV.

    double resonance_id = 0;  ///< Legacy target-polarization field; GST `resid` for physical events.

    double weight = 1;  ///< Uniform constant 1 or physical interaction tag; not a cross-section weight.

    std::vector<Particle> particles;  ///< Output order is preserved; the writer rejects an empty list.
};
#pragma endregion

// particleMass ----------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* particleMass */
/**
 * @brief Return the configured mass convention for one supported output species.
 *
 * @param pid PDG code for electron, proton, neutron, charged/neutral pion, or photon.
 * @param legacy Select the archived rounded compatibility table when true; false selects the current
 *               PDG table. Every value comes from constants.h.
 *
 * @return Particle mass in GeV/c².
 *
 * @throws std::runtime_error If pid is not part of the supported LUND particle contract.
 *
 * @note This lookup does not validate event-generator status or particle selection. Adapters decide
 *       which truth particles are retained before requesting a mass.
 */
double particleMass(int pid);
#pragma endregion

#pragma endregion

}  // namespace samples
