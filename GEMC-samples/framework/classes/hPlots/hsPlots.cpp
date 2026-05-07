//
// Created by Alon Sportes on 11/04/2025.
//

#ifndef HSPLOTS_CPP
#define HSPLOTS_CPP

#include "hsPlots.h"

// hsPlots manages a list of sliced histograms (TH1D or TH2D) created from user-defined ranges.
// Histogram objects are stored externally and passed via pointer, enabling ownership control outside the class.

/*
Example usage:

std::vector<std::vector<double>> slices = {{0, 1}, {1, 2}, {2, 3}};
std::vector<TObject*> HistoList;
std::vector<TH1*> SlicedHistoList;

hsPlots myPlots(slices, hsPlots::TH2D_TYPE, HistoList, &SlicedHistoList,
                "theta_phi", "Theta vs Phi;#phi [deg];#theta [deg]",
                100, -180, 180, 100, 0, 180, "theta");

// Fill with: theta slice variable, phi, theta
myPlots.Fill(1.5, 45, 90);

// Save plots to disk
myPlots.SaveHistograms("output_dir", "hist_output");
*/

// hsPlots constructor --------------------------------------------------------------------------------------------------------------------------------------------------

// Constructor to initialize the hsPlots object
// This constructor takes a vector of slice limits, a histogram type (TH1D or TH2D), a base name for the histograms,
// a title template for the histograms, and the number of bins and limits for the x and y axes.
// It creates the histograms based on the provided slice limits and adds them to the SlicedHistoList.
// The histograms are created using the ROOT library's TH1D and TH2D classes.
// The constructor also takes a vector of TObject pointers (HistoList) to store the created histograms.
// The histograms are named using the base name and the slice index, and the titles are generated using the title template and the slice limits.
// @param SlicedHistoListPtr Pointer to a vector where the histograms will be stored.
//        The caller is responsible for managing the lifetime of the vector and the histograms inside it.
/**
 * @brief Construct sliced histograms (TH1D or TH2D) according to user-provided slice limits.
 *
 * This constructor initializes an `hsPlots` instance that manages a vector of histograms, one per slice.
 * The constructor **does not** modify the provided slice limits. In particular, it does **not** append an extra
 * overflow slice. If you want an open-ended overflow region, include it explicitly in `sliceLimits`.
 *
 * @param sliceLimits            Vector of {low, high} ranges defining each slice. Must be non-empty.
 * @param type                   Histogram type to create for each slice: TH1D_TYPE or TH2D_TYPE.
 * @param HistoList              External sink vector to which created histograms are appended (no ownership transfer).
 * @param SlicedHistoListPtr     Pointer to the internal vector where histograms for each slice are stored.
 * @param baseName               Base name used to derive unique histogram names per slice.
 * @param titleTemplate          Title format: "MainTitle;XLabel;YLabel". YLabel ignored beyond TH2D.
 * @param nbinsX, xlow, xup      X axis binning and range.
 * @param nbinsY, ylow, yup      Y axis binning and range (used only for TH2D_TYPE).
 * @param slice_var              Slice variable (with optional units), e.g. "P^{truth}_{nFD} [GeV/c]".
 */
hsPlots::hsPlots(const std::vector<std::vector<double>>& sliceLimits,       // Input slice ranges {lo, hi}
                 HistoType type,                                            // Target histogram type
                 std::vector<TObject*>& HistoList,                          // External list to receive created histos
                 std::vector<TObject*>* SlicedHistoListPtr,                 // Internal storage (owned externally)
                 const std::string& baseName,                               // Base histogram name prefix
                 const std::string& titleTemplate,                          // "Main;X;Y" format
                 const int& nbinsX, const double& xlow, const double& xup,  // X-axis binning
                 const int& nbinsY, const double& ylow, const double& yup,  // Y-axis binning (TH2D only)
                 std::string slice_var                                      // Slice variable with units
                 )
    : SliceLimits(sliceLimits),               // Copy initial slice limits
      histoType(type),                        // Save chosen histogram type
      SlicedHistoListPtr(SlicedHistoListPtr)  // Store pointer to vector of slice histos
{
    bool PrintOut = false;  // Local verbose flag

    // Validate slice limits: each must be {low, high} with low < high, and lows must be non-decreasing.
    if (SliceLimits.empty()) {
        std::cerr << "hsPlots::hsPlots: ERROR: SliceLimits is empty.\n";
        exit(1);
    }

    for (size_t i = 0; i < SliceLimits.size(); ++i) {
        if (SliceLimits[i].size() != 2) {
            std::cerr << "hsPlots::hsPlots: ERROR: SliceLimits[" << i << "] does not have 2 elements.\n";
            exit(1);
        }

        if (!(SliceLimits[i][0] < SliceLimits[i][1])) {
            std::cerr << "hsPlots::hsPlots: ERROR: SliceLimits[" << i << "] has non-increasing bounds: {" << SliceLimits[i][0] << ", " << SliceLimits[i][1] << "}.\n";
            exit(1);
        }

        if (i > 0 && !(SliceLimits[i - 1][0] <= SliceLimits[i][0])) {
            std::cerr << "hsPlots::hsPlots: ERROR: Slice lower bounds are not non-decreasing at index " << i << ".\n";
            exit(1);
        }
    }

    // Split slice_var into the variable label and units, e.g. ("P^{truth}_{nFD}", "[GeV/c]")
    auto slice_var_pair = bt::splitVarAndUnits(slice_var);  // .first = var, .second = units

    int count = 0;  // Slice counter (for error context)

    // Optional constructor debug prints
    if (PrintOut) {
        std::cout << "\nhsPlots constructor PrintOuts:" << std::endl;
        std::cout << "hsPlots::hsPlots: SliceLimits.size() = " << SliceLimits.size() << std::endl;
        std::cout << "hsPlots::hsPlots: histoType = " << ((histoType == TH1D_TYPE) ? "TH1D_TYPE" : (histoType == TH2D_TYPE) ? "TH2D_TYPE" : "UNKNOWN") << std::endl;
        std::cout << "hsPlots::hsPlots: baseName = " << baseName << std::endl;
        std::cout << "hsPlots::hsPlots: titleTemplate = " << titleTemplate << std::endl;
        std::cout << "hsPlots::hsPlots: slice_var_pair.first = " << slice_var_pair.first << std::endl;
        std::cout << "hsPlots::hsPlots: slice_var_pair.second = " << slice_var_pair.second << std::endl;
        std::cout << std::endl;
    }

    // Pre-scan the title template to extract main title and axis labels
    size_t firstSep, secondSep;             // Positions of ';' separators
    std::string mainTitle, xLabel, yLabel;  // Parsed pieces

    // --- Create one histogram per slice --------------------------------------------------------
    for (const auto& range : SliceLimits) {
        // Validate {lo, hi}
        if (range.size() != 2) {
            std::cerr << "Invalid slice range at index " << count << std::endl;
            continue;  // Skip malformed range
        }

        std::ostringstream name, title;  // Builders for unique name and decorated title

        // Histogram name encodes slice range for traceability
        name << baseName << "_slice_from_" << bt::ToStringWithPrecision(range.at(0), 2) << "_to_" << bt::ToStringWithPrecision(range.at(1), (range.at(1) == 9999) ? 0 : 2);

        // Parse title template of the form: "Main;X;Y"
        firstSep = titleTemplate.find(';');
        secondSep = titleTemplate.find(';', firstSep + 1);
        size_t thirdSep = titleTemplate.find(';', secondSep + 1);  // Any extra fields

        bool hasSplit = (firstSep != std::string::npos && secondSep != std::string::npos);

        if (hasSplit) {
            // Extract substrings for main and axis labels
            mainTitle = titleTemplate.substr(0, firstSep);
            xLabel = titleTemplate.substr(firstSep + 1, secondSep - firstSep - 1);
            yLabel = titleTemplate.substr(secondSep + 1);

            // For TH2D, ignore any extra fields beyond Y label
            if (histoType == TH2D_TYPE && thirdSep != std::string::npos) {
                yLabel = titleTemplate.substr(secondSep + 1, thirdSep - secondSep - 1);
                std::cerr << "\033[33m[hsPlots WARNING]\033[0m Ignoring extra ';' field in titleTemplate for TH2D histogram: \"" << titleTemplate << "\"\n";
            }

            if (PrintOut) {
                std::cout << "\nhsPlots constructor PrintOuts:" << std::endl;
                std::cout << "hsPlots::hsPlots: mainTitle = " << mainTitle << std::endl;
                std::cout << "hsPlots::hsPlots: xLabel = " << xLabel << std::endl;
                std::cout << "hsPlots::hsPlots: yLabel = " << yLabel << std::endl;
                std::cout << std::endl;
            }
        } else {
            // If the template has no ';', treat the entire string as the main title
            std::cout << std::endl;
            std::cerr << "\033[33m[hsPlots WARNING]\033[0m No splits have been found in titleTemplate. Using entire string as mainTitle.\n";
            mainTitle = titleTemplate;
        }

        // Basic validation of required labels
        if (mainTitle.empty()) {
            std::cerr << "hsPlots::hsPlots: ERROR: mainTitle is empty. Please provide a valid titleTemplate.\n";
            exit(1);
        }

        if (xLabel.empty()) {
            std::cerr << "hsPlots::hsPlots: ERROR: xLabel is empty. Please provide a valid titleTemplate.\n";
            exit(1);
        } else {
            size_t xLabel_firstSep = xLabel.find(';');  // Reject nested separators in xLabel
            bool xLabel_hasSplit = (xLabel_firstSep != std::string::npos);

            if (xLabel_hasSplit) {
                std::cerr << "\033[33m[hsPlots WARNING]\033[0m Ignoring extra ';' field in xLabel for histogram: \"" << xLabel << "\"\n";
                exit(1);
            }
        }

        if (yLabel.empty()) {
            std::cerr << "hsPlots::hsPlots: ERROR: yLabel is empty. Please provide a valid titleTemplate.\n";
            exit(1);
        } else {
            size_t yLabel_firstSep = yLabel.find(';');  // Reject nested separators in yLabel
            bool yLabel_hasSplit = (yLabel_firstSep != std::string::npos);

            if (yLabel_hasSplit) {
                std::cerr << "\033[33m[hsPlots WARNING]\033[0m Ignoring extra ';' field in yLabel for histogram: \"" << yLabel << "\"\n";
                exit(1);
            }
        }

        // Compose a decorated title that embeds the slice range and the slice variable with units
        title << "#splitline{ " << mainTitle << " }{ Slice limits: " << bt::ToStringWithPrecision(range.at(0), 2) << " #leq " << slice_var_pair.first << " #leq "
              << bt::ToStringWithPrecision(range.at(1), (range.at(1) == 9999) ? 0 : 2) << " " << slice_var_pair.second << " }";

        // Instantiate the histogram for this slice based on requested type
        if (histoType == TH1D_TYPE) {
            TH1D* hist = new TH1D(name.str().c_str(), title.str().c_str(), nbinsX, xlow, xup);  // 1D hist

            if (hasSplit) {  // Apply parsed axis titles when available
                hist->GetXaxis()->SetTitle(xLabel.c_str());
                hist->GetYaxis()->SetTitle(yLabel.c_str());
            }

            SlicedHistoListPtr->push_back(hist);  // Store in slice list
        } else if (histoType == TH2D_TYPE) {
            TH2D* hist = new TH2D(name.str().c_str(), title.str().c_str(), nbinsX, xlow, xup, nbinsY, ylow, yup);  // 2D hist

            if (hasSplit) {  // Apply parsed axis titles when available
                hist->GetXaxis()->SetTitle(xLabel.c_str());
                hist->GetYaxis()->SetTitle(yLabel.c_str());
            }

            SlicedHistoListPtr->push_back(hist);  // Store in slice list
        }

        ++count;  // Next slice index
    }

    // After creating all per-slice histograms, expose them via the external HistoList sink
    for (int i = 0; i < static_cast<int>(SlicedHistoListPtr->size()); i++) {
        HistoList.push_back((*SlicedHistoListPtr)[i]);  // Append pointer (no ownership transfer)
    }
}

// hsPlots Destructor ---------------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Destructor for hsPlots.
 *
 * Histograms are *not* deleted here because the owning code manages their lifetime externally.
 * Uncomment the loop below only if `hsPlots` owns the histograms and should delete them.
 */
hsPlots::~hsPlots() {
    // for (auto* h : *SlicedHistoListPtr) { delete h; }  // Ownership model-dependent
}

// FindSliceIndex function ----------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Find the index of the slice that contains a given value.
 *
 * Scans `SliceLimits` and returns the first index i such that
 * `SliceLimits[i][0] <= value < SliceLimits[i][1]`. If no slice contains the value,
 * returns -1.
 *
 * @param value   The quantity used for slicing (e.g., truth momentum).
 * @return        Zero-based index of the matching slice, or -1 if not found.
 */
int hsPlots::FindSliceIndex(double value) {
    if (SliceLimits.empty()) return -1;

    // Detect a "semi-infinite upper" scheme like {low_i, 9999} for all i.
    const double commonUpper = SliceLimits[0][1];
    bool allUpperEqual = true;
    for (size_t i = 1; i < SliceLimits.size(); ++i) {
        if (std::fabs(SliceLimits[i][1] - commonUpper) > 1e-9) {
            allUpperEqual = false;
            break;
        }
    }

    if (allUpperEqual) {
        // Choose the rightmost slice whose lower bound is <= value.
        // This makes {0,9999},{10,9999},{20,9999},... behave like consecutive bins of width Delta.
        size_t lo = 0, hi = SliceLimits.size();
        while (lo < hi) {
            size_t mid = lo + (hi - lo) / 2;
            if (value < SliceLimits[mid][0]) {
                hi = mid;
            } else {
                lo = mid + 1;
            }
        }
        int idx = static_cast<int>(lo) - 1;
        // Guard against values below the first lower bound.
        if (idx < 0) return -1;
        return idx;
    }

    // Generic case: pick the first slice with low <= value < high.
    for (size_t i = 0; i < SliceLimits.size(); ++i) {
        if (value >= SliceLimits[i][0] && value < SliceLimits[i][1]) { return static_cast<int>(i); }
    }
    return -1;
}

// Fill function --------------------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Fill the per-slice histogram(s) with the provided coordinates and weight.
 *
 * Fills all overlapping slices when the slice limits use the {low_i, commonUpper} scheme (e.g., {0,9999},{50,9999},...).
 * In standard non-overlapping [low, high) ranges, fills only the matching slice.
 *
 * @param sliceVar  Value used to determine the slice (e.g., P_truth).
 * @param x         X coordinate (for TH1D and TH2D).
 * @param y         Y coordinate (only used for TH2D).
 * @param weight    Event weight.
 */
void hsPlots::Fill(double sliceVar, double x, double y, double weight) {
    bool PrintOut = false;  // Verbose-level toggle for warnings

    if (SlicedHistoListPtr == nullptr) {
        std::cerr << "\033[31m\n\nhsPlots::Fill: ERROR: SlicedHistoListPtr is null!\nAborting...\033[0m\n";
        exit(1);
    }

    if (SliceLimits.empty()) {
        if (ForceFullVarRange) {
            std::cerr << "\033[31m\n\nhsPlots::Fill: ERROR: SliceLimits is empty while ForceFullVarRange is true.\nAborting...\033[0m\n";
            exit(1);
        }

        return;
    }

    // Detect a "semi-infinite upper" scheme like {low_i, 9999} for all i.
    const double commonUpper = SliceLimits[0][1];
    bool allUpperEqual = true;
    for (size_t i = 1; i < SliceLimits.size(); ++i) {
        if (std::fabs(SliceLimits[i][1] - commonUpper) > 1e-9) {
            allUpperEqual = false;
            break;
        }
    }

    // Helper lambda to fill a single histogram at index idx (with type checks and errors).
    auto fill_one = [&](int idx) {
        if (idx < 0 || idx >= static_cast<int>(SlicedHistoListPtr->size())) {
            if (ForceFullVarRange) {
                std::cerr << "\033[31m\n\nhsPlots::Fill: ERROR: Invalid slice index " << idx << " (out of range " << SlicedHistoListPtr->size() << ").\nAborting...\033[0m\n";
                exit(1);
            }

            if (PrintOut) { std::cerr << "hsPlots::Fill: WARNING: Skipping out-of-range slice index " << idx << ".\n"; }

            return;
        }

        TObject* obj = (*SlicedHistoListPtr)[idx];
        if (!obj) {
            std::cerr << "\033[31m\n\nhsPlots::Fill: ERROR: Histogram at index " << idx << " is null!\nAborting...\033[0m\n";
            exit(1);
        }

        if (histoType == TH1D_TYPE) {
            if (auto* h1 = dynamic_cast<TH1D*>(obj)) {
                h1->Fill(x, weight);
            } else {
                std::cerr << "\033[31m\n\nhsPlots::Fill: ERROR: Object at index " << idx << " is not TH1D as expected (actual: " << obj->ClassName() << ").\nAborting...\033[0m\n";
                exit(1);
            }
        } else if (histoType == TH2D_TYPE) {
            if (auto* h2 = dynamic_cast<TH2D*>(obj)) {
                h2->Fill(x, y, weight);
            } else {
                std::cerr << "\033[31m\n\nhsPlots::Fill: ERROR: Object at index " << idx << " is not TH2D as expected (actual: " << obj->ClassName() << ").\nAborting...\033[0m\n";
                exit(1);
            }
        } else {
            std::cerr << "\033[31m\n\nhsPlots::Fill: ERROR: Unknown histoType.\nAborting...\033[0m\n";
            exit(1);
        }
    };

    if (allUpperEqual) {
        // Overlapping slices case: fill ALL slices whose lower bound <= sliceVar.
        // Find the last slice with lower <= value using binary search on lower bounds.
        size_t lo = 0, hi = SliceLimits.size();

        while (lo < hi) {
            size_t mid = lo + (hi - lo) / 2;

            if (sliceVar < SliceLimits[mid][0]) {
                hi = mid;
            } else {
                lo = mid + 1;
            }
        }

        int lastIdx = static_cast<int>(lo) - 1;

        if (lastIdx < 0) {
            // Value is below the first lower bound.
            if (ForceFullVarRange) {
                std::cerr << "\033[31m\n\nhsPlots::Fill: ERROR: Value " << sliceVar << " is below the minimum slice lower bound " << SliceLimits.front()[0]
                          << " while ForceFullVarRange is true.\nAborting...\033[0m\n";
                exit(1);
            }

            if (PrintOut) { std::cerr << "hsPlots::Fill: WARNING: Value " << sliceVar << " is below all slice lower bounds. Skipping fill.\n"; }

            return;
        }

        for (int idx = 0; idx <= lastIdx; ++idx) { fill_one(idx); }

        return;  // Done in overlapping mode
    }

    // Generic non-overlapping case: fill only the first matching [low, high) slice.
    int index = FindSliceIndex(sliceVar);
    const bool invalidIndex = (index < 0) || (index >= static_cast<int>(SlicedHistoListPtr->size()));

    if (ForceFullVarRange && invalidIndex) {
        std::cerr << "\033[31m\n\nhsPlots::Fill: ERROR: Invalid slice index!\n"
                  << "index = " << index << "\n"
                  << "sliceVar = " << sliceVar << "\n"
                  << "Aborting...\033[0m\n";
        exit(1);
    }

    if (invalidIndex) {
        if (PrintOut) { std::cerr << "hsPlots::Fill: WARNING: Value " << sliceVar << " falls outside defined slice ranges. Skipping fill.\n"; }

        return;
    }

    fill_one(index);
}

// SaveHistograms function ----------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Save all per-slice histograms to a multi-page PDF and individual PDF images.
 *
 * For each histogram in `SlicedHistoListPtr`, this function formats axes, optionally overlays
 * a clear "Empty histogram" notice when there are no entries, draws the content, and appends
 * a page to the output PDF. It also writes a per-slice PDF file into a subdirectory.
 *
 * @param outputDir      Directory where outputs are written.
 * @param baseFileName   Base filename (without extension) used for the PDF and PNG directory.
 */
void hsPlots::SaveHistograms(const std::string& outputDir, const std::string& baseFileName) const {
    // NDC bounds and default text size for the empty-histogram notice overlay
    double x_1 = 0.18, y_1 = 0.3, x_2 = 0.86, y_2 = 0.7;
    double diplayTextSize = 0.1;

    // Compose the output multi-page PDF path and a subdirectory for per-slice images
    std::string PDF_File = outputDir + "/" + baseFileName + ".pdf";
    std::string PNG_Files_Base_Directory = outputDir + "/PNG_plots_" + baseFileName + "/";
    system(("mkdir -p " + PNG_Files_Base_Directory).c_str());  // Ensure subdir exists

    // Prepare a canvas with generous margins for axis titles and color palettes
    TCanvas* canvas = new TCanvas("canvas", "Histogram Canvas", 1000, 750);
    canvas->cd()->SetGrid();
    canvas->cd()->SetTopMargin(0.14);
    canvas->cd()->SetBottomMargin(0.14);
    canvas->cd()->SetLeftMargin(0.18);
    canvas->cd()->SetRightMargin(0.12);

    canvas->Print((PDF_File + "[").c_str());  // Open multi-page PDF stream

    // Iterate over per-slice histograms
    for (size_t i = 0; i < SlicedHistoListPtr->size(); ++i) {
        canvas->cd();
        canvas->Clear();  // Fresh page

        TObject* obj = (*SlicedHistoListPtr)[i];  // Retrieve this slice's object

        if (obj->InheritsFrom("TH1")) {  // Only handle TH1-derived types here
            TH1* hist = (TH1*)obj;       // Unified 1D/2D pointer

            // Optional skip block (kept commented): skip rendering if empty
            // if (hist->GetEntries() == 0) { continue; }

            // Axis cosmetics: readable labels and centered titles
            hist->GetXaxis()->SetTitleSize(0.06);
            hist->GetXaxis()->SetLabelSize(0.0425);
            hist->GetXaxis()->CenterTitle(true);
            hist->GetYaxis()->SetTitleSize(0.06);
            hist->GetYaxis()->SetLabelSize(0.0425);
            hist->GetYaxis()->CenterTitle(true);

            if (hist->InheritsFrom("TH2")) {  // 2D content branch
                if (hist->Integral() == 0. || hist->GetEntries() == 0) {
                    DrawEmptyHistogramNotice(x_1, y_1, x_2, y_2, diplayTextSize);  // Overlay notice
                }
                hist->Draw("colz");                  // Draw with color palette
            } else if (hist->InheritsFrom("TH1")) {  // 1D content branch
                if (hist->Integral() == 0. || hist->GetEntries() == 0) {
                    DrawEmptyHistogramNotice(x_1, y_1, x_2, y_2, diplayTextSize);  // Overlay notice
                }
                hist->Draw();  // Default 1D draw
            }

            // Append a page to the multi-page PDF
            canvas->Print(PDF_File.c_str());

            // Also save a per-slice PDF with index and name (uses PDF extension for high quality)
            std::string PNG_File = PNG_Files_Base_Directory + "/" + std::to_string(i) + "_" + hist->GetName() + ".pdf";
            canvas->SaveAs(PNG_File.c_str());
        } else {
            std::cerr << "\033[31m[hsPlots::SaveHistograms] ERROR: Object at index " << i << " is not a TH1-derived histogram.\n\033[0m";
        }
    }

    canvas->Print((PDF_File + "]").c_str());  // Close multi-page PDF stream

    delete canvas;  // Destroy canvas and free resources
}

// GetSlicedHistoList function ------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Return a copy of the internal vector of per-slice histogram pointers.
 *
 * Ownership is not transferred; the caller receives a shallow copy of the vector of pointers.
 *
 * @return Vector of TObject* where each entry corresponds to a slice's histogram.
 */
std::vector<TObject*> hsPlots::GetSlicedHistoList() const {
    return *SlicedHistoListPtr;  // Return by value to avoid exposing internal reference
}

#endif  // HSPLOTS_CPP
