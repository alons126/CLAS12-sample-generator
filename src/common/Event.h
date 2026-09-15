//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file Event.h
 * @brief Shared event and particle records.
 *
 * Purpose:
 *   Represent PDG identity, mass and momentum in GeV, vertices in cm, and LUND header metadata.
 *
 * Workflow:
 *   Generators populate Event values; monitoring reads them; LundWriter serializes them.
 */

#pragma once
#include <TVector3.h>

#include <cstdint>
#include <vector>

namespace samples {

// Public interface -------------------------------------------------------------
#pragma region /* Public interface */

// Particle object ------------------------------------------------
#pragma region /* Particle object */
/**
 * @struct Particle
 * @brief One output particle, owned by its event.
 *
 * Purpose: carry one generated or converted particle without coupling it to ROOT file ownership.
 * Workflow: a generator populates the record; Event owns it; the writer and diagnostics read it.
 *
 * Units: mass and Cartesian momentum are in GeV; vertex coordinates are in cm.
 * The writer derives mass-shell energy rather than storing an independent energy.
 */
struct Particle {
    int pid;            ///< PDG code used for output identity and diagnostic grouping.
    double mass;        ///< Selected mass convention in GeV; assigned before serialization.
    TVector3 momentum;  ///< Cartesian momentum in GeV; energy is derived from mass and momentum.
    TVector3 vertex;    ///< Interaction position in cm; shared by particles from the same event.
};
#pragma endregion

// Event object ------------------------------------------------
#pragma region /* Event object */
/**
 * @struct Event
 * @brief Shared record passed from generation to output and diagnostics.
 *
 * Purpose: provide one format-independent event contract to both output workflows.
 * Workflow: construct per event, populate metadata/particles, serialize, then fill diagnostics.
 * Ownership: the particle vector owns its records; consumers borrow the event during a call.
 *
 * Metadata: A/Z are explicit nuclear header values, independent of vertex geometry.
 * GENIE uses weight for its historical process tag, not a cross-section weight.
 * The legacy uniform writer may serialize a per-file ID instead of this run ID.
 */
struct Event {
    std::uint64_t id = 0;  ///< Run event index for uniform; scanned-entry index for GENIE.
    int A = 1, Z = 1;      ///< Explicit target mass/charge metadata; defaults preserve historical values.
    // Beam energy is in GeV; resonance_id and weight are historical header metadata.
    // Uniform weight defaults to 1; GENIE replaces it with a selected process tag.
    double beam_energy = 0, resonance_id = 0, weight = 1;
    std::vector<Particle> particles;  ///< Output order is preserved; the writer rejects an empty list.
};
#pragma endregion

/** @brief Return a supported PDG mass in GeV; legacy selects historical pion constants. */
double particleMass(int pid, bool legacy = true);
#pragma endregion
}  // namespace samples
