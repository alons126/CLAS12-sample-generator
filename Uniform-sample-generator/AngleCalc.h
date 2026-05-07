#include <cmath>
#include <iostream>
#include <vector>

#include "ConfigTopDir.cpp"

double CalcdPhi(double dPhiTemp) {
    double dPhi;

    if (dPhiTemp > 180.0) {
        dPhi = dPhiTemp - 360.0;
    } else if (dPhiTemp < -180.0) {
        dPhi = dPhiTemp + 360.0;
    } else {
        dPhi = dPhiTemp;
    }

    return dPhi;
}

// Function to find the angle closest to 180 degrees away from phi_N
double getPhi_e(TString OutPutFolder, double phi_N) {
    double phi_e_offset = 0.;  // Electron phi_e offset due to the solenoid field

    string OutPutFolder0(OutPutFolder.Data());

    if (findSubstring(OutPutFolder0, "2070MeV")) {
        phi_e_offset = 16.;
    } else if (findSubstring(OutPutFolder0, "4029MeV")) {
        phi_e_offset = 7.;
    } else if (findSubstring(OutPutFolder0, "5986MeV")) {
        phi_e_offset = 5.;
    }

    std::vector<double> possible_angles = {-120, -60, 0, 60, 120, 180};

    // Calculate the target angle (180 degrees away from phi_N)
    double target_angle = CalcdPhi(phi_N + 180);

    // Find the closest possible angle
    double closest_angle = possible_angles[0];
    double min_diff = std::abs(CalcdPhi(target_angle - closest_angle));

    for (double angle : possible_angles) {
        double diff = std::abs(CalcdPhi(target_angle - angle));
        if (diff < min_diff) {
            min_diff = diff;
            closest_angle = angle;
        }
    }

    return closest_angle + phi_e_offset;
    // return closest_angle;
}