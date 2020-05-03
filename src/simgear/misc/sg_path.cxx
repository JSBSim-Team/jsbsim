// sg_path.cxx -- routines to abstract out path separator differences
//               between MacOS and the rest of the world
//
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
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 51
// Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//


#include <simgear/compiler.h>

#include <simgear/misc/strutils.hxx>

#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <fstream>
#include <cstdlib>
#include <iostream>

#ifdef _WIN32
#include <direct.h>
#endif
#include "sg_path.hxx"

using std::string;

/**
 * define directory path separators
 */

static const char sgDirPathSep = '/';
static const char sgDirPathSepBad = '\\';

#ifdef _WIN32
const char SGPath::pathListSep = ';';
#else
const char SGPath::pathListSep = ':';
#endif

// For windows, replace "\" by "/".
void
SGPath::fix()
{
    string::size_type sz = path.size();
    for ( string::size_type i = 0; i < sz; ++i ) {
        if ( path[i] == sgDirPathSepBad ) {
            path[i] = sgDirPathSep;
        }
    }
    // drop trailing "/"
    while ((sz>1)&&(path[sz-1]==sgDirPathSep))
    {
        path.resize(--sz);
    }
}


// default constructor
SGPath::SGPath(PermissionChecker validator)
    : path(""),
    _permission_checker(validator),
    _cached(false),
    _rwCached(false),
    _cacheEnabled(true)
{
}


// create a path based on "path"
SGPath::SGPath( const std::string& p, PermissionChecker validator )
    : path(p),
    _permission_checker(validator),
    _cached(false),
    _rwCached(false),
    _cacheEnabled(true)
{
    fix();
}

// create a path based on "path"
SGPath::SGPath(const std::wstring& p, PermissionChecker validator) :
	_permission_checker(validator),
	_cached(false),
	_rwCached(false),
	_cacheEnabled(true)
{
	path = simgear::strutils::convertWStringToUtf8(p);
	fix();
}


// create a path based on "path" and a "subpath"
SGPath::SGPath( const SGPath& p,
                const std::string& r,
                PermissionChecker validator )
    : path(p.path),
    _permission_checker(validator),
    _cached(false),
    _rwCached(false),
    _cacheEnabled(p._cacheEnabled)
{
    append(r);
    fix();
}

SGPath SGPath::fromLocal8Bit(const char *name)
{
    return SGPath(simgear::strutils::convertWindowsLocal8BitToUtf8(name));
}

SGPath SGPath::fromUtf8(const std::string& bytes, PermissionChecker p)
{
    return SGPath(bytes, p);
}


SGPath::SGPath(const SGPath& p) :
  path(p.path),
  _permission_checker(p._permission_checker),
  _cached(p._cached),
  _rwCached(p._rwCached),
  _cacheEnabled(p._cacheEnabled),
  _canRead(p._canRead),
  _canWrite(p._canWrite),
  _exists(p._exists),
  _isDir(p._isDir),
  _isFile(p._isFile),
  _modTime(p._modTime),
  _size(p._size)
{
}

SGPath& SGPath::operator=(const SGPath& p)
{
  path = p.path;
  _permission_checker = p._permission_checker,
  _cached = p._cached;
  _rwCached = p._rwCached;
  _cacheEnabled = p._cacheEnabled;
  _canRead = p._canRead;
  _canWrite = p._canWrite;
  _exists = p._exists;
  _isDir = p._isDir;
  _isFile = p._isFile;
  _modTime = p._modTime;
  _size = p._size;
  return *this;
}

// destructor
SGPath::~SGPath() {
}

// set path
void SGPath::set( const string& p ) {
    path = p;
    fix();
    _cached = false;
    _rwCached = false;
}

//------------------------------------------------------------------------------
void SGPath::setPermissionChecker(PermissionChecker validator)
{
  _permission_checker = validator;
  _rwCached = false;
}

//------------------------------------------------------------------------------
SGPath::PermissionChecker SGPath::getPermissionChecker() const
{
  return _permission_checker;
}

//------------------------------------------------------------------------------
void SGPath::set_cached(bool cached)
{
  _cacheEnabled = cached;
  _cached = false;
}

// append another piece to the existing path
void SGPath::append( const string& p ) {
    if ( path.empty() ) {
        path = p;
    } else {
    if ( p[0] != sgDirPathSep ) {
        path += sgDirPathSep;
    }
        path += p;
    }
    fix();
    _cached = false;
    _rwCached = false;
}

//------------------------------------------------------------------------------
SGPath SGPath::operator/( const std::string& p ) const
{
  SGPath ret = *this;
  ret.append(p);
  return ret;
}

#if defined(ENABLE_OLD_PATH_API)
//add a new path component to the existing path string
void SGPath::add( const string& p ) {
    append( SGPath::pathListSep+p );
}
#endif

// concatenate a string to the end of the path without inserting a
// path separator
void SGPath::concat( const string& p ) {
    if ( path.empty() ) {
        path = p;
    } else {
        path += p;
    }
    fix();
    _cached = false;
    _rwCached = false;
}

std::string SGPath::local8BitStr() const
{
    return simgear::strutils::convertUtf8ToWindowsLocal8Bit(path);
}

// Get the file part of the path (everything after the last path sep)
string SGPath::file() const
{
    string::size_type index = path.rfind(sgDirPathSep);
    if (index != string::npos) {
        return path.substr(index + 1);
    } else {
        return path;
    }
}


// get the directory part of the path.
string SGPath::dir() const {
    int index = path.rfind(sgDirPathSep);
    if (index >= 0) {
        return path.substr(0, index);
    } else {
        return "";
    }
}

SGPath SGPath::dirPath() const
{
	return SGPath::fromUtf8(dir());
}

// get the base part of the path (everything but the extension.)
string SGPath::base() const
{
    string::size_type index = path.rfind(".");
    string::size_type lastSep = path.rfind(sgDirPathSep);

// tolerate dots inside directory names
    if ((lastSep != string::npos) && (index < lastSep)) {
        return path;
    }

    if (index != string::npos) {
        return path.substr(0, index);
    } else {
        return path;
    }
}

string SGPath::file_base() const
{
    string::size_type index = path.rfind(sgDirPathSep);
    if (index == string::npos) {
        index = 0; // no separator in the name
    } else {
        ++index; // skip past the separator
    }

    string::size_type firstDot = path.find(".", index);
    if (firstDot == string::npos) {
        return path.substr(index); // no extensions
    }

    return path.substr(index, firstDot - index);
}

// get the extension (everything after the final ".")
// but make sure no "/" follows the "." character (otherwise it
// is has to be a directory name containing a ".").
string SGPath::extension() const {
    int index = path.rfind(".");
    if ((index >= 0)  && (path.find("/", index) == string::npos)) {
        return path.substr(index + 1);
    } else {
        return "";
    }
}

//------------------------------------------------------------------------------
void SGPath::validate() const
{
  if (_cached && _cacheEnabled) {
    return;
  }

  if (path.empty()) {
	  _exists = false;
      _canWrite = _canRead = false;
	  return;
  }

#if defined(_MSC_VER) || defined(__MINGW32__)
  struct _stat buf ;
  bool remove_trailing = false;
  std::wstring statPath(wstr());
  if ((path.length() > 1) && (path.back() == '/')) {
	  statPath.pop_back();
  }

  if (_wstat(statPath.c_str(), &buf ) < 0) {
    _exists = false;
    _canRead = false;

      // check parent directory for write-ability
      std::wstring parentPath = simgear::strutils::convertUtf8ToWString(dir());
      struct _stat parentBuf;
      if (_wstat(parentPath.c_str(), &parentBuf) >= 0) {
          _canWrite = parentBuf.st_mode & _S_IWRITE;
      } else {
          _canWrite = false;
      }
  } else {
    _exists = true;
    _isFile = ((S_IFREG & buf.st_mode ) !=0);
    _isDir = ((S_IFDIR & buf.st_mode ) !=0);
    _modTime = buf.st_mtime;
    _size = buf.st_size;
	_canRead = _S_IREAD & buf.st_mode;
	_canWrite = _S_IWRITE & buf.st_mode;
  }

#else
  struct stat buf ;

  if (stat(path.c_str(), &buf ) < 0) {
    _exists = false;
    _canRead = false;

      // check parent directory for write-ability
      std::string parentPath = dir();
      struct stat parentBuf;
      if (stat(parentPath.c_str(), &parentBuf) >= 0) {
          _canWrite = parentBuf.st_mode & S_IWUSR;
      } else {
          _canWrite = false;
      }
  } else {
    _exists = true;
    _isFile = ((S_ISREG(buf.st_mode )) != 0);
    _isDir = ((S_ISDIR(buf.st_mode )) != 0);
    _modTime = buf.st_mtime;
    _size = buf.st_size;
    _canRead = S_IRUSR & buf.st_mode;
    _canWrite = S_IWUSR & buf.st_mode;
  }

#endif
    // ensure permissions are no less restrictive than what the
    // permissions checker offers
    if ( _permission_checker ) {
        Permissions p = _permission_checker(*this);
        _canRead &= p.read;
        _canWrite &= p.write;
    }

  _cached = true;
}

//------------------------------------------------------------------------------
void SGPath::checkAccess() const
{
  if( _rwCached && _cacheEnabled )
    return;

  validate();
  _rwCached = true;
}

bool SGPath::exists() const
{
  validate();
  return _exists;
}

//------------------------------------------------------------------------------
bool SGPath::canRead() const
{
  checkAccess();
  return _canRead;
}

//------------------------------------------------------------------------------
bool SGPath::canWrite() const
{
  checkAccess();
  return _canWrite;
}

bool SGPath::isDir() const
{
  validate();
  return _exists && _isDir;
}

bool SGPath::isFile() const
{
  validate();
  return _exists && _isFile;
}

string_list sgPathBranchSplit( const string &dirpath ) {
    string_list path_elements;
    string element, path = dirpath;
    while ( ! path.empty() ) {
        size_t p = path.find( sgDirPathSep );
        if ( p != string::npos ) {
            element = path.substr( 0, p );
            path.erase( 0, p + 1 );
        } else {
            element = path;
            path = "";
        }
        if ( ! element.empty() )
            path_elements.push_back( element );
    }
    return path_elements;
}


string_list sgPathSplit( const string &search_path ) {
    string tmp = search_path;
    string_list result;
    result.clear();

    bool done = false;

    while ( !done ) {
        int index = tmp.find(SGPath::pathListSep);
        if (index >= 0) {
            result.push_back( tmp.substr(0, index) );
            tmp = tmp.substr( index + 1 );
        } else {
            if ( !tmp.empty() )
                result.push_back( tmp );
            done = true;
        }
    }

    return result;
}

bool SGPath::isAbsolute() const
{
  if (path.empty()) {
    return false;
  }

#ifdef _WIN32
  // detect '[A-Za-z]:/'
  if (path.size() > 2) {
    if (isalpha(path[0]) && (path[1] == ':') && (path[2] == sgDirPathSep)) {
      return true;
    }
  }
#endif

  return (path[0] == sgDirPathSep);
}

bool SGPath::isNull() const
{
  return path.empty();
}

#if defined(ENABLE_OLD_PATH_API)
std::string SGPath::str_native() const
{
#ifdef _WIN32
    std::string s = local8BitStr();
    std::string::size_type pos;
    std::string nativeSeparator;
    nativeSeparator = sgDirPathSepBad;

    while( (pos=s.find( sgDirPathSep )) != std::string::npos ) {
        s.replace( pos, 1, nativeSeparator );
    }
    return s;
#else
    return utf8Str();
#endif
}
#endif

time_t SGPath::modTime() const
{
    validate();
    return _modTime;
}

size_t SGPath::sizeInBytes() const
{
    validate();
    return _size;
}

bool SGPath::operator==(const SGPath& other) const
{
    return (path == other.path);
}

bool SGPath::operator!=(const SGPath& other) const
{
    return (path != other.path);
}

//------------------------------------------------------------------------------
SGPath SGPath::fromEnv(const char* name, const SGPath& def)
{
#if defined(_MSC_VER) || defined(__MINGW32__)
	std::wstring wname = simgear::strutils::convertUtf8ToWString(name);
	const wchar_t* val = _wgetenv(wname.c_str());
	if (val && val[0])
		return SGPath(val, def._permission_checker);
#else
  const char* val = getenv(name);
  if( val && val[0] )
    return SGPath(val, def._permission_checker);
#endif
  return def;
}

//------------------------------------------------------------------------------

std::vector<SGPath> SGPath::pathsFromEnv(const char *name)
{
    std::vector<SGPath> r;
#if defined(_MSC_VER)  || defined(__MINGW32__)
	std::wstring wname = simgear::strutils::convertUtf8ToWString(name);
	const wchar_t* val = _wgetenv(wname.c_str());
#else
	const char* val = getenv(name);
#endif
	if (!val) {
		return r;
	}
   
#if defined(_MSC_VER)  || defined(__MINGW32__)
	return pathsFromUtf8(simgear::strutils::convertWStringToUtf8(val));
#else
	return pathsFromUtf8(val);
#endif
}

//------------------------------------------------------------------------------

std::vector<SGPath> SGPath::pathsFromUtf8(const std::string& paths)
{
	std::vector<SGPath> r;
	string_list items = sgPathSplit(paths);
	string_list_iterator it;
	for (it = items.begin(); it != items.end(); ++it) {
		r.push_back(SGPath::fromUtf8(it->c_str()));
	}

	return r;
}

//------------------------------------------------------------------------------

std::vector<SGPath> SGPath::pathsFromLocal8Bit(const std::string& paths)
{
    std::vector<SGPath> r;
    string_list items =  sgPathSplit(paths);
    string_list_iterator it;
    for (it = items.begin(); it != items.end(); ++it) {
        r.push_back(SGPath::fromLocal8Bit(it->c_str()));
    }

    return r;
}

//------------------------------------------------------------------------------
SGPath SGPath::realpath() const
{
#if defined(_MSC_VER) || defined(__MINGW32__)
    // with absPath NULL, will allocate, and ignore length
  	std::wstring ws = wstr();
    wchar_t *buf = _wfullpath( NULL, ws.c_str(), _MAX_PATH );
#else
    // POSIX
    char* buf = ::realpath(path.c_str(), NULL);
#endif
    if (!buf) // File does not exist: return the realpath it would have if created now
              // (needed for fgValidatePath security)
    {
        if (path.empty()) {
            return SGPath(".").realpath(); // current directory
        }
        std::string this_dir = dir();
        if (isAbsolute() && this_dir.empty()) { // top level
            this_dir = "/";
        }
        if (file() == "..") {
            this_dir = SGPath(this_dir).realpath().dir();
            if (this_dir.empty()) { // invalid path: .. above root
                return SGPath();
            }
            return SGPath(this_dir).realpath(); // use native path separator,
                        // and handle 'existing/nonexisting/../symlink' paths
        }
        return SGPath(this_dir).realpath() / file();
    }

#if defined(_MSC_VER) || defined(__MINGW32__)
	  SGPath p = SGPath(std::wstring(buf), NULL);
#else
		SGPath p(SGPath::fromLocal8Bit(buf));
#endif
    free(buf);
    return p;

}

//------------------------------------------------------------------------------

std::string SGPath::join(const std::vector<SGPath>& paths, const std::string& joinWith)
{
    std::string r;
    if (paths.empty()) {
        return r;
    }

    r = paths[0].utf8Str();
    for (size_t i=1; i<paths.size(); ++i) {
        r += joinWith + paths[i].utf8Str();
    }

    return r;
}

//------------------------------------------------------------------------------
std::wstring SGPath::wstr() const
{
	return simgear::strutils::convertUtf8ToWString(path);
}

//------------------------------------------------------------------------------
bool SGPath::permissionsAllowsWrite() const
{
    return _permission_checker ? _permission_checker(*this).write : true;
}

