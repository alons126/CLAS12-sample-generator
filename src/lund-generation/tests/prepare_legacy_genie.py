#
# Created by Alon Sportes on 14/09/2026.
#

"""Generate a maintained test adapter around the archived converter.

Purpose:
    Read the external archive and write a separate build-tree reference with redirected output setup.

Workflow:
    CTest supplies paths and fixtures; assertions or exit codes report failures to the test runner.

Notes:
    Test fixtures are isolated; external and legacy sources are read-only.
"""

from pathlib import Path
import sys
source, destination = map(Path, sys.argv[1:])
text = source.read_text()

# Redirect only external includes and output setup. Physics and rollover code are unchanged.
# Test execution ------------------------------------------------
# region Execution
for relative in ['../include/targets.h', '../framework/namespaces/general_utilities/utilities.h', '../framework/classes/DSCuts/DSCuts.h']:
    text = text.replace('"'+relative+'"', '"'+str((source.parent/relative).resolve())+'"')

start = text.index('    TString OutputFileBase =')
end = text.index('    // Make lundfiles directory:', start)
text = text[:start] + '''    TString OutputFileBase = legacy_output.c_str();
    TString OutputTopDir = legacy_output.c_str();
    std::filesystem::create_directories(legacy_output);

''' + text[end:]
text = text.replace('system((', 'legacy_mkdir((')
text = text.replace('    // Create a canvas', '    TFile reference_hist((legacy_output + "/histograms.root").c_str(), "RECREATE");\n    theta_e_VS_phi_e->Write();\n    reference_hist.Close();\n    // Create a canvas')
prelude = '''#include <filesystem>
#include <map>
#include <vector>
#include <string>
#include <stdexcept>
#include <TROOT.h>
using namespace std;
std::string legacy_output;
int legacy_mkdir(const char* command) {
    std::string text(command);
    if (text.rfind("mkdir -p " + legacy_output + "/", 0) != 0) throw std::runtime_error("Unexpected legacy filesystem command");
    std::filesystem::create_directories(text.substr(9));
    return 0;
}
'''
postlude = '''
int main(int argc, char** argv) {
    if (argc != 7) return 2;
    gROOT->SetBatch(true);
    legacy_output = std::filesystem::absolute(argv[2]).string();
    if (std::filesystem::exists(legacy_output)) throw std::runtime_error("Expected new legacy test directory");
    ran.SetSeed(12345);
    GENIE_to_LUND_converter(argv[1], std::stoi(argv[3]), argv[4], std::stoi(argv[5]), std::stoi(argv[6]));
}
'''

# Macro in restored utility header only bridges the legacy namespace; do not export it to main.
destination.write_text(prelude+text+'\n#undef targets\n'+postlude)

# endregion
