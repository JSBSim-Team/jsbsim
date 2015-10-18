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
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <iomanip>

#include <Systems/Systems.h>
#include "Aircraft.h"
#include "types.h"

using namespace std;

int main(int argc, char *argv[])
{
    Aeromatic::Aeromatic aeromatic;
    string input;

    cout << "** General Information **" << endl << endl;
    for (unsigned i=0; i<aeromatic._general.size(); ++i)
    {
        cout << aeromatic._general[i]->name();
        cout << " (" << aeromatic._general[i]->get() << ")";
        cout << ": ";

        unsigned options = aeromatic._general[i]->no_options();
        for(unsigned j=0; j<options; ++j)
        {
            cout << endl;
            cout << " " << j << ": ";
            cout << aeromatic._general[i]->get_option(j);
        }
        if (options) cout << endl;

        getline(cin, input);
        if (!input.empty()) {
            aeromatic._general[i]->set(input);
        }
    };
    cout << endl;

    cout << "** Weight and Balance **" << endl << endl;
    for (unsigned i=0; i<aeromatic._weight_balance.size(); ++i)
    {
        cout << aeromatic._weight_balance[i]->name();
        cout << " (" << aeromatic._weight_balance[i]->get() << ")";
        cout << ": ";

        unsigned options = aeromatic._weight_balance[i]->no_options();
        for(unsigned j=0; j<options; ++j)
        {
            cout << endl;
            cout << " " << j << ": ";
            cout << aeromatic._weight_balance[i]->get_option(j);
        }
        if (options) cout << endl;

        getline(cin, input);
        if (!input.empty()) {
            aeromatic._weight_balance[i]->set(input);
        }
    };  
    cout << endl;

    cout << "** Geometry **" << endl << endl;
    for (unsigned i=0; i<aeromatic._geometry.size(); ++i)
    {
        cout << aeromatic._geometry[i]->name();
        cout << " (" << aeromatic._geometry[i]->get() << ")";
        cout << ": ";

        unsigned options = aeromatic._geometry[i]->no_options();
        for(unsigned j=0; j<options; ++j)
        {
            cout << endl;
            cout << " " << j << ": ";
            cout << aeromatic._geometry[i]->get_option(j);
        }
        if (options) cout << endl;

        getline(cin, input);
        if (!input.empty()) {
            aeromatic._geometry[i]->set(input);
        }
    };  
    cout << endl;

    cout << "** Systems **" << endl << endl;
    const std::vector<Aeromatic::System*> systems = aeromatic.get_systems();
    for (unsigned i=0; i<systems.size(); ++i)
    {
        Aeromatic::System* system = systems[i];

//      if (system->_inputs.size()) {
//          cout << "  ** " << system->get_description() << endl << endl;
//      }

        Aeromatic::Param* param;
        system->param_reset();
        while ((param = system->param_next()) != 0)
        {
            cout << param->name();
            cout << " (" << param->get() << ")";
            cout << ": ";

            unsigned options = param->no_options();
            for (unsigned k=0; k<options; ++k)
            {
                cout << endl;
                cout << " " << k << ": ";
                cout << param->get_option(k);
            }
            if (options) cout << endl;

            getline(cin, input);
            if (!input.empty()) {
                param->set(input);
            }
        }
        cout << endl;
    }

    aeromatic.fdm();
}
