#include <unistd.h>

#include <iostream>

#include "ConfigPrefix.cpp"
#include "ConfigTopDir.cpp"
#include "DisplyText.C"
#include "Generate_uniform_event.C"
#include "Histograms.cpp"
#include "TList.h"
#include "TObjString.h"
// #include "targets.h"

/* root CodeRun.cpp -q -b */
/* LUND format: https://gemc.jlab.org/gemc/html/documentation/generator/lund.html */

void Uniform_sample_generator(const bool gen_1e_events, const bool gen_ep_events, const bool gen_en_events, double Ebeam = 5.98636,
                              //   double Ebeam = 4.02962,
                              //   double Ebeam = 2.07052,
                              TString OutPutFolder = "/lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/Uniform_e-p-n_samples/5986MeV/OutPut/",
                              //   TString OutPutFolder = "/lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/Uniform_e-p-n_samples/4029MeV/OutPut/",
                              //   TString OutPutFolder = "/lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/Uniform_e-p-n_samples/2070MeV/OutPut/",
                              // int nFiles = 10, int nEvents = 10000,
                              //   int nFiles = 500, int nEvents = 10000,
                              //   int nFiles = 1000, int nEvents = 10000,
                              //   int nFiles = 2500, int nEvents = 10000,
                              int nFiles = 25000, int nEvents = 10000, double theta_e_min = 5., double theta_e_max = 40., double theta_p_min = 5., double theta_p_max = 45.,
                              double theta_n_min = 5., double theta_n_max = 35.) {
    bool LimitedLocalGeneration = false;

    // -------------------------------------------------------------------------------------------------
    cout << "\033[33m" << "\n\n\n===========================================================================\n\033[0m";
    cout << "\033[33m" << setw(50) << "Uniform sample generator\n\033[0m";
    cout << "\033[33m" << "===========================================================================\n\n\033[0m";

    if (gen_1e_events && gen_ep_events && gen_en_events) {
        cout << "\033[33m" << "Uniform_sample_generator:\033[0m change gen_1e_events or gen_ep_events or gen_en_events!... Exiting" << endl, exit(0);
    }

    if (gen_1e_events && gen_ep_events) { cout << "\033[33m" << "Uniform_sample_generator:\033[0m change gen_1e_events or gen_ep_events!... Exiting" << endl, exit(0); }

    if (gen_1e_events && gen_en_events) { cout << "\033[33m" << "Uniform_sample_generator:\033[0m change gen_1e_events or gen_en_events!... Exiting" << endl, exit(0); }

    if (gen_en_events && gen_ep_events) { cout << "\033[33m" << "Uniform_sample_generator:\033[0m change gen_en_events or gen_ep_events!... Exiting" << endl, exit(0); }

    TString OutputFileNamePrefix = ConfigPrefix(gen_1e_events, gen_ep_events, gen_en_events, Ebeam);
    OutPutFolder = ConfigTopDir(gen_1e_events, gen_ep_events, gen_en_events, Ebeam, OutPutFolder);  // reconfigure OutPutFolder according to working directory (ifarm or local)

    string CurrentDir = GetCurrentDirectory();
    if (LimitedLocalGeneration && findSubstring(CurrentDir, "Users/alon/Projects/Uniform-sample-generator")) { nFiles = 10; }

    bool GenerateLundFiles = true;
    int DisplaySpace = 74;

    DisplyText("OutputFileNamePrefix", DisplaySpace, OutputFileNamePrefix);
    DisplyText("nFiles", DisplaySpace, nFiles);
    DisplyText("nEvents", DisplaySpace, nEvents);
    DisplyText("Beam energy [GeV]", DisplaySpace, Ebeam);

    DisplyText("GenerateLundFiles", DisplaySpace, GenerateLundFiles);

    cout << "\n";

    TRandom3 ran(0);
    const int nParticles = 2;
    double mass_e = 0.511e-3, mass_p = 0.938272, mass_n = 0.93957;
    DisplyText("nParticles", DisplaySpace, nParticles);
    DisplyText("mass_e [GeV]", DisplaySpace, mass_e);
    DisplyText("mass_p [GeV]", DisplaySpace, mass_p);
    DisplyText("mass_n [GeV]", DisplaySpace, mass_n);

    cout << "\n";

    string OutPutFolder0(OutPutFolder.Data());
    DisplyText("OutPutFolder", DisplaySpace, OutPutFolder);

    if (findSubstring(OutPutFolder0, "volatile") && findSubstring(CurrentDir, "Users/alon/Projects/Uniform-sample-generator")) {
        std::cerr << "\033[31m" << "Error!" << "\033[0m"
                  << "Output directory:\n\t" << OutPutFolder0 << "\nCurrent directory:\n\t" << CurrentDir << "\nDo not match the same running system! Aborting..." << endl;
        exit(0);
    }

    TString lundPath = OutPutFolder + "lundfiles", mchipoPath = OutPutFolder + "mchipo";
    TString reconhipoPath = OutPutFolder + "reconhipo", rootfilesPath = OutPutFolder + "rootfiles";
    string lundPath0(lundPath.Data()), mchipoPath0(mchipoPath.Data());
    string reconhipoPath0(reconhipoPath.Data()), rootfilesPath0(rootfilesPath.Data());
    DisplyText("lundPath", DisplaySpace, lundPath);
    DisplyText("mchipoPath", DisplaySpace, mchipoPath);
    DisplyText("reconhipoPath", DisplaySpace, reconhipoPath);
    DisplyText("rootfilesPath", DisplaySpace, rootfilesPath);

    TString MonitoringPlotsPath = OutPutFolder + "MonitoringPlotsPath/";
    string MonitoringPlotsPath0(MonitoringPlotsPath.Data());
    DisplyText("MonitoringPlotsPath", DisplaySpace, MonitoringPlotsPath);

    TList* plots = new TList();
    string OutputFileNamePrefix0(OutputFileNamePrefix.Data());
    string listName = OutPutFolder0 + OutputFileNamePrefix0 + "_plots.root";
    const char* TListName = listName.c_str();
    DisplyText("Plot list path", DisplaySpace, listName);

    cout << "\n";

    // Not useful usually
    double targP = 0.;  // polarization
    double beamP = 0.;  // polarization
    int interactN = 1;
    int beamType = 11;
    DisplyText("targP", DisplaySpace, targP);
    DisplyText("beamP", DisplaySpace, beamP);
    DisplyText("interactN", DisplaySpace, interactN);
    DisplyText("beamType", DisplaySpace, beamType);

    double beamE_in_lundfiles = Ebeam;  // GeV
    DisplyText("beamE_in_lundfiles", DisplaySpace, beamE_in_lundfiles);

    double weight = 1;
    DisplyText("weight", DisplaySpace, weight);

    cout << "\033[33m" << "\nCreating plot directories...\n\033[0m";

    system(("rm -r " + OutPutFolder0).c_str());     // Delete old output folder
    system(("mkdir -p " + OutPutFolder0).c_str());  // Make new output folder

    system(("mkdir -p " + lundPath0).c_str());       // Make new lundfiles folder
    system(("mkdir -p " + mchipoPath0).c_str());     // Make new mchipo folder
    system(("mkdir -p " + reconhipoPath0).c_str());  // Make new reconhipo folder
    system(("mkdir -p " + rootfilesPath0).c_str());  // Make new rootfiles folder

    system(("mkdir -p " + MonitoringPlotsPath0).c_str());  // Make new monitoring plots folder

    /* Add particles in event below */
    string target = "Ar";
    // string target = "1-foil-small";
    // string target = "1-foil";
    TVector3 vtx;
    // TVector3 vtx = randomVertex(target);

    if (GenerateLundFiles) {
        InitHistograms(gen_1e_events, gen_ep_events, gen_en_events, Ebeam);

        cout << "\033[33m" << "\nGenerating lund files...\n\n\033[0m";

        for (int iFiles = 1; iFiles < nFiles + 1; iFiles++) {
            TString OutFileName = Form("%s/%s_%d.txt", lundPath.Data(), OutputFileNamePrefix.Data(), iFiles);
            cout << "\033[33m" << "OutFileName:\033[0m " << std::setw(49) << OutFileName << "\n";

            ofstream OutFile;
            OutFile.open(OutFileName);
            if (!OutFile.is_open()) { cout << "\033[33m" << "Output file cannot be created\033[0m" << endl; }

            TString formatstring, outstring;

            if (gen_1e_events) {
                Generate_uniform_event(
                    target, TH1_hist_list_1e, TH2_hist_list_1e, OutFile, formatstring, outstring, ran, nEvents, targP, beamP, interactN, beamType,
                    // Generate_uniform_event(vtx, TH1_hist_list_1e, TH2_hist_list_1e, OutFile, formatstring, outstring, ran, nEvents, targP, beamP, interactN, beamType,
                    beamE_in_lundfiles, Ebeam, weight, mass_e, theta_e_min, theta_e_max);
            }

            if (gen_ep_events) {
                Generate_uniform_event(
                    OutPutFolder, target, TH1_hist_list_ep, TH2_hist_list_ep, OutFile, formatstring, outstring, ran, 2212, nEvents, nParticles, targP, beamP,
                    // Generate_uniform_event(OutPutFolder, vtx, TH1_hist_list_ep, TH2_hist_list_ep, OutFile, formatstring, outstring, ran, 2212, nEvents, nParticles, targP, beamP,
                    interactN, beamType, beamE_in_lundfiles, Ebeam, weight, mass_e, mass_p, theta_e_min, theta_e_max, theta_p_min, theta_p_max);
            }

            if (gen_en_events) {
                Generate_uniform_event(
                    OutPutFolder, target, TH1_hist_list_en, TH2_hist_list_en, OutFile, formatstring, outstring, ran, 2112, nEvents, nParticles, targP, beamP,
                    // Generate_uniform_event(OutPutFolder, vtx, TH1_hist_list_en, TH2_hist_list_en, OutFile, formatstring, outstring, ran, 2112, nEvents, nParticles, targP, beamP,
                    interactN, beamType, beamE_in_lundfiles, Ebeam, weight, mass_e, mass_n, theta_e_min, theta_e_max, theta_n_min, theta_n_max);
            }

            OutFile.close();
        }

        cout << "\033[33m" << "\nPlotting and saving monitoring plots\n\n\033[0m";

        TCanvas* c1 = new TCanvas("canvas", "canvas", 1000, 750);  // normal res
        c1->SetGrid(), c1->SetBottomMargin(0.14), c1->SetLeftMargin(0.16), c1->SetRightMargin(0.12), c1->cd();

        int num = 0;

        if (gen_1e_events) {
            string pdfFile_1e_0 = MonitoringPlotsPath0 + "/Uniform_1e_plots_" + ConfigBeamE(Ebeam) + ".pdf";
            const char* pdfFile_1e = pdfFile_1e_0.c_str();
            cout << pdfFile_1e_0 << "\n\n";
            cout << pdfFile_1e << "\n\n";
            c1->Print(Form("%s[", pdfFile_1e));  // Open the PDF file

            for (int i = 0; i < TH1_hist_list_1e.size(); i++) {
                TH1_hist_list_1e[i]->Sumw2();
                TH1_hist_list_1e[i]->GetXaxis()->CenterTitle();
                TH1_hist_list_1e[i]->GetXaxis()->SetTitleSize(0.06);
                TH1_hist_list_1e[i]->GetXaxis()->SetLabelSize(0.0425);
                TH1_hist_list_1e[i]->GetYaxis()->SetTitle("Number of events");
                TH1_hist_list_1e[i]->GetYaxis()->CenterTitle();
                TH1_hist_list_1e[i]->GetYaxis()->SetTitleSize(0.06);
                TH1_hist_list_1e[i]->GetYaxis()->SetLabelSize(0.0425);
                plots->Add(TH1_hist_list_1e[i]);

                TH1_hist_list_1e[i]->Draw();
                string SavePath = MonitoringPlotsPath0 + to_string(num + 1) + "_" + TH1_hist_list_1e[i]->GetName() + ".png";
                c1->SaveAs(SavePath.c_str());
                c1->Print(pdfFile_1e);  // Save the current canvas (histogram) to the PDF
                c1->Clear();
                ++num;
            }

            for (int i = 0; i < TH2_hist_list_1e.size(); i++) {
                TH2_hist_list_1e[i]->GetXaxis()->CenterTitle();
                TH2_hist_list_1e[i]->GetXaxis()->SetTitleSize(0.06);
                TH2_hist_list_1e[i]->GetXaxis()->SetLabelSize(0.0425);
                TH2_hist_list_1e[i]->GetYaxis()->CenterTitle();
                TH2_hist_list_1e[i]->GetYaxis()->SetTitleSize(0.06);
                TH2_hist_list_1e[i]->GetYaxis()->SetLabelSize(0.0425);
                plots->Add(TH2_hist_list_1e[i]);

                TH2_hist_list_1e[i]->Draw("colz");
                string SavePath = MonitoringPlotsPath0 + to_string(num + 1) + "_" + TH2_hist_list_1e[i]->GetName() + ".png";
                c1->SaveAs(SavePath.c_str());
                c1->Print(pdfFile_1e);  // Save the current canvas (histogram) to the PDF
                c1->Clear();
                ++num;
            }

            c1->Print(Form("%s]", pdfFile_1e));  // Close the PDF file

            c1->Clear();
            delete c1;
            gDirectory->Clear();
            TH1_hist_list_1e.clear();
            TH2_hist_list_1e.clear();
        }

        if (gen_ep_events) {
            string pdfFile_ep_0 = MonitoringPlotsPath0 + "/Uniform_ep_plots_" + ConfigBeamE(Ebeam) + ".pdf";
            const char* pdfFile_ep = pdfFile_ep_0.c_str();
            cout << pdfFile_ep_0 << "\n\n";
            cout << pdfFile_ep << "\n\n";
            c1->Print(Form("%s[", pdfFile_ep));  // Open the PDF file

            for (int i = 0; i < TH1_hist_list_ep.size(); i++) {
                TH1_hist_list_ep[i]->Sumw2();
                TH1_hist_list_ep[i]->GetXaxis()->CenterTitle();
                TH1_hist_list_ep[i]->GetXaxis()->SetTitleSize(0.06);
                TH1_hist_list_ep[i]->GetXaxis()->SetLabelSize(0.0425);
                TH1_hist_list_ep[i]->GetYaxis()->SetTitle("Number of events");
                TH1_hist_list_ep[i]->GetYaxis()->CenterTitle();
                TH1_hist_list_ep[i]->GetYaxis()->SetTitleSize(0.06);
                TH1_hist_list_ep[i]->GetYaxis()->SetLabelSize(0.0425);
                plots->Add(TH1_hist_list_ep[i]);

                TH1_hist_list_ep[i]->Draw();
                string SavePath = MonitoringPlotsPath0 + to_string(num + 1) + "_" + TH1_hist_list_ep[i]->GetName() + ".png";
                c1->SaveAs(SavePath.c_str());
                c1->Print(pdfFile_ep);  // Save the current canvas (histogram) to the PDF
                c1->Clear();
                ++num;
            }

            for (int i = 0; i < TH2_hist_list_ep.size(); i++) {
                TH2_hist_list_ep[i]->GetXaxis()->CenterTitle();
                TH2_hist_list_ep[i]->GetXaxis()->SetTitleSize(0.06);
                TH2_hist_list_ep[i]->GetXaxis()->SetLabelSize(0.0425);
                TH2_hist_list_ep[i]->GetYaxis()->CenterTitle();
                TH2_hist_list_ep[i]->GetYaxis()->SetTitleSize(0.06);
                TH2_hist_list_ep[i]->GetYaxis()->SetLabelSize(0.0425);
                plots->Add(TH2_hist_list_ep[i]);

                TH2_hist_list_ep[i]->Draw("colz");
                string SavePath = MonitoringPlotsPath0 + to_string(num + 1) + "_" + TH2_hist_list_ep[i]->GetName() + ".png";
                c1->SaveAs(SavePath.c_str());
                c1->Print(pdfFile_ep);  // Save the current canvas (histogram) to the PDF
                c1->Clear();
                ++num;
            }

            c1->Print(Form("%s]", pdfFile_ep));  // Close the PDF file

            c1->Clear();
            delete c1;
            gDirectory->Clear();
            TH1_hist_list_ep.clear();
            TH2_hist_list_ep.clear();
        }

        if (gen_en_events) {
            string pdfFile_en_0 = MonitoringPlotsPath0 + "/Uniform_en_plots_" + ConfigBeamE(Ebeam) + ".pdf";
            const char* pdfFile_en = pdfFile_en_0.c_str();
            cout << pdfFile_en_0 << "\n\n";
            cout << pdfFile_en << "\n\n";
            c1->Print(Form("%s[", pdfFile_en));  // Open the PDF file

            for (int i = 0; i < TH1_hist_list_en.size(); i++) {
                TH1_hist_list_en[i]->Sumw2();
                TH1_hist_list_en[i]->GetXaxis()->CenterTitle();
                TH1_hist_list_en[i]->GetXaxis()->SetTitleSize(0.06);
                TH1_hist_list_en[i]->GetXaxis()->SetLabelSize(0.0425);
                TH1_hist_list_en[i]->GetYaxis()->SetTitle("Number of events");
                TH1_hist_list_en[i]->GetYaxis()->CenterTitle();
                TH1_hist_list_en[i]->GetYaxis()->SetTitleSize(0.06);
                TH1_hist_list_en[i]->GetYaxis()->SetLabelSize(0.0425);
                plots->Add(TH1_hist_list_en[i]);

                TH1_hist_list_en[i]->Draw();
                string SavePath = MonitoringPlotsPath0 + to_string(num + 1) + "_" + TH1_hist_list_en[i]->GetName() + ".png";
                c1->SaveAs(SavePath.c_str());
                c1->Print(pdfFile_en);  // Save the current canvas (histogram) to the PDF
                c1->Clear();
                ++num;
            }

            for (int i = 0; i < TH2_hist_list_en.size(); i++) {
                TH2_hist_list_en[i]->GetXaxis()->CenterTitle();
                TH2_hist_list_en[i]->GetXaxis()->SetTitleSize(0.06);
                TH2_hist_list_en[i]->GetXaxis()->SetLabelSize(0.0425);
                TH2_hist_list_en[i]->GetYaxis()->CenterTitle();
                TH2_hist_list_en[i]->GetYaxis()->SetTitleSize(0.06);
                TH2_hist_list_en[i]->GetYaxis()->SetLabelSize(0.0425);
                plots->Add(TH2_hist_list_en[i]);

                TH2_hist_list_en[i]->Draw("colz");
                string SavePath = MonitoringPlotsPath0 + to_string(num + 1) + "_" + TH2_hist_list_en[i]->GetName() + ".png";
                c1->SaveAs(SavePath.c_str());
                c1->Print(pdfFile_en);  // Save the current canvas (histogram) to the PDF
                c1->Clear();
                ++num;
            }

            c1->Print(Form("%s]", pdfFile_en));  // Close the PDF file

            c1->Clear();
            delete c1;
            gDirectory->Clear();
            TH1_hist_list_en.clear();
            TH2_hist_list_en.clear();
        }

        TFile* plots_fout = new TFile(TListName, "recreate");
        plots_fout->cd();
        plots->Write();
        plots_fout->Write();
        plots_fout->Close();
    } else {
        cout << "\033[33m" << "\nLund files generation disabled.\n\033[0m";
    }
}
