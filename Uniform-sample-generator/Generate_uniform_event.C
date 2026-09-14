#include <cmath>

#include "AngleCalc.h"
#include "Histograms.h"
#include "addParticle.C"
#include "targets.h"

#pragma region /* Uniform (e,e') events */

void Generate_uniform_event(const string &target, vector<TH1 *> TH1_hist_list, vector<TH2 *> TH2_hist_list, ofstream &OutFile, TString formatstring, TString outstring, TRandom3 &ran,
                            int nEvents = 10000, double targP = 0., double beamP = 0., int interactN = 1, int beamType = 11, double beamE_in_lundfiles = -99, double Ebeam = -99,
                            double weight = 1, double mass_e = 0.511e-3, double theta_e_min = 5., double theta_e_max = 40.) {
    double pi = 3.14159265359;
    int i = 0;

    while (i < nEvents) {
        TVector3 temp_vtx_e = randomVertex(target);

        /* Create nEvents uniform (e,e') events. LUND header for the event: */
        formatstring = "%i \t %i \t %i \t %f \t %f \t %i \t %f \t %i \t %d \t %.2f \n";
        outstring = Form(formatstring, 1, 1 /* A */, 1 /* Z */, targP, beamP, beamType, beamE_in_lundfiles, interactN, i, weight);

        /* Electron */
        TVector3 P_e_3v;
        double Theta_e = ran.Uniform(theta_e_min, theta_e_max);  // Uniform Theta_e from theta_e_min to theta_e_max
        double Phi_e = ran.Uniform(-180., 180.);                 // Uniform Phi_e from -180 to +180
        double P_e = ran.Uniform(0, Ebeam);                      // Uniform P_e from 0 to P_e = beamE
        P_e_3v.SetMagThetaPhi(P_e, Theta_e * TMath::DegToRad(), Phi_e * TMath::DegToRad());
        OutFile << outstring;
        OutFile << AddParticle(1, 11, P_e_3v, mass_e, temp_vtx_e);
        // OutFile << AddParticle(1, 11, P_e_3v, mass_e, vtx);

        hTheta_e_1e->Fill(P_e_3v.Theta() * TMath::RadToDeg());
        hPhi_e_1e->Fill(P_e_3v.Phi() * TMath::RadToDeg());
        hP_e_1e->Fill(P_e_3v.Mag());

        hTheta_e_VS_Phi_e_1e->Fill(P_e_3v.Phi() * TMath::RadToDeg(), P_e_3v.Theta() * TMath::RadToDeg());
        hTheta_e_VS_P_e_1e->Fill(P_e_3v.Mag(), P_e_3v.Theta() * TMath::RadToDeg());
        hPhi_e_VS_P_e_1e->Fill(P_e_3v.Mag(), P_e_3v.Phi() * TMath::RadToDeg());

        hVx_e_1e->Fill(temp_vtx_e.X());
        hVy_e_1e->Fill(temp_vtx_e.Y());
        hVz_e_1e->Fill(temp_vtx_e.Z());

        ++i;
    }
}

#pragma endregion

#pragma region /* Uniform (e,e'p) and (e,e'n) events */

void Generate_uniform_event(TString OutPutFolder, const string &target, vector<TH1 *> TH1_hist_list, vector<TH2 *> TH2_hist_list, ofstream &OutFile, TString formatstring,
                            TString outstring, TRandom3 &ran, int N_pid, int nEvents = 10000, const int nParticles = 2, double targP = 0., double beamP = 0., int interactN = 1,
                            int beamType = 11, double beamE_in_lundfiles = -99, double Ebeam = -99, double weight = 1, double mass_e = 0.511e-3, double mass_N = 0.938272,
                            double theta_e_min = 5., double theta_e_max = 40., double theta_N_min = 5., double theta_N_max = 45.) {
    double pi = 3.14159265359;
    int i = 0;

    while (i < nEvents) {
        TVector3 temp_vtx_e = randomVertex(target);

        /* Create nEvents uniform (e,e'p) events and nEvents uniform (e,e'n) events. LUND header for the event: */
        formatstring = "%i \t %i \t %i \t %.3f \t %.3f \t %i \t %.1f \t %i \t %i \t %.3f \n";
        outstring = Form(formatstring, nParticles, 1 /* A */, 1 /* Z */, targP, beamP, beamType, beamE_in_lundfiles, interactN, i, weight);
        // OutFile << outstring;

        /* Nucleon: uniform Theta_N, Phi_N, and P_N */
        TVector3 P_N_3v;
        double Theta_N = ran.Uniform(theta_N_min, theta_N_max);  // Uniform Theta_N from theta_N_min to theta_N_max
        double Phi_N = ran.Uniform(-180., 180.);                 // Uniform Phi_N from -180 to +180
        // double P_N = ran.Uniform(0.3, Ebeam);                    // Uniform P_N from 0 to P_N = beamE
        double P_N = 1.;  // Uniform P_N from 0.3 to P_N = beamE
        P_N_3v.SetMagThetaPhi(P_N, Theta_N * TMath::DegToRad(), Phi_N * TMath::DegToRad());

        /* Electron: constant Theta_e; Phi_e is the inverse of Phi_N; and constant P_e */
        TVector3 P_e_3v;
        double Theta_e = 25.;                          // Constant Theta_e at Theta_e = 25 degrees
        double Phi_e = getPhi_e(OutPutFolder, Phi_N);  // Phi_e is one of {-120, -60, 0, 60, 120, 180} that is the closest to 180 degrees away from phi_N
        double P_e = Ebeam;                            // Constant P_e at P_e = Ebeam, since the electron is the trigger
        P_e_3v.SetMagThetaPhi(P_e, Theta_e * TMath::DegToRad(), Phi_e * TMath::DegToRad());

        // OutFile << outstring;
        // OutFile << AddParticle(1, 11, P_e_3v, mass_e, randomVertex(target));
        // OutFile << AddParticle(2, N_pid, P_N_3v, mass_N, randomVertex(target));
        // // OutFile << AddParticle(1, 11, P_e_3v, mass_e, vtx);
        // // OutFile << AddParticle(2, N_pid, P_N_3v, mass_N, vtx);
        // // OutFile << "\n";

        if (N_pid == 2212) {
            TVector3 temp_vtx_N = temp_vtx_e;  // Set the nucleon vertex to be the same as the electron vertex as they come from the same interaction point.

            OutFile << outstring;
            OutFile << AddParticle(1, 11, P_e_3v, mass_e, temp_vtx_e);
            OutFile << AddParticle(2, N_pid, P_N_3v, mass_N, temp_vtx_N);

            hTheta_e_ep->Fill(P_e_3v.Theta() * TMath::RadToDeg());
            hPhi_e_ep->Fill(P_e_3v.Phi() * TMath::RadToDeg());
            hP_e_ep->Fill(P_e_3v.Mag());

            hVx_e_ep->Fill(temp_vtx_e.X());
            hVy_e_ep->Fill(temp_vtx_e.Y());
            hVz_e_ep->Fill(temp_vtx_e.Z());

            hTheta_e_VS_Phi_e_ep->Fill(P_e_3v.Phi() * TMath::RadToDeg(), P_e_3v.Theta() * TMath::RadToDeg());
            hTheta_e_VS_P_e_ep->Fill(P_e_3v.Mag(), P_e_3v.Theta() * TMath::RadToDeg());
            hPhi_e_VS_P_e_ep->Fill(P_e_3v.Mag(), P_e_3v.Phi() * TMath::RadToDeg());

            hTheta_p_ep->Fill(P_N_3v.Theta() * TMath::RadToDeg());
            hPhi_p_ep->Fill(P_N_3v.Phi() * TMath::RadToDeg());
            hP_p_ep->Fill(P_N_3v.Mag());

            hVx_p_ep->Fill(temp_vtx_N.X());
            hVy_p_ep->Fill(temp_vtx_N.Y());
            hVz_p_ep->Fill(temp_vtx_N.Z());

            hTheta_p_VS_Phi_p_ep->Fill(P_N_3v.Phi() * TMath::RadToDeg(), P_N_3v.Theta() * TMath::RadToDeg());
            hTheta_p_VS_P_p_ep->Fill(P_N_3v.Mag(), P_N_3v.Theta() * TMath::RadToDeg());
            hPhi_p_VS_P_p_ep->Fill(P_N_3v.Mag(), P_N_3v.Phi() * TMath::RadToDeg());

            hP_e_VS_P_p_ep->Fill(P_N_3v.Mag(), P_e_3v.Mag());
            hP_e_VS_Theta_p_ep->Fill(P_N_3v.Theta() * TMath::RadToDeg(), P_e_3v.Mag());
            hP_e_VS_Phi_p_ep->Fill(P_N_3v.Phi() * TMath::RadToDeg(), P_e_3v.Mag());

            hTheta_e_VS_P_p_ep->Fill(P_N_3v.Mag(), P_e_3v.Theta() * TMath::RadToDeg());
            hTheta_e_VS_Theta_p_ep->Fill(P_N_3v.Theta() * TMath::RadToDeg(), P_e_3v.Theta() * TMath::RadToDeg());
            hTheta_e_VS_Phi_p_ep->Fill(P_N_3v.Phi() * TMath::RadToDeg(), P_e_3v.Theta() * TMath::RadToDeg());

            hPhi_e_VS_P_p_ep->Fill(P_N_3v.Mag(), P_e_3v.Phi() * TMath::RadToDeg());
            hPhi_e_VS_Theta_p_ep->Fill(P_N_3v.Theta() * TMath::RadToDeg(), P_e_3v.Phi() * TMath::RadToDeg());
            hPhi_e_VS_Phi_p_ep->Fill(P_N_3v.Phi() * TMath::RadToDeg(), P_e_3v.Phi() * TMath::RadToDeg());
        } else if (N_pid == 2112) {
            TVector3 temp_vtx_N = temp_vtx_e;  // Set the nucleon vertex to be the same as the electron vertex as they come from the same interaction point.

            OutFile << outstring;
            OutFile << AddParticle(1, 11, P_e_3v, mass_e, temp_vtx_e);
            OutFile << AddParticle(2, N_pid, P_N_3v, mass_N, temp_vtx_N);

            hTheta_e_en->Fill(P_e_3v.Theta() * TMath::RadToDeg());
            hPhi_e_en->Fill(P_e_3v.Phi() * TMath::RadToDeg());
            hP_e_en->Fill(P_e_3v.Mag());

            hVx_e_en->Fill(temp_vtx_e.X());
            hVy_e_en->Fill(temp_vtx_e.Y());
            hVz_e_en->Fill(temp_vtx_e.Z());

            hTheta_e_VS_Phi_e_en->Fill(P_e_3v.Phi() * TMath::RadToDeg(), P_e_3v.Theta() * TMath::RadToDeg());
            hTheta_e_VS_P_e_en->Fill(P_e_3v.Mag(), P_e_3v.Theta() * TMath::RadToDeg());
            hPhi_e_VS_P_e_en->Fill(P_e_3v.Mag(), P_e_3v.Phi() * TMath::RadToDeg());

            hTheta_n_en->Fill(P_N_3v.Theta() * TMath::RadToDeg());
            hPhi_n_en->Fill(P_N_3v.Phi() * TMath::RadToDeg());
            hP_n_en->Fill(P_N_3v.Mag());

            hVx_n_en->Fill(temp_vtx_N.X());
            hVy_n_en->Fill(temp_vtx_N.Y());
            hVz_n_en->Fill(temp_vtx_N.Z());

            hTheta_n_VS_Phi_n_en->Fill(P_N_3v.Phi() * TMath::RadToDeg(), P_N_3v.Theta() * TMath::RadToDeg());
            hTheta_n_VS_P_n_en->Fill(P_N_3v.Mag(), P_N_3v.Theta() * TMath::RadToDeg());
            hPhi_n_VS_P_n_en->Fill(P_N_3v.Mag(), P_N_3v.Phi() * TMath::RadToDeg());

            hP_e_VS_P_n_en->Fill(P_N_3v.Mag(), P_e_3v.Mag());
            hP_e_VS_Theta_n_en->Fill(P_N_3v.Theta() * TMath::RadToDeg(), P_e_3v.Mag());
            hP_e_VS_Phi_n_en->Fill(P_N_3v.Phi() * TMath::RadToDeg(), P_e_3v.Mag());

            hTheta_e_VS_P_n_en->Fill(P_N_3v.Mag(), P_e_3v.Theta() * TMath::RadToDeg());
            hTheta_e_VS_Theta_n_en->Fill(P_N_3v.Theta() * TMath::RadToDeg(), P_e_3v.Theta() * TMath::RadToDeg());
            hTheta_e_VS_Phi_n_en->Fill(P_N_3v.Phi() * TMath::RadToDeg(), P_e_3v.Theta() * TMath::RadToDeg());

            hPhi_e_VS_P_n_en->Fill(P_N_3v.Mag(), P_e_3v.Phi() * TMath::RadToDeg());
            hPhi_e_VS_Theta_n_en->Fill(P_N_3v.Theta() * TMath::RadToDeg(), P_e_3v.Phi() * TMath::RadToDeg());
            hPhi_e_VS_Phi_n_en->Fill(P_N_3v.Phi() * TMath::RadToDeg(), P_e_3v.Phi() * TMath::RadToDeg());
        }

        ++i;
    }
}

#pragma endregion
