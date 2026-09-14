//
// Created by Alon Sportes on 24/02/2025.
//

#ifndef UTILITIES_H
#define UTILITIES_H

#include <TGraph.h>
#include <TH1D.h>
#include <TLatex.h>
#include <TObject.h>

#include <iostream>
#include <string>

#include "basic_tools.h"
namespace utilities {
using namespace basic_tools;
using basic_tools::FindSubstring;

// ConfigRegion function ------------------------------------------------------------------------------------------------------------------------------------------------

const std::string ConfigRegion(const std::string& Histogram2DNameCopy) { return FindSubstring(Histogram2DNameCopy, "CD") ? "CD" : "FD"; }

// ConfigSampleType function --------------------------------------------------------------------------------------------------------------------------------------------

const std::string ConfigSampleType(const std::string& SampleName) {
    if (FindSubstring(SampleName, "sim")) {
        return "s";
    } else if (FindSubstring(SampleName, "data")) {
        return "d";
    }

    return "";
}

// GetHistogramFromVector function --------------------------------------------------------------------------------------------------------------------------------------

TObject* GetHistogramFromVector(const std::vector<TObject*>& HistoList, const std::string& searchString, const std::string& searchStringOption = "name") {
    for (const auto& hist : HistoList) {
        if (!hist) continue;

        if ((searchStringOption == "Name" || searchStringOption == "name") && hist->GetName() == searchString) {
            return hist;
        } else if ((searchStringOption == "Title" || searchStringOption == "title") && hist->GetTitle() == searchString) {
            return hist;
        }
    }

    return nullptr;  // Return nullptr if no match is found
}

// FillHistogramByProcess function --------------------------------------------------------------------------------------------------------------------------------------

void FillHistogramByProcess(double Variable, TH1D* Histogram_All_Int, TH1D* Histogram_QE, TH1D* Histogram_MEC, TH1D* Histogram_RES, TH1D* Histogram_DIS, bool qel, bool mec, bool res,
                            bool dis, double weight = 1., bool IsMC = true, bool IsData = false) {
    Histogram_All_Int->Fill(Variable, weight);

    if (IsMC) {
        if (qel) {
            Histogram_QE->Fill(Variable, weight);
        } else if (mec) {
            Histogram_MEC->Fill(Variable, weight);
        } else if (res) {
            Histogram_RES->Fill(Variable, weight);
        } else if (dis) {
            Histogram_DIS->Fill(Variable, weight);
        }
    }
}

// GetHistogramEntries function -----------------------------------------------------------------------------------------------------------------------------------------

double GetHistogramEntriesFromVector(const std::vector<TObject*>& HistoList, const std::string& searchString, const std::string& searchStringOption = "name") {
    if (TObject* Object = GetHistogramFromVector(HistoList, searchString, searchStringOption)) {
        if (TH1* Histogram = dynamic_cast<TH1*>(Object)) { return Histogram->GetEntries(); }
    }
    return -1;  // Return -1 if no match is found
}

// drawtext function ----------------------------------------------------------------------------------------------------------------------------------------------------

void drawtext() {
    Int_t i, n;
    Double_t x, y;
    TLatex l;

    l.SetTextSize(0.025);
    l.SetTextFont(0);
    //    l.SetTextFont(42);
    l.SetTextAlign(21);
    l.SetTextColor(kMagenta);
    //    l.SetTextColor(kBlue);
    l.SetLineColor(kWhite);

    auto g = (TGraph*)gPad->GetListOfPrimitives()->FindObject("Graph");
    n = g->GetN();

    for (i = 0; i < n; i++) {
        g->GetPoint(i, x, y);
        l.PaintText(x, y + 0.02, Form("(%4.5f,%4.1f)", x, y));
    }
}

};  // namespace utilities

namespace converter_targets_namespace {}
#define targets converter_targets_namespace

inline constexpr double m_e = 0.511e-3;
inline constexpr double m_p = 0.938272;
inline constexpr double m_n = 0.93957;
inline constexpr double m_piplus = 0.13957;
inline constexpr double m_piminus = 0.13957;
inline constexpr double m_pizero = 0.13957;

#endif  // UTILITIES_H
