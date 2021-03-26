// aeromatic.cpp -- main command prompt uility
//
// Based on Aeromatic2 PHP code by David P. Culp
// Started June 2003
//
// C++-ified and modulized by Erik Hofman, started October 2015.
//
// Copyright (C) 2003, David P. Culp <davidculp2@comcast.net>
// Copyright (C) 2015 Erik Hofman <erik@ehofman.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA

#if HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(_MSC_VER)
#  include <float.h>
#elif defined(__GNUC__) && !defined(sgi)
#  include <fenv.h>
#endif

#include <string.h>
#include <iostream>
#include <fstream>
#include <iomanip>

#include <Systems/Systems.h>
#include "Aircraft.h"
#include "types.h"

using namespace std;

char *
getCommandLineOption(int argc, char **argv, char *option)
{
    int slen = strlen(option);
    char *rv = 0;
    int i;

    for (i=0; i<argc; i++)
    {
        if (strncmp(argv[i], option, slen) == 0)
        {
            rv = (char*)"";
            i++;
            if (i<argc) rv = argv[i];
        }
    }

    return rv;
}

void ask(istream& in, ofstream& log, Aeromatic::Param* param)
{
    string input;

    cout << param->name();
    cout << " [" << param->get_units() << "]";
    cout << " (" << param->get() << ")";
    cout << ": ";

    unsigned options = param->no_options();
    for(unsigned j=0; j<options; ++j)
    {
        cout << endl;
        cout << " " << j << ": ";
        cout << param->get_option(j);
    }
    if (options) cout << endl;

    getline(in, input);
    if (!input.empty() && input[0] != ' ')
    {
        if (input == "?" || input == "h" || input == "help")
        {
            cout << param->help() << endl;
            getline(in, input);
        }
        if (!input.empty() && input[0] != ' ') {
            param->set(input);
        }
    }
    if (log.is_open()) {
        log << input;
        if (param->get_type() != Aeromatic::PARAM_STRING &&
            param->get_type() != Aeromatic::PARAM_BOOL) {
            log << std::setw(32-input.size()) << "; " << param->name();
        }
        log << endl;
    }
}

struct noop {
    void operator()(...) const {}
};

void help()
{
    printf("AeromatiC++ version " AEROMATIC_VERSION_STR "\n\n");
    printf("Usage: aeromatic [options]\n");
    printf("A tool to generate a JSBSim Flight Dynamics Model using just a few paremeters.\n");

    printf("\nOptions:\n");
    printf(" -l, --log <file>\t\tLog the output to a log file.\n");
    printf(" -i, --input <file>\t\tRead the input parameters from a log file.\n");
    printf(" -h, --help\t\t\tprint this message and exit\n");

    printf("\nWhen run without any parameters the program will generate an FDM and exit.\n");

    printf("\n");
    exit(-1);
}

int main(int argc, char *argv[])
{
    Aeromatic::Aeromatic aeromatic;
    ofstream log;
    ifstream in;

    if (getCommandLineOption(argc, argv, (char*)"-h") != NULL ||
        getCommandLineOption(argc, argv, (char*)"--help") != NULL) {
        help();
    }


    char *file = getCommandLineOption(argc, argv, (char*)"-l");
    if (file)
    {
        log.open(file);
        if (log.fail() || log.bad())
        {
            cerr << "Failed to open logfile: " << file << endl;
            log.close();
        }
    }

    file = getCommandLineOption(argc, argv, (char*)"-i");
    if (file)
    {
        in.open(file);
        if (in.fail() || in.bad())
        {
            cerr << "Failed to open parameter file: " << file << endl;
            in.close();
        }
    }

    if (!in.is_open())
    {
        in.copyfmt(cin);
        in.clear(cin.rdstate());
        in.basic_ios<char>::rdbuf(cin.rdbuf());
    }

    cout << endl;
    cout << "** AeromatiC++ version " << AEROMATIC_VERSION_STR << endl;
    cout << "Aeromatic is a JSBSim configuration file generation utility." << endl;
    cout << "Please enter aircraft data when prompted." << endl << endl;
    cout << "You can always enter 'h' to get verbose help" << endl << endl;

    cout << "** General Information **" << endl << endl;
    for (unsigned i=0; i<aeromatic._general.size(); ++i) {
        ask(in, log, aeromatic._general[i]);
    }
    cout << endl;

    cout << "** Weight and Balance **" << endl << endl;
    for (unsigned i=0; i<aeromatic._weight_balance.size(); ++i) {
        ask(in, log, aeromatic._weight_balance[i]);
    }  
    cout << endl;

    cout << "** Geometry **" << endl << endl;
    for (unsigned i=0; i<aeromatic._geometry.size(); ++i) {
        ask(in, log, aeromatic._geometry[i]);
    }
    cout << endl;

    cout << "** Systems **" << endl << endl;
    const vector<Aeromatic::System*> systems = aeromatic.get_systems();
    for (unsigned i=0; i<systems.size(); ++i)
    {
        Aeromatic::System* system = systems[i];

//      if (system->_inputs.size()) {
//          cout << "  ** " << system->get_description() << endl << endl;
//      }

        Aeromatic::Param* param;
        system->param_reset();
        while ((param = system->param_next()) != 0) {
            ask(in, log, param);
        }
        cout << endl;
    }

    if (aeromatic.fdm())
    {
        cout << "We're finished, the files have been written to: " << endl;
        cout << aeromatic._dir;
    }
    else {
        cout << "Error: Unable to write files to: " << endl;
        cout << aeromatic._dir;
    }
    cout << endl << endl;

    cout << "Press enter to continue." << endl;
    string input;
    getline(cin, input);

    if (in.is_open()) {
        in.close();
    }

    if (log.is_open()) {
       log.close();
    }
}
