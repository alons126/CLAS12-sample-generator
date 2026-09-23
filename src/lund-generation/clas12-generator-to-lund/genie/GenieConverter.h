//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file GenieConverter.h
 * @brief GENIE GST conversion entry-point contract.
 *
 * Purpose:
 *   Declare the GENIE-specific input adapter behind the generator-independent physical-conversion
 *   boundary. The adapter translates existing GST truth into the common in-memory Event model; shared
 *   components remain responsible for target sampling, LUND serialization, splitting and provenance.
 *
 * Workflow:
 *   convertPhysical selects GENIE -> convertGenie validates and scans the GST chain -> accepted
 *   QE/MEC/RES/DIS entries become LUND events -> LundWriter publishes the completed run manifest.
 *
 * Inputs:
 *   A resolved RunConfig identifies the GST input, target geometry, independent A/Z metadata, beam
 *   energy, vertex seed, event limits, output location and physical-sample provenance.
 *
 * Outputs:
 *   Split LUND text files and a completion manifest are written under the resolved run directory. No
 *   event-generator execution, detector simulation or physical-conversion monitoring file is produced.
 *
 * Ownership and lifetime:
 *   The caller owns RunConfig. convertGenie borrows it only for the duration of the synchronous call and
 *   returns after all output streams are finalized or an exception has interrupted conversion.
 *
 * Invariants:
 *   GST particle momenta and ordering are preserved for supported identities; one sampled vertex is
 *   shared by all particles in a written event. Neutral-pion decay photons must already exist upstream.
 *   Only QE, MEC, RES and DIS reactions are supported; another reaction requires an adapter update.
 *
 * Failure:
 *   Invalid configuration, unusable GST schema or data, absence of supported interactions, unsafe output
 *   preparation and filesystem/serialization failures are reported by exception.
 */

#pragma once
#include "core/config/RunConfig.h"

namespace samples {

// Public interface ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Public interface */

/**
 * @brief Convert supported events from an existing GENIE GST input into LUND.
 *
 * Purpose:
 *   Provide the complete GENIE implementation of the physical-input adapter without exposing ROOT types
 *   or GENIE-specific records to the dispatcher or common LUND layer.
 *
 * Workflow:
 *   Revalidate the resolved settings, validate required GST branches, traverse entries in chain order,
 *   retain QE/MEC/RES/DIS interactions, copy the scattered electron and supported detector-stable final
 *   state, apply the submission-block cutoff only before a follow-up file starts, stop at capacity or
 *   input exhaustion, then finalize the run.
 *
 * Inputs:
 *   config is an immutable, caller-owned RunConfig whose physical fields have already been resolved. The
 *   function validates it again at its public boundary and does not retain a reference after returning.
 *
 * Outputs:
 *   No C++ value is returned. Successful completion leaves split LUND files and lund-gen-log.json in the
 *   resolved output tree; scanned and written counts are also reported to standard output.
 *
 * Assumptions:
 *   Final-state PDG and momentum arrays are parallel variable-length GST branches counted by nf. PDG 111
 *   is not serialized because neutral pions must be decayed into photons before GST production. Reaction
 *   selection supports only QE, MEC, RES and DIS; adding another process requires changing this adapter.
 *
 * Failure:
 *   Throws on invalid configuration, empty or incompatible input, inconsistent final-state array lengths,
 *   an input with no supported interactions, target-sampling failure or output failure. A failed run does
 *   not publish a completion manifest, although partial diagnostic output may remain for inspection.
 *
 * @param config Resolved input, output, target, event-limit and provenance settings; borrowed for this
 *               call and never modified.
 */
void convertGenie(const RunConfig& config);
#pragma endregion

}  // namespace samples
