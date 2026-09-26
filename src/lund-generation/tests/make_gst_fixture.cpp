//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file make_gst_fixture.cpp
 * @brief Create synthetic GST inputs for converter tests.
 *
 * Purpose:
 *   Create normal, comparison, and invalid test files without an external dataset.
 *
 * Workflow:
 *   CTest chooses the case and reads this program's exit status.
 */

#include <TFile.h>
#include <TTree.h>

#include <string>

// main ------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* main */
/**
 * @brief Create synthetic GST inputs for converter tests.
 *
 * Steps:
 *   Create a GST tree, add the fields for the chosen case, fill it, and save it.
 *
 * @param argc Number of executable arguments.
 * @param argv Paths and options supplied by the caller.
 *
 * @return Zero on success; nonzero if the command or output file is invalid.
 */
int main(int argc, char** argv) {
    if (argc < 2) { return 1; }
    TFile file(argv[1], "RECREATE");
    TTree tree("gst", "Synthetic conversion fixture");
    Bool_t qel = true, mec = false, res = false, dis = false;
    const std::string mode = argc > 2 ? argv[2] : "normal";
    // Include two unsupported particles to test filtering while keeping the supported photon.
    Int_t resid = 7, nf = mode == "large" ? 300 : 7, coordinate_count = mode == "mismatched-arrays" ? 6 : nf;
    Int_t pdgf[320] = {2212, 2112, 211, -211, 111, 22, 321};
    Double_t pxf[320] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7};
    Double_t pyf[320] = {}, pzf[320] = {1, 1, 1, 1, 1, 1, 1};
    Double_t pxl = 0.5, pyl = 0.1, pzl = 2;

    // Make all 300 particles valid and different so the test can check that no fixed limit is used.
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
    // Add four supported processes, one skipped event, and enough input to test the cutoff.
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
