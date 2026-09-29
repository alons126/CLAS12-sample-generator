//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file GenieConverterGST.h
 * @brief Declares the reader that copies GENIE GST events into LUND files.
 *
 * Purpose:
 *   A GST file stores a table in which each row describes one GENIE event before detector simulation.
 *   This reader copies the parts that LUND needs. It does not run GENIE or calculate new particle momenta.
 *
 * Workflow:
 *   convertPhysical selects `genie-gst` -> convertGenieGST checks the GST data -> supported events are
 *   copied into Event objects -> LundWriter writes and splits the LUND files and records the run settings.
 *
 * Inputs:
 *   RunConfig gives the input files, output location, event limits, beam energy, target shape, target A
 *   and Z values, and the random seed used to choose event positions inside the target.
 *
 * Outputs:
 *   The physical LUND converter writes split LUND text files and a JSON run log. It does not run GENIE,
 *   simulate the detector, or create ROOT monitoring plots.
 *
 * Ownership and lifetime:
 *   The caller owns RunConfig. convertGenieGST() reads it during the call and returns after output is closed
 *   or an error stops conversion.
 *
 * Rules:
 *   For example, if a GST event contains an electron, proton, and photon, those particles keep their GST
 *   momenta. All three receive one shared, randomly chosen position inside the configured target.
 *   Only QE, MEC, RES, and DIS interactions are written. These names mean quasi-elastic,
 *   meson-exchange current, resonance production, and deep-inelastic scattering.
 *
 * Failure:
 *   The function throws an exception when settings or GST data are invalid, no supported event can be
 *   written, the output directory is unsafe, or an output file cannot be written.
 */

#pragma once

#include "core/config/RunConfig.h"

namespace samples {

// Public interface ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Public interface */

/**
 * @brief Copy supported events from existing GENIE GST files into LUND files.
 *
 * Purpose:
 *   Keep the details of reading ROOT GST files in one place. The rest of the LUND code receives ordinary
 *   Event objects and does not need to know how GST branches are stored.
 *
 * Workflow:
 *   Check the settings and required GST fields, read input entries in order, and keep only QE, MEC, RES,
 *   and DIS events. Copy the scattered electron first, then supported final-state particles in their GST
 *   order. Stop at the requested event count, the end of the input, or the input-tail rule described
 *   below. Finally, close the files and write the run log.
 *
 * Inputs:
 *   config contains the final input, target, event-limit, output, and naming settings. The caller owns it;
 *   this function only reads it while the call is running.
 *
 * Outputs:
 *   Success leaves split LUND files and `lund-creation-log.json` in the run directory. Progress shows both
 *   scanned GST entries and written LUND events because unsupported entries are scanned but not written.
 *
 * Input rules:
 *   `nf` says how many final-state particles the current GST entry contains. The particle-ID and momentum
 *   arrays must all contain exactly that many values. A PDG ID is the standard integer used to name a
 *   particle type. The reader copies protons, neutrons, charged pions, and photons. It skips other IDs.
 *   In particular, a remaining neutral pion (PDG 111) is skipped; its decay photons must already be in
 *   the GST input because this function does not invent a decay.
 *
 * Input-tail rule:
 *   Suppose `events-per-file` is 10,000. Before opening a second or later LUND file, the function checks
 *   whether at least 10,000 GST input entries remain, including the current entry. If fewer remain, it
 *   stops successfully. The check counts input entries, not accepted events, so skipped interactions can
 *   make the last written file shorter than 10,000 events.
 *
 * Failure:
 *   Throws on invalid settings, empty or incompatible input, mismatched array lengths, an input with no
 *   supported interactions, failure to choose an event position, or output failure. A failed run does not
 *   write a completion log, although partial files may remain for inspection.
 *
 * @param config Final input, output, target, event-limit, and naming settings read during this call.
 */
void convertGenieGST(const RunConfig& config);
#pragma endregion

}  // namespace samples
