// zlib input file stream wrapper.
//
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
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 51
// Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//

#include <simgear/compiler.h>
#include <string>

#include "sgstream.hxx"

#include <simgear/misc/sg_path.hxx>

using std::istream;
using std::ostream;

sg_ifstream::sg_ifstream(const SGPath& path, std::ios::openmode io_mode)
{
#if defined(_MSC_VER)
    std::wstring ps = path.wstr();
#else
    std::string ps = path.local8BitStr();
#endif
    std::ifstream::open(ps.c_str(), io_mode);
}

void sg_ifstream::open( const SGPath& name, std::ios::openmode io_mode )
{
#if defined(_MSC_VER)
    std::wstring ps = name.wstr();
#else
    std::string ps = name.local8BitStr();
#endif
    std::ifstream::open(ps.c_str(), io_mode);
}

sg_ofstream::sg_ofstream(const SGPath& path, std::ios::openmode io_mode)
{
#if defined(_MSC_VER)
    std::wstring ps = path.wstr();
#else
    std::string ps = path.local8BitStr();
#endif
    std::ofstream::open(ps.c_str(), io_mode);
}

void sg_ofstream::open( const SGPath& name, std::ios::openmode io_mode )
{
#if defined(_MSC_VER)
    std::wstring ps = name.wstr();
#else
    std::string ps = name.local8BitStr();
#endif
    std::ofstream::open(ps.c_str(), io_mode);
}
