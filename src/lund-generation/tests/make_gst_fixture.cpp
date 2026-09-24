//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file make_gst_fixture.cpp
 * @brief Create synthetic GST inputs for converter tests.
 *
 * Purpose:
 *   Provide normal, parity and malformed-input cases without external event datasets.
 *
 * Workflow:
 *   CTest supplies paths and fixtures; assertions or exit codes report failures to the test runner.
 */

#include <TFile.h>
#include <TTree.h>

#include <string>

// main ------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* main */
/**
 * @brief Create synthetic GST inputs for converter tests.
 *
 * Algorithm:
 *   Provide normal, parity and malformed-input cases without external event datasets.
 *
 * @param argc Number of executable arguments.
 * @param argv Paths and options supplied by the caller.
 *
 * @return Zero on success; nonzero for a failed run, invalid invocation or test mismatch.
 */
int main(int argc, char** argv) {
    if (argc < 2) { return 1; }
    TFile file(argv[1], "RECREATE");
    TTree tree("gst", "Synthetic conversion fixture");
    Bool_t qel = true, mec = false, res = false, dis = false;
    const std::string mode = argc > 2 ? argv[2] : "normal";
    // Include two unsupported identities to verify filtering while retaining the supported photon.
    // Large mode repeats zero-initialized unsupported entries.
    Int_t resid = 7, nf = mode == "large" ? 300 : 7, coordinate_count = mode == "mismatched-arrays" ? 6 : nf;
    Int_t pdgf[320] = {2212, 2112, 211, -211, 111, 22, 321};
    Double_t pxf[320] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7};
    Double_t pyf[320] = {}, pzf[320] = {1, 1, 1, 1, 1, 1, 1};
    Double_t pxl = 0.5, pyl = 0.1, pzl = 2;

    // Make every entry in the 300-element case supported and distinguishable. The integration test
    // then proves that conversion traverses the complete ROOT-reported length rather than a fixed cap.
    if (mode == "large") {
        for (int i = 0; i < nf; ++i) {
            pdgf[i] = 22;
            pxf[i] = 0.001 * i;
            pyf[i] = 0.002 * i;
            pzf[i] = 1. + 0.003 * i;
        }
    }
    tree.Branch("qel", &qel, "qel/O");
    tree.Branch("mec", &mec, "mec/O");
    tree.Branch("res", &res, "res/O");
    tree.Branch("dis", &dis, "dis/O");
    tree.Branch("resid", &resid, "resid/I");
    tree.Branch("nf", &nf, "nf/I");
    tree.Branch("coordinate_count", &coordinate_count, "coordinate_count/I");
    tree.Branch("pdgf", pdgf, "pdgf[nf]/I");
    tree.Branch("pxf", pxf, mode == "mismatched-arrays" ? "pxf[coordinate_count]/D" : "pxf[nf]/D");
    tree.Branch("pyf", pyf, "pyf[nf]/D");
    tree.Branch("pzf", pzf, "pzf[nf]/D");
    tree.Branch("pxl", &pxl, "pxl/D");
    tree.Branch("pyl", &pyl, "pyl/D");
    Double_t El = 2.1, Ef[320] = {};
    tree.Branch("El", &El, "El/D");
    tree.Branch("Ef", Ef, "Ef[nf]/D");
    Float_t wrong_pzl = 2;
    if (mode == "wrong-type") {
        tree.Branch("pzl", &wrong_pzl, "pzl/F");
    } else if (mode != "missing") {
        tree.Branch("pzl", &pzl, "pzl/D");
    }
    // Four supported processes plus a skipped event and an input tail that exercises the cutoff.
    for (int i = 0; i < (mode == "empty" ? 0 : mode == "parity" ? 24000 : 7); ++i) {
        qel = (i == 0 || i >= 5);
        mec = i == 1;
        res = i == 2;
        dis = i == 3;
        if (mode == "parity") {
            qel = i % 5 == 0;
            mec = i % 5 == 1;
            res = i % 5 == 2;
            dis = i % 5 == 3;
            pxl = 0.2 + (i % 71) * 0.013;
            pyl = -0.1 + (i % 13) * 0.01;
            pzl = 1 + (i % 17) * 0.03;
        }
        if (mode == "unsupported") { qel = mec = res = dis = false; }
        tree.Fill();
    }
    tree.Write();
    return file.IsZombie() ? 1 : 0;
}
#pragma endregion
