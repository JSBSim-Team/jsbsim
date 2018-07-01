/**
 * \file sgstream.hxx
 * input file stream wrapper.
 */

// Written by Bernie Bright, 1998
//
// Copyright (C) 1998  Bernie Bright - bbright@c031.aone.net.au
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//


#ifndef _SGSTREAM_HXX
#define _SGSTREAM_HXX

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>

#include <iostream>
#include <fstream>

#include <string>

class SGPath;

class sg_ifstream : public std::ifstream
{
public:
    sg_ifstream() {}

    sg_ifstream(const SGPath& path, std::ios::openmode io_mode = std::ios::in | std::ios::binary);

    void open( const SGPath& name,
	       std::ios::openmode io_mode = std::ios::in|std::ios::binary );
};

class sg_ofstream : public std::ofstream
{
public:
    sg_ofstream() { }
    sg_ofstream(const SGPath& path, std::ios::openmode io_mode = std::ios::out | std::ios::binary);

    void open( const SGPath& name,
	       std::ios::openmode io_mode = std::ios::out|std::ios::binary );
};

#endif /* _SGSTREAM_HXX */
