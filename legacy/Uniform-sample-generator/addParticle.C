#include <fstream>
#include <iostream>

#include "TFile.h"
#include "TString.h"
#include "TVector3.h"

using namespace std;

TString AddParticle(int part_idx, int pid, TVector3 momentum, double mass, TVector3 vtx, int active = 1) {
    // LUND info for each particle in the event
    TString formatstring = "%i \t %.3f \t %i \t %i \t %i \t %i \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \n";
    double energy = sqrt(mass * mass + momentum.Mag2());
    TString outstring = Form(formatstring, part_idx, 0.0, active, pid, 0, 0, momentum.Px(), momentum.Py(), momentum.Pz(), energy, mass, vtx.X(), vtx.Y(), vtx.Z());

    return outstring;
}
