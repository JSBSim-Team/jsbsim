/**
 * \file sg_path.hxx
 * Routines to abstract out path separator differences between MacOS
 * and the rest of the world.
 */

// Written by Curtis L. Olson, started April 1999.
//
// Copyright (C) 1999  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _SG_PATH_HXX
#define _SG_PATH_HXX

#include <sys/types.h>
#include <string>
#include <ctime>
#include <vector>

#include "simgear/compiler.h"
#include "JSBSim_API.h"

#ifdef _MSC_VER
  typedef int mode_t;
#endif

typedef std::vector<std::string> string_list;
typedef string_list::iterator string_list_iterator;

/**
 * A class to hide path separator difference across platforms and assist
 * in managing file system path names.
 *
 * Paths can be input in any platform format and will be converted
 * automatically to the proper format.
 */

class JSBSIM_API SGPath {

public:

    // OS-dependent separator used in paths lists
    static const char pathListSep;

    struct Permissions
    {
      bool read : 1;
      bool write : 1;
    };
    typedef Permissions (*PermissionChecker)(const SGPath&);

    /** Default constructor */
    explicit SGPath(PermissionChecker validator = NULL);

    /** Copy contructor */
    SGPath(const SGPath& p);
    
    SGPath& operator=(const SGPath& p);

    /**
     * Construct a path based on the starting path provided.
     * @param p initial path
     */
    explicit SGPath( const std::string& p, PermissionChecker validator = NULL );

    explicit SGPath(const std::wstring& p, PermissionChecker validator = NULL);

    /**
     * Construct a path based on the starting path provided and a relative subpath
     * @param p initial path
     * @param r relative subpath
     */
    SGPath( const SGPath& p,
            const std::string& r,
            PermissionChecker validator = NULL );

    /** Destructor */
    ~SGPath();

    /**
     * Set path to a new value
     * @param p new path
     */
    void set( const std::string& p );
    SGPath& operator= ( const char* p ) { this->set(p); return *this; }

    bool operator==(const SGPath& other) const;
    bool operator!=(const SGPath& other) const;

    void setPermissionChecker(PermissionChecker validator);
    PermissionChecker getPermissionChecker() const;

    /**
     * Set if file information (exists, type, mod-time) is cached or
     * retrieved each time it is queried. Caching is enabled by default
     */
    void set_cached(bool cached);
    
    /**
     * Append another piece to the existing path.  Inserts a path
     * separator between the existing component and the new component.
     * @param p additional path component */
    void append( const std::string& p );

    /**
     * Get a copy of this path with another piece appended.
     *
     * @param p additional path component
     */
    SGPath operator/( const std::string& p ) const;

    /**
     * Append a new piece to the existing path.  Inserts a search path
     * separator to the existing path and the new patch component.
     * @param p additional path component */
    void add( const std::string& p );

    /**
     * Concatenate a string to the end of the path without inserting a
     * path separator.
     * @param p additional path suffix
     */
    void concat( const std::string& p );

    /**
     * Returns a path with the absolute pathname that names the same file, whose
     * resolution does not involve '.', '..', or symbolic links.
     */
    SGPath realpath() const;

    /**
     * Get the file part of the path (everything after the last path sep)
     * @return file string
     */
    std::string file() const;
  
    /**
     * Get the directory part of the path.
     * @return directory string
     */
    std::string dir() const;
  
    /**
     * Get the base part of the path (everything but the final extension.)
     * @return the base string
     */
    std::string base() const;

    /**
     * Get the base part of the filename (everything before the first '.')
     * @return the base filename
     */
    std::string file_base() const;

    /**
     * Get the extension part of the path (everything after the final ".")
     * @return the extension string
     */
    std::string extension() const;
    
    /**
     * Get the path string
     * @return path string
     */
    std::string str() const { return path; }
    std::string utf8Str() const { return path; }

    std::string local8BitStr() const;

    std::wstring wstr() const;

    /**
     * Get the path string
     * @return path in "C" string (ptr to char array) form.
     */
    const char* c_str() const { return path.c_str(); }

    /**
     * Get the path string in OS native form
     */
    std::string str_native() const;

    /**
     * Determine if file exists by attempting to fopen it.
     * @return true if file exists, otherwise returns false.
     */
    bool exists() const;

    /**
     * Check if reading file is allowed. Readabilty does not imply the existance
     * of the file.
     *
     * @note By default all files will be marked as readable. No check is made
     *       if the operating system allows the given file to be read. Derived
     *       classes may actually implement custom read/write rights.
     */
    bool canRead() const;
    bool canWrite() const;

    bool isFile() const;
    bool isDir() const;
    
    /**
     * Opposite sense to isAbsolute
     */
    bool isRelative() const { return !isAbsolute(); }
    
    /**
     * Is this an absolute path?
     * I.e starts with a directory seperator, or a single character + colon
     */
    bool isAbsolute() const;
    
    /**
     * check for default constructed path
     */
    bool isNull() const;
    
    /**
     * modification time of the file
     */
    time_t modTime() const;

    /**
     *
     */
    size_t sizeInBytes() const;

    /**
     * return the path of the parent directory of this path.
     */
    SGPath dirPath() const;

    /**
     * Get a path stored in the environment variable with the given \a name.
     *
     * @param name  Name of the environment variable
     * @param def   Default path to return if the environment variable does not
     *              exist or is empty.
     */
    static SGPath fromEnv(const char* name, const SGPath& def = SGPath());

    static SGPath fromUtf8(const std::string& bytes, PermissionChecker p = NULL);

    static SGPath fromLocal8Bit(const char* name);

    static std::vector<SGPath> pathsFromEnv(const char* name);

    static std::vector<SGPath> pathsFromUtf8(const std::string& paths);

    static std::vector<SGPath> pathsFromLocal8Bit(const std::string& paths);

    static std::string join(const std::vector<SGPath>& paths, const std::string& joinWith);
private:

    void fix();

    void validate() const;
    void checkAccess() const;

    bool permissionsAllowsWrite() const;

    std::string path;
    PermissionChecker _permission_checker;

    mutable bool _cached : 1;
    mutable bool _rwCached : 1;
    bool _cacheEnabled : 1; ///< cacheing can be disbled if required
    mutable bool _canRead : 1;
    mutable bool _canWrite : 1;
    mutable bool _exists : 1;
    mutable bool _isDir : 1;
    mutable bool _isFile : 1;
    mutable time_t _modTime;
    mutable size_t _size;
};

/// Output to an ostream
template<typename char_type, typename traits_type>
inline
std::basic_ostream<char_type, traits_type>&
operator<<(std::basic_ostream<char_type, traits_type>& s, const SGPath& p)
{ return s << "Path \"" << p.utf8Str() << "\""; }

/**
 * Split a directory string into a list of it's parent directories.
 */
string_list sgPathBranchSplit( const std::string &path );

/**
 * Split a directory search path into a vector of individual paths
 */
string_list sgPathSplit( const std::string &search_path );

#endif // _SG_PATH_HXX
