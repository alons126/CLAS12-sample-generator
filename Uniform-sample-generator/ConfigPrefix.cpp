//
// Created by alons on 14/08/2024.
//

#include <iomanip>
#include <string>

#include "ConfigBeamE.cpp"
#include "TString.h"

using namespace std;

TString ConfigPrefix(const bool gen_1e_events, const bool gen_ep_events, const bool gen_en_events, const double &BeamE) {
    TString opfnPrefix;
    string be = ConfigBeamE(BeamE);

    if (gen_1e_events) {
        opfnPrefix = "Uniform_1e_sample_" + be;
    } else if (gen_ep_events) {
        opfnPrefix = "Uniform_ep_sample_" + be;
    } else if (gen_en_events) {
        opfnPrefix = "Uniform_en_sample_" + be;
    }

    return opfnPrefix;
}
