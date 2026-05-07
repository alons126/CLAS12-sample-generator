#include <cmath>

#if independent_tester
#include "addParticle.C"
#include "Histograms.h"
#endif

/* Uniform electron events (electron tester) */
void Generate_uniform_event_e_tester(TVector3 vtx, vector<TH1 *> TH1_hist_list, vector<TH2 *> TH2_hist_list, ofstream &OutFile, TString formatstring, TString outstring, TRandom3 &ran,
                                     int nEvents = 10000, double targP = 0., double beamP = 0., int interactN = 1, int beamType = 11, double beamE_in_lundfiles = -99,
                                     double Ebeam = -99, double weight = 1, double mass_e = 0.511e-3, double theta_e_min = 5., double theta_e_max = 40.)
{
    double pi = 3.14159265359;
    int i = 0;

    while (i < nEvents)
    {
        /* Create nEvents uniform (e,e'p) events and nEvents uniform (e,e'n) events
           LUND header for the event: */
        formatstring = "%i \t %i \t %i \t %.3f \t %.3f \t %i \t %.1f \t %i \t %i \t %.3f \n";
        outstring = Form(formatstring, 1, 1 /* A */, 1 /* Z */, targP, beamP, beamType, beamE_in_lundfiles, interactN, i, weight);
        // OutFile << outstring;

        /* Electron */
        TVector3 P_e_3v;
        double Theta_e = ran.Uniform(theta_e_min, theta_e_max); // Uniform Theta_e from theta_e_min to theta_e_max
        double Phi_e = ran.Uniform(-180., 180.);                // Uniform Phi_e from -180 to +180
        double P_e = Ebeam;                                     // P_e is Ebeam to test Theta_e and Phi_e acceptance
        P_e_3v.SetMagThetaPhi(P_e, Theta_e * TMath::DegToRad(), Phi_e * TMath::DegToRad());
        OutFile << outstring;
        OutFile << AddParticle(1, 11, P_e_3v, mass_e, vtx);

        hTheta_e_Tester_e->Fill(P_e_3v.Theta() * TMath::RadToDeg());
        hPhi_e_Tester_e->Fill(P_e_3v.Phi() * TMath::RadToDeg());
        hP_e_Tester_e->Fill(P_e_3v.Mag());

        hTheta_e_VS_Phi_e_Tester_e->Fill(P_e_3v.Phi() * TMath::RadToDeg(), P_e_3v.Theta() * TMath::RadToDeg());
        hTheta_e_VS_P_e_Tester_e->Fill(P_e_3v.Mag(), P_e_3v.Theta() * TMath::RadToDeg());
        hPhi_e_VS_P_e_Tester_e->Fill(P_e_3v.Mag(), P_e_3v.Phi() * TMath::RadToDeg());

        ++i;
    }
}
