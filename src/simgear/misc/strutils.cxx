// String utilities.
//
// Written by Bernie Bright, started 1998
//
// Copyright (C) 1998  Bernie Bright - bbright@bigpond.net.au
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

#include <ctype.h>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <string.h>             // strerror_r() and strerror_s()
#include <errno.h>

#if defined(HAVE_CPP11_CODECVT)
    #include <codecvt> // new in C++11
#endif

#include "strutils.hxx"

#include <simgear/compiler.h>   // SG_WINDOWS


#if defined(SG_WINDOWS)
	#include <windows.h>
#endif

using std::string;
using std::vector;
using std::stringstream;
using std::cout;
using std::endl;

namespace simgear {
    namespace strutils {

	/*
	 * utf8ToLatin1() convert utf8 to latin, useful for accent character (i.e éâàîè...)
	 */
	template <typename Iterator> size_t get_length (Iterator p) {
		unsigned char c = static_cast<unsigned char> (*p);
		if (c < 0x80) return 1;
		else if (!(c & 0x20)) return 2;
		else if (!(c & 0x10)) return 3;
		else if (!(c & 0x08)) return 4;
		else if (!(c & 0x04)) return 5;
		else return 6;
	}

	typedef unsigned int value_type;
	template <typename Iterator> value_type get_value (Iterator p) {
		size_t len = get_length (p);
		if (len == 1) return *p;
		value_type res = static_cast<unsigned char> ( *p & (0xff >> (len + 1))) << ((len - 1) * 6 );
		for (--len; len; --len) {
		        value_type next_byte = static_cast<unsigned char> (*(++p)) - 0x80;
		        if (next_byte & 0xC0) return 0x00ffffff; // invalid UTF-8
			res |= next_byte << ((len - 1) * 6);
			}
		return res;
	}

	string utf8ToLatin1( string& s_utf8 ) {
		string s_latin1;
		for (string::iterator p = s_utf8.begin(); p != s_utf8.end(); ++p) {
			value_type value = get_value<string::iterator&>(p);
			if (value > 0x10ffff) return s_utf8; // invalid UTF-8: guess that the input was already Latin-1
			if (value > 0xff) SG_LOG(SG_IO, SG_WARN, "utf8ToLatin1: wrong char value: " << value);
			s_latin1 += static_cast<char>(value);
		}
		return s_latin1;
	}

	/**
	 *
	 */
	static vector<string>
	split_whitespace( const string& str, int maxsplit )
	{
	    vector<string> result;
	    string::size_type len = str.length();
	    string::size_type i = 0;
	    string::size_type j;
	    int countsplit = 0;

	    while (i < len)
	    {
		while (i < len && isspace((unsigned char)str[i]))
		{
		    ++i;
		}

		j = i;

		while (i < len && !isspace((unsigned char)str[i]))
		{
		    ++i;
		}

		if (j < i)
		{
		    result.push_back( str.substr(j, i-j) );
		    ++countsplit;
		    while (i < len && isspace((unsigned char)str[i]))
		    {
			++i;
		    }

		    if (maxsplit && (countsplit >= maxsplit) && i < len)
		    {
			result.push_back( str.substr( i, len-i ) );
			i = len;
		    }
		}
	    }

	    return result;
	}

	/**
	 *
	 */
	vector<string>
	split( const string& str, const char* sep, int maxsplit )
	{
	    if (sep == 0)
		return split_whitespace( str, maxsplit );

	    vector<string> result;
	    int n = std::strlen( sep );
	    if (n == 0)
	    {
		// Error: empty separator string
		return result;
	    }
	    const char* s = str.c_str();
	    string::size_type len = str.length();
	    string::size_type i = 0;
	    string::size_type j = 0;
	    int splitcount = 0;

	    while (i+n <= len)
	    {
		if (s[i] == sep[0] && (n == 1 || std::memcmp(s+i, sep, n) == 0))
		{
		    result.push_back( str.substr(j,i-j) );
		    i = j = i + n;
		    ++splitcount;
		    if (maxsplit && (splitcount >= maxsplit))
			break;
		}
		else
		{
		    ++i;
		}
	    }

	    result.push_back( str.substr(j,len-j) );
	    return result;
	}


        string_list split_on_any_of(const std::string& str, const char* seperators)
        {
            if (seperators == NULL || (strlen(seperators) == 0)) {
                throw "illegal/missing seperator string";
            }

            string_list result;
            size_t pos = 0;
            size_t startPos = str.find_first_not_of(seperators, 0);
            for(;;)
            {
                pos = str.find_first_of(seperators, startPos);
                if (pos == string::npos) {
                    result.push_back(str.substr(startPos));
                    break;
                }
                result.push_back(str.substr(startPos, pos - startPos));
                startPos = str.find_first_not_of(seperators, pos);
                if (startPos == string::npos) {
                    break;
                }
            }
            return result;

        }

	/**
	 * The lstrip(), rstrip() and strip() functions are implemented
	 * in do_strip() which uses an additional parameter to indicate what
	 * type of strip should occur.
	 */
	const int LEFTSTRIP = 0;
	const int RIGHTSTRIP = 1;
	const int BOTHSTRIP = 2;

	static string
	do_strip( const string& s, int striptype )
	{
	    string::size_type len = s.length();
	    if( len == 0 ) // empty string is trivial
		return s;
	    string::size_type i = 0;
	    if (striptype != RIGHTSTRIP)
	    {
		while (i < len && isspace(s[i]))
		{
		    ++i;
		}
	    }

	    string::size_type j = len;
	    if (striptype != LEFTSTRIP)
	    {
		do
		{
		    --j;
		}
		while (j >= 1 && isspace(s[j]));
		++j;
	    }

	    if (i == 0 && j == len)
	    {
		return s;
	    }
	    else
	    {
		return s.substr( i, j - i );
	    }
	}

	string
	lstrip( const string& s )
	{
	    return do_strip( s, LEFTSTRIP );
	}

	string
	rstrip( const string& s )
	{
	    return do_strip( s, RIGHTSTRIP );
	}

	string
	strip( const string& s )
	{
	    return do_strip( s, BOTHSTRIP );
	}

        void
        stripTrailingNewlines_inplace(string& s)
        {
          // Florent Rougon: The following (harder to read) implementation is
          // much slower on my system (g++ 6.2.1 on Debian): 11.4 vs. 3.9
          // seconds on 50,000,000 iterations performed on a short
          // CRLF-terminated string---and it is even a bit slower (12.9 seconds)
          // with std::next(it) instead of (it+1).
          //
          for (string::reverse_iterator it = s.rbegin();
               it != s.rend() && (*it == '\r' || *it == '\n'); /* empty */) {
            it = string::reverse_iterator(s.erase( (it+1).base() ));
          }

          // Simple and fast but need C++11
          // while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) {
          //   s.pop_back();
          // }
        }

        string
        stripTrailingNewlines(const string& s)
        {
          string res = s;
          stripTrailingNewlines_inplace(res);

          return res;
        }

	string
	rpad( const string & s, string::size_type length, char c )
	{
	    string::size_type l = s.length();
	    if( l >= length ) return s;
	    string reply = s;
	    return reply.append( length-l, c );
	}

	string
	lpad( const string & s, size_t length, char c )
	{
	    string::size_type l = s.length();
	    if( l >= length ) return s;
	    string reply = s;
	    return reply.insert( 0, length-l, c );
	}

	bool
	starts_with( const string & s, const string & substr )
	{
	  return s.compare(0, substr.length(), substr) == 0;
	}

	bool
	ends_with( const string & s, const string & substr )
	{
	  if( substr.length() > s.length() )
	    return false;
	  return s.compare( s.length() - substr.length(),
	                    substr.length(),
	                    substr ) == 0;
	}

    string simplify(const string& s)
    {
        string result; // reserve size of 's'?
        string::const_iterator it = s.begin(),
            end = s.end();

    // advance to first non-space char - simplifes logic in main loop,
    // since we can always prepend a single space when we see a
    // space -> non-space transition
        for (; (it != end) && isspace(*it); ++it) { /* nothing */ }

        bool lastWasSpace = false;
        for (; it != end; ++it) {
            char c = *it;
            if (isspace(c)) {
                lastWasSpace = true;
                continue;
            }

            if (lastWasSpace) {
                result.push_back(' ');
            }

            lastWasSpace = false;
            result.push_back(c);
        }

        return result;
    }

    int to_int(const std::string& s, int base)
    {
        stringstream ss(s);
        switch (base) {
        case 8:      ss >> std::oct; break;
        case 16:     ss >> std::hex; break;
        default: break;
        }

        int result;
        ss >> result;
        return result;
    }

    int compare_versions(const string& v1, const string& v2, int maxComponents)
    {
        vector<string> v1parts(split(v1, "."));
        vector<string> v2parts(split(v2, "."));

        int lastPart = std::min(v1parts.size(), v2parts.size());
        if (maxComponents > 0) {
            lastPart = std::min(lastPart, maxComponents);
        }

        for (int part=0; part < lastPart; ++part) {
            int part1 = to_int(v1parts[part]);
            int part2 = to_int(v2parts[part]);

            if (part1 != part2) {
                return part1 - part2;
            }
        } // of parts iteration

        // reached end - longer wins
        return v1parts.size() - v2parts.size();
    }

    string join(const string_list& l, const string& joinWith)
    {
        string result;
        unsigned int count = l.size();
        for (unsigned int i=0; i < count; ++i) {
            result += l[i];
            if (i < (count - 1)) {
                result += joinWith;
            }
        }

        return result;
    }

    string uppercase(const string &s) {
      string rslt(s);
      for(string::iterator p = rslt.begin(); p != rslt.end(); p++){
        *p = toupper(*p);
      }
      return rslt;
    }

    string lowercase(const string &s) {
      string rslt(s);
      for(string::iterator p = rslt.begin(); p != rslt.end(); p++){
        *p = tolower(*p);
      }
      return rslt;
    }

    void lowercase(string &s) {
      for(string::iterator p = s.begin(); p != s.end(); p++){
        *p = tolower(*p);
      }
    }

#if defined(SG_WINDOWS)
static std::wstring convertMultiByteToWString(DWORD encoding, const std::string& a)
{
    std::vector<wchar_t> result;
    DWORD flags = 0;
    int requiredWideChars = MultiByteToWideChar(encoding, flags,
                        a.c_str(), a.size(),
                        NULL, 0);
    result.resize(requiredWideChars);
    MultiByteToWideChar(encoding, flags, a.c_str(), a.size(),
                        result.data(), result.size());
	return std::wstring(result.data(), result.size());
}

static std::string convertWStringToMultiByte(DWORD encoding, const std::wstring& w)
{
	std::vector<char> result;
	DWORD flags = 0;
	int requiredMBChars = WideCharToMultiByte(encoding, flags,
		w.data(), w.size(),
		NULL, 0, NULL, NULL);
	result.resize(requiredMBChars);
	WideCharToMultiByte(encoding, flags,
		w.data(), w.size(),
		result.data(), result.size(), NULL, NULL);
	return std::string(result.data(), result.size());
}
#endif

std::wstring convertUtf8ToWString(const std::string& a)
{
#ifdef SG_WINDOWS
    return convertMultiByteToWString(CP_UTF8, a);
#elif defined(HAVE_CPP11_CODECVT)
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> ucs2conv;
    return ucs2conv.from_bytes(a);
#else
    return std::wstring();
#endif
}

std::string convertWStringToUtf8(const std::wstring& w)
{
#ifdef SG_WINDOWS
	return convertWStringToMultiByte(CP_UTF8, w);
#elif defined(HAVE_CPP11_CODECVT)
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> ucs2conv;
    return ucs2conv.to_bytes(w);
#else
    return std::string();
#endif
}

std::string convertWindowsLocal8BitToUtf8(const std::string& a)
{
#ifdef SG_WINDOWS
	return convertWStringToMultiByte(CP_UTF8, convertMultiByteToWString(CP_ACP, a));
#else
    return a;
#endif
}

std::string convertUtf8ToWindowsLocal8Bit(const std::string& a)
{
#ifdef SG_WINDOWS
	return convertWStringToMultiByte(CP_ACP, convertMultiByteToWString(CP_UTF8, a));
#else
    return a;
#endif
}

//------------------------------------------------------------------------------
static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

static const unsigned char base64_decode_map[128] =
{
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127,  62, 127, 127, 127,  63,  52,  53,
    54,  55,  56,  57,  58,  59,  60,  61, 127, 127,
    127,  64, 127, 127, 127,   0,   1,   2,   3,   4,
    5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
    15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
    25, 127, 127, 127, 127, 127, 127,  26,  27,  28,
    29,  30,  31,  32,  33,  34,  35,  36,  37,  38,
    39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
    49,  50,  51, 127, 127, 127, 127, 127
};


static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

static bool is_whitespace(unsigned char c) {
    return ((c == ' ') || (c == '\r') || (c == '\n'));
}

void decodeBase64(const std::string& encoded_string, std::vector<unsigned char>& ret)
{
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];

  while (in_len-- && ( encoded_string[in_] != '=')) {
    if (is_whitespace( encoded_string[in_])) {
        in_++;
        continue;
    }

    if (!is_base64(encoded_string[in_])) {
        break;
    }

    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = base64_decode_map[char_array_4[i]];

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret.push_back(char_array_3[i]);
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = base64_decode_map[char_array_4[j]];

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
  }
}

//------------------------------------------------------------------------------
const char hexChar[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

std::string encodeHex(const std::string& bytes)
{
  return encodeHex(
    reinterpret_cast<const unsigned char*>(bytes.c_str()),
    bytes.size()
  );
}

std::string encodeHex(const unsigned char* rawBytes, unsigned int length)
{
  std::string hex(length * 2, '\0');
  for (unsigned int i=0; i<length;++i) {
      unsigned char c = *rawBytes++;
      hex[i * 2] = hexChar[c >> 4];
      hex[i * 2 + 1] = hexChar[c & 0x0f];
  }

  return hex;
}

//------------------------------------------------------------------------------
std::string unescape(const char* s)
{
  std::string r;
  while( *s )
  {
    if( *s != '\\' )
    {
      r += *s++;
      continue;
    }

    if( !*++s )
      break;

    if (*s == '\\') {
        r += '\\';
    } else if (*s == 'n') {
        r += '\n';
    } else if (*s == 'r') {
        r += '\r';
    } else if (*s == 't') {
        r += '\t';
    } else if (*s == 'v') {
        r += '\v';
    } else if (*s == 'f') {
        r += '\f';
    } else if (*s == 'a') {
        r += '\a';
    } else if (*s == 'b') {
        r += '\b';
    } else if (*s == 'x') {
        if (!*++s)
            break;
        int v = 0;
        for (int i = 0; i < 2 && isxdigit(*s); i++, s++)
            v = v * 16 + (isdigit(*s) ? *s - '0' : 10 + tolower(*s) - 'a');
        r += v;
        continue;

    } else if (*s >= '0' && *s <= '7') {
        int v = *s++ - '0';
        for (int i = 0; i < 3 && *s >= '0' && *s <= '7'; i++, s++)
            v = v * 8 + *s - '0';
        r += v;
        continue;

    } else {
        r += *s;
    }
    s++;
  }
  return r;
}

string sanitizePrintfFormat(const string& input)
{
    string::size_type i = input.find("%n");
    if (i != string::npos) {
        SG_LOG(SG_IO, SG_WARN, "sanitizePrintfFormat: bad format string:" << input);
        return string();
    }

    return input;
}

std::string error_string(int errnum)
{
  char buf[512];                // somewhat arbitrary...
  // This could be simplified with C11 (annex K, optional...), which offers:
  //
  //   errno_t strerror_s( char *buf, rsize_t bufsz, errno_t errnum );
  //   size_t strerrorlen_s( errno_t errnum );

#if defined(_WIN32)
  errno_t retcode;
  // Always makes the string in 'buf' null-terminated
  retcode = strerror_s(buf, sizeof(buf), errnum);
#elif defined(_GNU_SOURCE)
  return std::string(strerror_r(errnum, buf, sizeof(buf)));
#elif (_POSIX_C_SOURCE >= 200112L) || defined(SG_MAC) || defined(__FreeBSD__)
  int retcode;
  // POSIX.1-2001 and POSIX.1-2008
  retcode = strerror_r(errnum, buf, sizeof(buf));
#else
#error "Could not find a thread-safe alternative to strerror()."
#endif

#if !defined(_GNU_SOURCE)
  if (retcode) {
    std::string msg = "unable to get error message for a given error number";
    // C++11 would make this shorter with std::to_string()
    std::ostringstream ostr;
    ostr << errnum;

#if !defined(_WIN32)
    if (retcode == ERANGE) {    // more specific error message in this case
      msg = std::string("buffer too small to hold the error message for "
                        "the specified error number");
    }
#endif

    throw msg + ostr.str();
  }

  return std::string(buf);
#endif  // !defined(_GNU_SOURCE)
}

} // end namespace strutils

} // end namespace simgear
