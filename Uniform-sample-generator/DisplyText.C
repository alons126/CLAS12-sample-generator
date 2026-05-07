#include <iostream>
#include <iomanip>
#include <string>

#include "TString.h"
#include "BoolToString.C"

using namespace std;

void DisplyText(const string &Varname, const int &Space, const string &Var)
{
    string VarnameToDisply = Varname + ":";
    string VarToDisply = Var;
    int VarnameToDisplyLength = Varname.size();
    cout << "\033[33m" << VarnameToDisply << "\033[0m" <<std::setw(Space - VarnameToDisplyLength) << VarToDisply << "\n";
}

void DisplyText(const string &Varname, const int &Space, const int &Var)
{
    string VarnameToDisply = Varname + ":";
    string VarToDisply = to_string(Var);
    int VarnameToDisplyLength = Varname.size();
    cout << "\033[33m" << VarnameToDisply << "\033[0m" <<std::setw(Space - VarnameToDisplyLength) << VarToDisply << "\n";
}

void DisplyText(const string &Varname, const int &Space, const double &Var)
{
    string VarnameToDisply = Varname + ":";
    string VarToDisply = to_string(Var);
    int VarnameToDisplyLength = Varname.size();
    cout << "\033[33m" << VarnameToDisply << "\033[0m" <<std::setw(Space - VarnameToDisplyLength) << VarToDisply << "\n";
}

void DisplyText(const string &Varname, const int &Space, const bool &Var)
{
    string VarnameToDisply = Varname + ":";
    string VarToDisply = BoolToString(Var);
    int VarnameToDisplyLength = Varname.size();
    cout << "\033[33m" << VarnameToDisply << "\033[0m" <<std::setw(Space - VarnameToDisplyLength) << VarToDisply << "\n";
}

void DisplyText(const string &Varname, const int &Space, const TString &Var)
{
    string VarnameToDisply = Varname + ":";
    string VarToDisply = Var.Data();
    int VarnameToDisplyLength = Varname.size();
    cout << "\033[33m" << VarnameToDisply << "\033[0m" <<std::setw(Space - VarnameToDisplyLength) << VarToDisply << "\n";
}
