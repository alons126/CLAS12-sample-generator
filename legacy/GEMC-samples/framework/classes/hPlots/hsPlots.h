//
// Created by Alon Sportes on 11/04/2025.
//

#ifndef HSPLOTS_H
#define HSPLOTS_H

#include <TCanvas.h>
#include <TClass.h>
#include <TH1.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TPaveText.h>
#include <TString.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace bt = basic_tools;

/**
 * @class hsPlots
 * @brief Manages collections of sliced ROOT histograms (TH1D or TH2D) defined over user-provided variable ranges.
 *
 * The hsPlots class automates the creation, organization, filling, and saving of histograms grouped by slices of an input variable.
 * It is particularly useful for studies where a variable (e.g. truth momentum) is divided into multiple slices, and distributions are
 * analyzed within each range.
 *
 * Ownership model:
 * - Histograms are created dynamically and stored in an external vector managed by the user.
 * - hsPlots holds a pointer to that vector but does not delete the histograms on destruction.
 * - This allows safe integration with frameworks that handle object ownership elsewhere.
 *
 * Example usage:
 * @code
 * std::vector<std::vector<double>> slices = {{0, 1}, {1, 2}, {2, 3}};
 * std::vector<TObject*> HistoList;
 * std::vector<TObject*> SlicedHistoList;
 *
 * hsPlots plots(slices, hsPlots::TH1D_TYPE, HistoList, &SlicedHistoList,
 *              "momentum_slices", "Momentum resolution;#Delta p/p;Counts", 50, -1.0, 1.0, 0, 0, 0, "p [GeV/c]");
 *
 * plots.Fill(1.5, 0.2); // Fill the slice corresponding to 1.5 GeV/c
 * plots.SaveHistograms("output", "momentum_resolutions");
 * @endcode
 */
class hsPlots {
   public:
    /**
     * @enum HistoType
     * @brief Defines the type of histogram each slice holds.
     */
    enum HistoType {
        TH1D_TYPE,  ///< One-dimensional histogram (TH1D)
        TH2D_TYPE   ///< Two-dimensional histogram (TH2D)
    };

    // hsPlots constructor ----------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * @brief Construct sliced histograms according to given slice limits.
     *
     * @param sliceLimits           Vector of slice ranges, each inner vector has two elements {lower, upper}.
     * @param type                  Histogram type: TH1D_TYPE or TH2D_TYPE.
     * @param HistoList             External sink vector to which created histograms are appended (no ownership transfer).
     * @param SlicedHistoListPtr    Pointer to internal vector where per-slice histograms are stored.
     * @param baseName              Base name used for histograms (each slice appends range to this base).
     * @param titleTemplate         Title format: "Main title;X axis;Y axis".
     * @param nbinsX, xlow, xup     X-axis binning parameters.
     * @param nbinsY, ylow, yup     Y-axis binning parameters (used only for TH2D_TYPE).
     * @param slice_var             Name and optional units of the slice variable.
     */
    hsPlots(const std::vector<std::vector<double>>& sliceLimits, HistoType type, std::vector<TObject*>& HistoList, std::vector<TObject*>* SlicedHistoListPtr, const std::string& baseName,
            const std::string& titleTemplate, const int& nbinsX, const double& xlow, const double& xup, const int& nbinsY, const double& ylow, const double& yup, std::string slice_var);

    // hsPlots Destructor -----------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * @brief Destructor for hsPlots.
     *
     * Histograms are not deleted automatically to prevent double ownership. Uncomment deletion logic
     * in the source file only if hsPlots exclusively owns the histograms.
     */
    ~hsPlots();

    // Fill function ----------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * @brief Fill the appropriate per-slice histogram with the given values.
     *
     * The slice is determined by comparing `sliceVar` to `SliceLimits`. Depending on the histogram type,
     * the fill call uses either `TH1D::Fill(x, weight)` or `TH2D::Fill(x, y, weight)`.
     *
     * @param sliceVar  Value used to determine which slice to fill.
     * @param x         X value (used in both TH1D and TH2D).
     * @param y         Y value (used only in TH2D, ignored otherwise).
     * @param weight    Weight to apply when filling.
     */
    void Fill(double sliceVar, double x, double y = 0, double weight = 1.0);

    // GetSlicedHistoList function --------------------------------------------------------------------------------------------------------------------------------------
    /**
     * @brief Return a copy of the vector containing pointers to per-slice histograms.
     *
     * Ownership is not transferred; the caller receives a shallow copy of pointers.
     * @return Vector of TObject* for each slice histogram.
     */
    std::vector<TObject*> GetSlicedHistoList() const;

    // DrawEmptyHistogramNotice function --------------------------------------------------------------------------------------------------------------------------------
    /**
     * @brief Draw a centered "Empty histogram" notice inside the current pad.
     *
     * Utility overlay to clearly indicate that a slice produced no entries. Draws an NDC box
     * with configurable bounds and text size.
     *
     * @param x_1,y_1   Lower-left NDC coordinates of the notice box.
     * @param x_2,y_2   Upper-right NDC coordinates of the notice box.
     * @param diplayTextSize  Text size for the message (default 0.1).
     */
    static void DrawEmptyHistogramNotice(double x_1, double y_1, double x_2, double y_2, double diplayTextSize) {
        TPaveText* displayText = new TPaveText(x_1, y_1, x_2, y_2, "NDC");  // Create an NDC-aligned box
        displayText->SetTextSize(diplayTextSize);                           // Set font size
        displayText->SetFillColor(0);                                       // Transparent background
        displayText->AddText("Empty histogram");                            // Message text
        displayText->SetTextAlign(22);                                      // Center both horizontally and vertically
        displayText->SetBit(kCanDelete);                                    // Let pad/canvas own and delete it
        displayText->Draw();                                                // Render on current pad
    }

    // SaveHistograms function ------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * @brief Save all histograms (1D or 2D) into a multi-page PDF and individual per-slice PDFs.
     *
     * Each histogram is drawn on a dedicated TCanvas page and appended to a PDF file.
     * If a histogram is empty, a visual "Empty histogram" notice is overlaid.
     *
     * @param outputDir      Target output directory.
     * @param baseFileName   Base name (without extension) for the saved files.
     */
    void SaveHistograms(const std::string& outputDir, const std::string& baseFileName) const;

   private:
    /**
     * @brief If true, any value outside the slice limits triggers a fatal error.
     * If false, out-of-range values are silently skipped.
     */
    bool ForceFullVarRange = false;

    /**
     * @brief Defines lower and upper limits for each slice.
     * Each inner vector must contain exactly two elements: {lower_limit, upper_limit}.
     */
    std::vector<std::vector<double>> SliceLimits;

    /**
     * @brief Pointer to the external vector that stores histogram pointers.
     * Histograms correspond one-to-one with entries in `SliceLimits`.
     * The external owner (caller) manages lifetime and deallocation.
     */
    std::vector<TObject*>* SlicedHistoListPtr;

    /**
     * @brief Type of histograms stored in slices (TH1D or TH2D).
     * Used to determine which ROOT histogram class to instantiate.
     */
    HistoType histoType;

    // FindSliceIndex function ------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * @brief Find the index of the slice corresponding to a given value.
     * @param value  The slicing variable value to locate.
     * @return Index of the matching slice, or -1 if none found.
     */
    int FindSliceIndex(double value);
};

#endif  // HSPLOTS_H
