#include <TFile.h>
#include <TH1.h>
#include <TKey.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
int main(int argc, char** argv) {
    if (argc != 3) return 2;
    try {
        TFile expected(argv[1]), actual(argv[2]);
        if (expected.IsZombie() || actual.IsZombie()) throw std::runtime_error("Missing ROOT histogram file");
        if (expected.GetNkeys() != actual.GetNkeys()) throw std::runtime_error("Different histogram count");
        TIter next(expected.GetListOfKeys());
        while (auto* key = static_cast<TKey*>(next())) {
            auto* old = dynamic_cast<TH1*>(expected.Get(key->GetName()));
            auto* now = dynamic_cast<TH1*>(actual.Get(key->GetName()));
            if (!old || !now || old->GetNcells() != now->GetNcells() || old->GetEntries() != now->GetEntries()) throw std::runtime_error(key->GetName());
            for (int bin = 0; bin < old->GetNcells(); ++bin)
                if (old->GetBinContent(bin) != now->GetBinContent(bin) || std::abs(old->GetBinError(bin) - now->GetBinError(bin)) > 1e-12)
                    throw std::runtime_error(std::string(key->GetName()) + ": bin mismatch");
            for (int axis = 0; axis < 2; ++axis) {
                auto* a = axis ? old->GetYaxis() : old->GetXaxis();
                auto* b = axis ? now->GetYaxis() : now->GetXaxis();
                if (a->GetNbins() != b->GetNbins() || a->GetXmin() != b->GetXmin() || a->GetXmax() != b->GetXmax()) throw std::runtime_error("Axis mismatch");
            }
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
