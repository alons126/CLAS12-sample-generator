//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file GenieConverterGST.h
 * @brief Declares how GENIE GST events are copied to LUND.
 *
 * Purpose:
 *   Read existing GENIE GST truth into the common Event type. Shared code samples target vertices,
 *   writes and splits LUND files, and records the run settings.
 *
 * Workflow:
 *   convertPhysical selects `genie-gst` -> convertGenieGST validates and scans the GST chain -> accepted
 *   QE/MEC/RES/DIS entries become LUND events -> LundWriter publishes the completed run manifest.
 *
 * Inputs:
 *   RunConfig gives the GST input, target geometry, separate A and Z values, beam energy, vertex seed,
 *   event limits, output location, and sample metadata.
 *
 * Outputs:
 *   Split LUND text files and a completion log are written under the resolved run directory. No
 *   event-generator execution, detector simulation or physical-conversion monitoring file is produced.
 *
 * Ownership and lifetime:
 *   The caller owns RunConfig. convertGenieGST() reads it during the call and returns after output is closed
 *   or an error stops conversion.
 *
 * Rules:
 *   GST particle momenta and ordering are preserved for supported identities; one sampled vertex is
 *   shared by all particles in a written event. Only QE, MEC, RES and DIS reactions are supported;
 *   another reaction requires a converter update.
 *
 * Failure:
 *   Invalid settings, unusable GST branches or data, no supported interactions, unsafe output
 *   preparation and file-writing failures are reported by exception.
 */

#pragma once
#include "core/config/RunConfig.h"

namespace samples {

// Public interface ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Public interface */

/**
 * @brief Convert supported events from existing GENIE GST input to LUND.
 *
 * Purpose:
 *   Keep ROOT and GST reading details inside this converter. The caller and shared LUND code work only
 *   with RunConfig and Event values.
 *
 * Workflow:
 *   Revalidate the resolved settings, validate required GST branches, traverse entries in chain order,
 *   keep QE/MEC/RES/DIS interactions, copy the scattered electron and supported final particles, apply
 *   the submission-block cutoff only before a later file starts, stop at capacity or
 *   input exhaustion, then finalize the run.
 *
 * Inputs:
 *   config belongs to the caller and already contains final physical-run values. The function checks it
 *   again and does not store it after returning.
 *
 * Outputs:
 *   No C++ value is returned. Success leaves split LUND files and lund-gen-log.json in the output
 *   directory and prints the scanned and written event counts.
 *
 * Expected input:
 *   Final-state PDG and momentum arrays have the same variable length stored in nf.
 *   Reaction selection supports only QE, MEC, RES and DIS; adding another process requires changing
 *   this converter.
 *
 * Failure:
 *   Throws on invalid configuration, empty or incompatible input, inconsistent final-state array lengths,
 *   an input with no supported interactions, target-sampling failure or output failure. A failed run does
 *   not publish a completion manifest, although partial diagnostic output may remain for inspection.
 *
 * @param config Final input, output, target, event-limit, and metadata settings read during this call.
 */
void convertGenieGST(const RunConfig& config);
#pragma endregion

}  // namespace samples
