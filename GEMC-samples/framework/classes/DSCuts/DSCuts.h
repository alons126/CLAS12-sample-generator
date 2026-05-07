//
// Created by Alon Sportes on 06/03/2023.
//

#ifndef DSCUTS_H
#define DSCUTS_H

#include <iostream>
#include <string>
#include <vector>

/**
 * @class DSCuts
 *
 * Lightweight container for fit-derived cut parameters and related metadata.
 *
 * Purpose:
 *   Store and manage all information required to define a single analysis cut:
 *     - cut variable name (e.g. momentum, beta, resolution, etc.)
 *     - detector region label (CD / FD / combined)
 *     - particle label and corresponding PDG code
 *     - applied-cut description string
 *     - mean value and lower/upper cut boundaries
 *     - fit-related quantities (fit mean, sigma factor, sigma value)
 *     - optional slice information (slice index and bounds)
 *
 * Notes:
 *   - `Cuts` is a 3-element vector: {mean, lower cut, upper cut}.
 *   - If the region argument `r` is empty, `Region` is set to "CD & FD".
 *   - `FitStdFactor` is determined from the *input* region argument `r` (legacy behavior).
 *   - `SetMeanFit()` currently writes to `MeanFromHistogram` to preserve existing behavior.
 *   - This class acts purely as a data/configuration container and performs no physics calculations.
 */
class DSCuts {
   public:
    // Default constructor
    DSCuts(std::string cv = "", std::string r = "", std::string p = "", std::string ac = "", double mean = 0, double llim = -9999, double ulim = 9999)
        : MeanFromHistogram(0.0),
          MeanFromFit(0.0),
          FitStdFactor(1.0),
          FitStd(0.0),
          Cuts({0.0, -9999.0, 9999.0}),
          CutVariable(""),
          Region(""),
          Particle(""),
          AppliedCuts(""),
          particlePDG(0),
          SliceNumber(0),
          SliceUpperb(0.0),
          SliceLowerb(0.0) {
        // 1) Store metadata strings.
        CutVariable = cv;
        Particle = p;
        AppliedCuts = ac;

        // 2) Store the cut parameters: {mean, lower, upper}.
        Cuts.at(0) = mean;
        Cuts.at(1) = llim;
        Cuts.at(2) = ulim;

        // 3) Region label: empty input means a combined region label.
        if (r == "") {
            Region = "CD & FD";
        } else {
            Region = r;
        }

        // 4) Choose sigma factor for fit-based cuts.
        //    Legacy behavior: decisions are based on the *input* argument `r`.
        if (r == "CD") {
            FitStdFactor = 3;  // sigma factor for CD cuts
        } else if (r == "FD") {
            FitStdFactor = 3;  // sigma factor for FD cuts
        } else {
            FitStdFactor = 1;
        }

        // 5) Map particle name strings to PDG codes.
        //    If none of the supported labels match, `particlePDG` remains 0.
        if (p == "electron" || p == "Electron" || p == "electrons" || p == "Electrons") {
            particlePDG = 11;
        } else if (p == "neutron" || p == "Neutron" || p == "neutrons" || p == "Neutrons") {
            particlePDG = 2112;
        } else if (p == "proton" || p == "Proton" || p == "protons" || p == "Protons") {
            particlePDG = 2212;
        } else if (p == "kplus" || p == "Kplus" || p == "k+" || p == "K+") {
            particlePDG = 321;
        } else if (p == "kminus" || p == "Kminus" || p == "k-" || p == "K-") {
            particlePDG = -321;
        } else if (p == "piplus" || p == "Piplus" || p == "pi+" || p == "Pi+") {
            particlePDG = 211;
        } else if (p == "piminus" || p == "Piminus" || p == "pi-" || p == "Pi-") {
            particlePDG = -211;
        } else if (p == "pizero" || p == "Pizero" || p == "pi0" || p == "Pi0") {
            particlePDG = 111;
        }
    }

    // Setters: update cached fit/histogram means and cut bounds.
    /* Set functions */
    void SetMeanHist(double mh) { MeanFromHistogram = mh; }

    // Legacy behavior: this writes to MeanFromHistogram (kept to avoid changing current behavior).
    void SetMeanFit(double mf) { MeanFromHistogram = mf; }

    void SetStdFactor(double sf) { FitStdFactor = sf; }

    void SetCutPram(double mean, double lcut, double ucut) { Cuts.at(0) = mean, Cuts.at(1) = lcut, Cuts.at(2) = ucut; }

    void SetMean(double mean) { Cuts.at(0) = mean; }

    void SetLowerCut(double lcut) { Cuts.at(1) = lcut; }

    void SetUpperCut(double ucut) { Cuts.at(2) = ucut; }

    void SetCutVariable(std::string cv) { CutVariable = cv; }

    void SetRegion(std::string r) { Region = r; }

    void SetPart(std::string p) { Particle = p; }

    void SetAppliedCuts(std::string ac) { AppliedCuts = ac; }

    void SetSliceNumber(int sNumber) { SliceNumber = sNumber; }

    void SetSliceUpperb(double sUpperb) { SliceUpperb = sUpperb; }

    void SetSliceLowerb(double sLowerb) { SliceLowerb = sLowerb; }

    // Getters: access cut parameters and bookkeeping metadata.
    /* Get functions */
    double GetMeanHist() const { return MeanFromHistogram; }

    double GetMeanFit() const { return MeanFromFit; }

    double GetStdFactor() const { return FitStdFactor; }

    double GetMean() const { return Cuts.at(0); }

    double GetMeanConst() const { return Cuts.at(0); }

    double GetLowerCut() const { return Cuts.at(1); }

    double GetLowerCutConst() const { return Cuts.at(1); }

    double GetUpperCut() const { return Cuts.at(2); }

    double GetUpperCutConst() const { return Cuts.at(2); }

    std::string GetCutVariable() const { return CutVariable; }

    std::string GetRegion() const { return Region; }

    std::string GetPart() const { return Particle; }

    int GetPartPDG() const { return particlePDG; }

    std::string GetAppliedCuts() const { return AppliedCuts; }

    int GetSliceNumber() const { return SliceNumber; }

    double GetSliceUpperb() const { return SliceUpperb; }

    double GetSliceLowerb() const { return SliceLowerb; }

    // Public fit/cut state (kept public for backward compatibility with existing analysis code).
    double MeanFromHistogram;
    double MeanFromFit;
    double FitStdFactor;
    double FitStd;

    // Cut triplet: {mean, lower cut, upper cut}.
    std::vector<double> Cuts = {0.0, -9999.0, 9999.0};

   private:
    std::string CutVariable, Region, Particle, AppliedCuts;

    int particlePDG, SliceNumber;

    double SliceUpperb, SliceLowerb;  // To be used in neutron resolution fit only!
};

#endif  // DSCUTS_H
