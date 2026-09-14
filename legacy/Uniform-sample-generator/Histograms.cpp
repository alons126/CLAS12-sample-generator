//
// Created by alons on 08/08/2024.
//

#include "Histograms.h"

void InitHistograms_Tester_e(const double &Ebeam)
{
        /* Tester_e events */
        hTheta_e_Tester_e = new TH1D("Theta_e_Tester_e", "#theta_{e} in Tester_e sample;#theta_{e} [#circ]", 100, 0, 50);
        TH1_hist_list_Tester_e.push_back(hTheta_e_Tester_e);

        hPhi_e_Tester_e = new TH1D("Phi_e_Tester_e", "#phi_{e} in Tester_e sample;#phi_{e} [#circ]", 100, -180, 180);
        TH1_hist_list_Tester_e.push_back(hPhi_e_Tester_e);

        hP_e_Tester_e = new TH1D("P_e_Tester_e", "P_{e} in Tester_e sample;P_{e} [GeV]", 100, 0, Ebeam * 1.1);
        TH1_hist_list_Tester_e.push_back(hP_e_Tester_e);

        hTheta_e_VS_Phi_e_Tester_e = new TH2D("Theta_e_VS_Phi_e_Tester_e", "#theta_{e} vs. #phi_{e} in Tester_e sample;#phi_{e} [#circ];#theta_{e} [#circ]", 100, -180, 180, 100, 0, 50);
        TH2_hist_list_Tester_e.push_back(hTheta_e_VS_Phi_e_Tester_e);

        hTheta_e_VS_P_e_Tester_e = new TH2D("Theta_e_VS_P_e_Tester_e", "#theta_{e} vs. P_{e} in Tester_e sample;P_{e} [GeV];#theta_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50);
        TH2_hist_list_Tester_e.push_back(hTheta_e_VS_P_e_Tester_e);

        hPhi_e_VS_P_e_Tester_e = new TH2D("Phi_e_VS_P_e_Tester_e", "#phi_{e} vs. P_{e} in Tester_e sample;P_{e} [GeV];#phi_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180);
        TH2_hist_list_Tester_e.push_back(hPhi_e_VS_P_e_Tester_e);
}

void InitHistograms(const bool gen_1e_events, const bool gen_ep_events, const bool gen_en_events, const double &Ebeam)
{
    if (gen_1e_events)
    {
        /* 1e events */
        hTheta_e_1e = new TH1D("Theta_e_1e", "#theta_{e} in (e,e') sample;#theta_{e} [#circ]", 100, 0, 50);
        TH1_hist_list_1e.push_back(hTheta_e_1e);

        hPhi_e_1e = new TH1D("Phi_e_1e", "#phi_{e} in (e,e') sample;#phi_{e} [#circ]", 100, -180, 180);
        TH1_hist_list_1e.push_back(hPhi_e_1e);

        hP_e_1e = new TH1D("P_e_1e", "P_{e} in (e,e') sample;P_{e} [GeV]", 100, 0, Ebeam * 1.1);
        TH1_hist_list_1e.push_back(hP_e_1e);

        hVx_e_1e = new TH1D("Vx_e_1e", "V_{e,x} of e in (e,e') sample;V_{e,x} [cm]", 100, -5, 5);
        TH1_hist_list_1e.push_back(hVx_e_1e);

        hVy_e_1e = new TH1D("Vy_e_1e", "V_{e,y} of e in (e,e') sample;V_{e,y} [cm]", 100, -5, 5);
        TH1_hist_list_1e.push_back(hVy_e_1e);

        hVz_e_1e = new TH1D("Vz_e_1e", "V_{e,z} of e in (e,e') sample;V_{e,z} [cm]", 100, -5, 5);
        TH1_hist_list_1e.push_back(hVz_e_1e);

        hTheta_e_VS_Phi_e_1e = new TH2D("Theta_e_VS_Phi_e_1e", "#theta_{e} vs. #phi_{e} in (e,e') sample;#phi_{e} [#circ];#theta_{e} [#circ]", 100, -180, 180, 100, 0, 50);
        TH2_hist_list_1e.push_back(hTheta_e_VS_Phi_e_1e);

        hTheta_e_VS_P_e_1e = new TH2D("Theta_e_VS_P_e_1e", "#theta_{e} vs. P_{e} in (e,e') sample;P_{e} [GeV];#theta_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50);
        TH2_hist_list_1e.push_back(hTheta_e_VS_P_e_1e);

        hPhi_e_VS_P_e_1e = new TH2D("Phi_e_VS_P_e_1e", "#phi_{e} vs. P_{e} in (e,e') sample;P_{e} [GeV];#phi_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180);
        TH2_hist_list_1e.push_back(hPhi_e_VS_P_e_1e);
    }

    if (gen_ep_events)
    {
        /* ep events */
        hTheta_e_ep = new TH1D("Theta_e_ep", "#theta_{e} in (e,e'p) sample;#theta_{e} [#circ]", 100, 0, 50);
        TH1_hist_list_ep.push_back(hTheta_e_ep);

        hPhi_e_ep = new TH1D("Phi_e_ep", "#phi_{e} in (e,e'p) sample;#phi_{e} [#circ]", 100, -180, 180);
        TH1_hist_list_ep.push_back(hPhi_e_ep);

        hP_e_ep = new TH1D("P_e_ep", "P_{e} in (e,e'p) sample;P_{e} [GeV]", 100, 0, Ebeam * 1.1);
        TH1_hist_list_ep.push_back(hP_e_ep);

        hVx_e_ep = new TH1D("Vx_e_ep", "V_{e,x} of e in (e,e'p) sample;V_{e,x} [cm]", 100, -5, 5);
        TH1_hist_list_ep.push_back(hVx_e_ep);

        hVy_e_ep = new TH1D("Vy_e_ep", "V_{e,y} of e in (e,e'p) sample;V_{e,y} [cm]", 100, -5, 5);
        TH1_hist_list_ep.push_back(hVy_e_ep);

        hVz_e_ep = new TH1D("Vz_e_ep", "V_{e,z} of e in (e,e'p) sample;V_{e,z} [cm]", 100, -5, 5);
        TH1_hist_list_ep.push_back(hVz_e_ep);

        hTheta_e_VS_Phi_e_ep = new TH2D("Theta_e_VS_Phi_e_ep", "#theta_{e} vs. #phi_{e} in (e,e'p) sample;#phi_{e} [#circ];#theta_{e} [#circ]", 100, -180, 180, 100, 0, 50);
        TH2_hist_list_ep.push_back(hTheta_e_VS_Phi_e_ep);

        hTheta_e_VS_P_e_ep = new TH2D("Theta_e_VS_P_e_ep", "#theta_{e} vs. P_{e} in (e,e'p) sample;P_{e} [GeV];#theta_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50);
        TH2_hist_list_ep.push_back(hTheta_e_VS_P_e_ep);

        hPhi_e_VS_P_e_ep = new TH2D("Phi_e_VS_P_e_ep", "#phi_{e} vs. P_{e} in (e,e'p) sample;P_{e} [GeV];#phi_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180);
        TH2_hist_list_ep.push_back(hPhi_e_VS_P_e_ep);

        hTheta_p_ep = new TH1D("Theta_p_ep", "#theta_{p} in (e,e'p) sample;#theta_{p} [#circ]", 100, 0, 50);
        TH1_hist_list_ep.push_back(hTheta_p_ep);

        hPhi_p_ep = new TH1D("Phi_p_ep", "#phi_{p} in (e,e'p) sample;#phi_{p} [#circ]", 100, -180, 180);
        TH1_hist_list_ep.push_back(hPhi_p_ep);

        hP_p_ep = new TH1D("P_p_ep", "P_{p} in (e,e'p) sample;P_{p} [GeV]", 100, 0, Ebeam * 1.1);
        TH1_hist_list_ep.push_back(hP_p_ep);

        hVx_p_ep = new TH1D("Vx_p_ep", "V_{p,x} of p in (e,e'p) sample;V_{p,x} [cm]", 100, -5, 5);
        TH1_hist_list_ep.push_back(hVx_p_ep);

        hVy_p_ep = new TH1D("Vy_p_ep", "V_{p,y} of p in (e,e'p) sample;V_{p,y} [cm]", 100, -5, 5);
        TH1_hist_list_ep.push_back(hVy_p_ep);

        hVz_p_ep = new TH1D("Vz_p_ep", "V_{p,z} of p in (e,e'p) sample;V_{p,z} [cm]", 100, -5, 5);
        TH1_hist_list_ep.push_back(hVz_p_ep);

        hTheta_p_VS_Phi_p_ep = new TH2D("Theta_p_VS_Phi_p_ep", "#theta_{p} vs. #phi_{p} in (e,e'p) sample;#phi_{p} [#circ];#theta_{p} [#circ]", 100, -180, 180, 100, 0, 50);
        TH2_hist_list_ep.push_back(hTheta_p_VS_Phi_p_ep);

        hTheta_p_VS_P_p_ep = new TH2D("Theta_p_VS_P_p_ep", "#theta_{p} vs. P_{p} in (e,e'p) sample;P_{p} [GeV];#theta_{p} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50);
        TH2_hist_list_ep.push_back(hTheta_p_VS_P_p_ep);

        hPhi_p_VS_P_p_ep = new TH2D("Phi_p_VS_P_p_ep", "#phi_{p} vs. P_{p} in (e,e'p) sample;P_{p} [GeV];#phi_{p} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180);
        TH2_hist_list_ep.push_back(hPhi_p_VS_P_p_ep);

        hP_e_VS_P_p_ep = new TH2D("P_e_VS_P_p_ep", "P_{e} vs. P_{p} in (e,e'p) sample;P_{p} [GeV];P_{e} [GeV]", 100, 0, Ebeam * 1.1, 100, 0, Ebeam * 1.1);
        TH2_hist_list_ep.push_back(hP_e_VS_P_p_ep);

        hP_e_VS_Theta_p_ep = new TH2D("P_e_VS_Theta_p_ep", "P_{e} vs. #theta_{p} in (e,e'p) sample;#theta_{p} [#circ];P_{e} [GeV]", 100, 0, 50, 100, 0, Ebeam * 1.1);
        TH2_hist_list_ep.push_back(hP_e_VS_Theta_p_ep);

        hP_e_VS_Phi_p_ep = new TH2D("P_e_VS_Phi_p_ep", "P_{e} vs. #phi_{p} in (e,e'p) sample;#phi_{p} [#circ];P_{e} [GeV]", 100, -180, 180, 100, 0, Ebeam * 1.1);
        TH2_hist_list_ep.push_back(hP_e_VS_Phi_p_ep);

        hTheta_e_VS_P_p_ep = new TH2D("Theta_e_VS_P_p_ep", "#theta_{e} vs. P_{p} in (e,e'p) sample;P_{p} [GeV];#theta_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50);
        TH2_hist_list_ep.push_back(hTheta_e_VS_P_p_ep);

        hTheta_e_VS_Theta_p_ep = new TH2D("Theta_e_VS_Theta_p_ep", "#theta_{e} vs. #theta_{p} in (e,e'p) sample;#theta_{p} [#circ];#theta_{e} [#circ]", 100, 0, 50, 100, 0, 50);
        TH2_hist_list_ep.push_back(hTheta_e_VS_Theta_p_ep);

        hTheta_e_VS_Phi_p_ep = new TH2D("Theta_e_VS_Phi_p_ep", "#theta_{e} vs. #phi_{p} in (e,e'p) sample;#phi_{p} [#circ];#theta_{e} [#circ]", 100, -180, 180, 100, 0, 50);
        TH2_hist_list_ep.push_back(hTheta_e_VS_Phi_p_ep);

        hPhi_e_VS_P_p_ep = new TH2D("Phi_e_VS_P_p_ep", "#phi_{e} vs. P_{p} in (e,e'p) sample;P_{p} [GeV];#phi_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180);
        TH2_hist_list_ep.push_back(hPhi_e_VS_P_p_ep);

        hPhi_e_VS_Theta_p_ep = new TH2D("Phi_e_VS_Theta_p_ep", "#phi_{e} vs. #theta_{p} in (e,e'p) sample;#theta_{p} [#circ];#phi_{e} [#circ]", 100, 0, 50, 100, -180, 180);
        TH2_hist_list_ep.push_back(hPhi_e_VS_Theta_p_ep);

        hPhi_e_VS_Phi_p_ep = new TH2D("Phi_e_VS_Phi_p_ep", "#phi_{e} vs. #phi_{p} in (e,e'p) sample;#phi_{p} [#circ];#phi_{e} [#circ]", 100, -200, 200, 100, -200, 200);
        TH2_hist_list_ep.push_back(hPhi_e_VS_Phi_p_ep);
    }

    if (gen_en_events)
    {
        /* en events */
        hTheta_e_en = new TH1D("Theta_e_en", "#theta_{e} in (e,e'n) sample;#theta_{e} [#circ]", 100, 0, 50);
        TH1_hist_list_en.push_back(hTheta_e_en);

        hPhi_e_en = new TH1D("Phi_e_en", "#phi_{e} in (e,e'n) sample;#phi_{e} [#circ]", 100, -180, 180);
        TH1_hist_list_en.push_back(hPhi_e_en);

        hP_e_en = new TH1D("P_e_en", "P_{e} in (e,e'n) sample;P_{e} [GeV]", 100, 0, Ebeam * 1.1);
        TH1_hist_list_en.push_back(hP_e_en);

        hVx_e_en = new TH1D("Vx_e_en", "V_{e,x} of e in (e,e'n) sample;V_{e,x} [cm]", 100, -5, 5);
        TH1_hist_list_en.push_back(hVx_e_en);

        hVy_e_en = new TH1D("Vy_e_en", "V_{e,y} of e in (e,e'n) sample;V_{e,y} [cm]", 100, -5, 5);
        TH1_hist_list_en.push_back(hVy_e_en);

        hVz_e_en = new TH1D("Vz_e_en", "V_{e,z} of e in (e,e'n) sample;V_{e,z} [cm]", 100, -5, 5);
        TH1_hist_list_en.push_back(hVz_e_en);

        hTheta_e_VS_Phi_e_en = new TH2D("Theta_e_VS_Phi_e_en", "#theta_{e} vs. #phi_{e} in (e,e'n) sample;#phi_{e} [#circ];#theta_{e} [#circ]", 100, -180, 180, 100, 0, 50);
        TH2_hist_list_en.push_back(hTheta_e_VS_Phi_e_en);

        hTheta_e_VS_P_e_en = new TH2D("Theta_e_VS_P_e_en", "#theta_{e} vs. P_{e} in (e,e'n) sample;P_{e} [GeV];#theta_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50);
        TH2_hist_list_en.push_back(hTheta_e_VS_P_e_en);

        hPhi_e_VS_P_e_en = new TH2D("Phi_e_VS_P_e_en", "#phi_{e} vs. P_{e} in (e,e'n) sample;P_{e} [GeV];#phi_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180);
        TH2_hist_list_en.push_back(hPhi_e_VS_P_e_en);

        hTheta_n_en = new TH1D("Theta_n_en", "#theta_{n} in (e,e'n) sample;#theta_{n} [#circ]", 100, 0, 50);
        TH1_hist_list_en.push_back(hTheta_n_en);

        hPhi_n_en = new TH1D("Phi_n_en", "#phi_{n} in (e,e'n) sample;#phi_{n} [#circ]", 100, -180, 180);
        TH1_hist_list_en.push_back(hPhi_n_en);

        hP_n_en = new TH1D("P_n_en", "P_{n} in (e,e'n) sample;P_{n} [GeV]", 100, 0, Ebeam * 1.1);
        TH1_hist_list_en.push_back(hP_n_en);

        hVx_n_en = new TH1D("Vx_n_en", "V_{n,x} of n in (e,e'n) sample;V_{n,x} [cm]", 100, -5, 5);
        TH1_hist_list_en.push_back(hVx_n_en);

        hVy_n_en = new TH1D("Vy_n_en", "V_{n,y} of n in (e,e'n) sample;V_{n,y} [cm]", 100, -5, 5);
        TH1_hist_list_en.push_back(hVy_n_en);

        hVz_n_en = new TH1D("Vz_n_en", "V_{n,z} of n in (e,e'n) sample;V_{n,z} [cm]", 100, -5, 5);
        TH1_hist_list_en.push_back(hVz_n_en);

        hTheta_n_VS_Phi_n_en = new TH2D("Theta_n_VS_Phi_n_en", "#theta_{n} vs. #phi_{n} in (e,e'n) sample;#phi_{n} [#circ];#theta_{n} [#circ]", 100, -180, 180, 100, 0, 50);
        TH2_hist_list_en.push_back(hTheta_n_VS_Phi_n_en);

        hTheta_n_VS_P_n_en = new TH2D("Theta_n_VS_P_n_en", "#theta_{n} vs. P_{n} in (e,e'n) sample;P_{n} [GeV];#theta_{n} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50);
        TH2_hist_list_en.push_back(hTheta_n_VS_P_n_en);

        hPhi_n_VS_P_n_en = new TH2D("Phi_n_VS_P_n_en", "#phi_{n} vs. P_{n} in (e,e'n) sample;P_{n} [GeV];#phi_{n} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180);
        TH2_hist_list_en.push_back(hPhi_n_VS_P_n_en);

        hP_e_VS_P_n_en = new TH2D("P_e_VS_P_n_en", "P_{e} vs. P_{n} in (e,e'n) sample;P_{n} [GeV];P_{e} [GeV]", 100, 0, Ebeam * 1.1, 100, 0, Ebeam * 1.1);
        TH2_hist_list_en.push_back(hP_e_VS_P_n_en);

        hP_e_VS_Theta_n_en = new TH2D("P_e_VS_Theta_n_en", "P_{e} vs. #theta_{n} in (e,e'n) sample;#theta_{n} [#circ];P_{e} [GeV]", 100, 0, 50, 100, 0, Ebeam * 1.1);
        TH2_hist_list_en.push_back(hP_e_VS_Theta_n_en);

        hP_e_VS_Phi_n_en = new TH2D("P_e_VS_Phi_n_en", "P_{e} vs. #phi_{n} in (e,e'n) sample;#phi_{n} [#circ];P_{e} [GeV]", 100, -180, 180, 100, 0, Ebeam * 1.1);
        TH2_hist_list_en.push_back(hP_e_VS_Phi_n_en);

        hTheta_e_VS_P_n_en = new TH2D("Theta_e_VS_P_n_en", "#theta_{e} vs. P_{n} in (e,e'n) sample;P_{n} [GeV];#theta_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50);
        TH2_hist_list_en.push_back(hTheta_e_VS_P_n_en);

        hTheta_e_VS_Theta_n_en = new TH2D("Theta_e_VS_Theta_n_en", "#theta_{e} vs. #theta_{n} in (e,e'n) sample;#theta_{n} [#circ];#theta_{e} [#circ]", 100, 0, 50, 100, 0, 50);
        TH2_hist_list_en.push_back(hTheta_e_VS_Theta_n_en);

        hTheta_e_VS_Phi_n_en = new TH2D("Theta_e_VS_Phi_n_en", "#theta_{e} vs. #phi_{n} in (e,e'n) sample;#phi_{n} [#circ];#theta_{e} [#circ]", 100, -180, 180, 100, 0, 50);
        TH2_hist_list_en.push_back(hTheta_e_VS_Phi_n_en);

        hPhi_e_VS_P_n_en = new TH2D("Phi_e_VS_P_n_en", "#phi_{e} vs. P_{n} in (e,e'n) sample;P_{n} [GeV];#phi_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180);
        TH2_hist_list_en.push_back(hPhi_e_VS_P_n_en);

        hPhi_e_VS_Theta_n_en = new TH2D("Phi_e_VS_Theta_n_en", "#phi_{e} vs. #theta_{n} in (e,e'n) sample;#theta_{n} [#circ];#phi_{e} [#circ]", 100, 0, 50, 100, -180, 180);
        TH2_hist_list_en.push_back(hPhi_e_VS_Theta_n_en);

        hPhi_e_VS_Phi_n_en = new TH2D("Phi_e_VS_Phi_n_en", "#phi_{e} vs. #phi_{n} in (e,e'n) sample;#phi_{n} [#circ];#phi_{e} [#circ]", 100, -200, 200, 100, -200, 200);
        TH2_hist_list_en.push_back(hPhi_e_VS_Phi_n_en);
    }
}
