/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGConfigFile.h
 Author:       Jon Berndt
 Date started: 03/29/00
 Purpose:      Config file read-in class
 Called by:    FGAircraft

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

HISTORY
--------------------------------------------------------------------------------
03/16/2000 JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGConfigFile.h"
#include <stdlib.h>
#include <math.h>

namespace JSBSim {

static const char *IdSrc = "$Id: FGConfigFile.cpp,v 1.47 2004/11/17 12:40:17 jberndt Exp $";
static const char *IdHdr = ID_CONFIGFILE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGConfigFile::FGConfigFile(string cfgFileName)
{
#if defined ( sgi ) && !defined( __GNUC__ ) && (_COMPILER_VERSION < 740)
  cfgfile.open(cfgFileName.c_str(), ios::in );
#else
  cfgfile.open(cfgFileName.c_str(), ios::in | ios::binary );
#endif
  CommentsOn = false;
  CurrentIndex = 0;
  Opened = true;
#if defined ( sgi ) && !defined( __GNUC__ ) && (_COMPILER_VERSION < 740)
   if (!cfgfile.fail() && !cfgfile.eof())  GetNextConfigLine();
#else
  if (cfgfile.is_open()) GetNextConfigLine();
#endif
  else Opened = false;

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGConfigFile::~FGConfigFile()
{
  cfgfile.close();
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGConfigFile::GetNextConfigLine(void)
{
  int comment_starts_at;
  int comment_ends_at;
  int comment_length;
  int line_length;
  bool start_comment, end_comment;
  string CommentStringTemp;

  do {
    CurrentLine = GetLine();
    line_length = CurrentLine.length();
    comment_starts_at = CurrentLine.find("<!--");
    
    if (comment_starts_at >= 0) start_comment = true;
    else start_comment = false;
    
    comment_ends_at = CurrentLine.find("-->");
    
    if (comment_ends_at >= 0) end_comment = true;
    else end_comment = false;

    if (!start_comment && !end_comment) {                              //  command comment
      if (CommentsOn) CommentStringTemp = CurrentLine;
      CommentString += CommentStringTemp + "\r\n";
    } else if (start_comment && comment_ends_at > comment_starts_at) { //  <!-- ... -->
      CommentsOn = false;
      comment_length = comment_ends_at + 2 - comment_starts_at + 1;
      LineComment = CurrentLine.substr(comment_starts_at+4, comment_length-4-3);
      CurrentLine.erase(comment_starts_at, comment_length);
      if (CurrentLine.find_first_not_of(" ") == string::npos) {
        CurrentLine.erase();
      }
    } else if ( start_comment && !end_comment) {                       //  <!-- ...
      CommentsOn = true;
      comment_length = line_length - comment_starts_at;
      CommentStringTemp = CurrentLine.substr(comment_starts_at+4, comment_length-4);
      CommentString = CommentStringTemp + "\r\n";
      CurrentLine.erase(comment_starts_at, comment_length);
    } else if (!start_comment && end_comment) {                       //  ... -->
      CommentsOn = false;
      comment_length = comment_ends_at + 2 + 1;
      CommentStringTemp = CurrentLine.substr(0, comment_length-4);
      CommentString += CommentStringTemp + "\r\n";
      CurrentLine.erase(0, comment_length);
    } else if (start_comment && comment_ends_at < comment_starts_at) { //  --> command <!--
      cerr << "Old comment ends and new one starts - bad JSBSim config file form." << endl;
      CommentsOn = false;
      comment_length = comment_ends_at + 2 + 1;
      CommentStringTemp = CurrentLine.substr(0, comment_length-4);
      CommentString += CommentStringTemp + "\r\n";
      CurrentLine.erase(0, comment_length);
    }
  } while (CommentsOn);

  CurrentIndex = 0;
  if (CurrentLine.length() == 0) {
    GetNextConfigLine();
  }
  return CurrentLine;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGConfigFile::GetValue(string val)
{
  string::size_type pos, p1, p2, ptest;

  if (val == "") {    // this call is to return the tag value
    pos = CurrentLine.find("<");
    if (pos != CurrentLine.npos) { // beginning brace found "<"
      p1 = CurrentLine.find_first_not_of(" ",pos+1);
      if (p1 != CurrentLine.npos) { // found first character of tag
        p2 = CurrentLine.find_first_of(" >",p1+1);
        if (p2 == CurrentLine.npos) p2 = p1+1;
        return CurrentLine.substr(p1,p2-p1);
      }
    } else {   // beginning brace "<" not found; this is a regular data line
      pos = CurrentLine.find_first_not_of(" ");
      if (pos != CurrentLine.npos) {  // first character in line found
        p2 = CurrentLine.find_first_of(" ",pos+1);
        if (p2 != CurrentLine.npos) {
          return CurrentLine.substr(pos,p2-pos);
        } else {
          return CurrentLine.substr(pos,CurrentLine.length()-pos);
        }
      }
    }
  } else { // return a value for a specific tag
    pos = CurrentLine.find(val);
    if (pos != CurrentLine.npos) {
      pos = CurrentLine.find("=",pos);
      if (pos != CurrentLine.npos) {
        ptest = CurrentLine.find_first_not_of(" ",pos+1);
        if (ptest != CurrentLine.npos) {
          p1 = ptest + 1;
          if (CurrentLine[ptest] == '"') { // quoted
            p2 = CurrentLine.find_first_of("\"",p1);
          } else { // not quoted
            p2 = CurrentLine.find_first_of(" ",p1);
          }
          if (p2 != CurrentLine.npos) {
            return CurrentLine.substr(p1,p2-p1);
          }
        }
      } else {   // "=" not found
        pos = CurrentLine.find(val);
        pos = CurrentLine.find_first_of(" ",pos+1);
        ptest = CurrentLine.find_first_not_of(" ",pos+1);
        if (ptest != CurrentLine.npos) {
          if (CurrentLine[ptest] == '"') { // quoted
            p1 = ptest + 1;
            p2 = CurrentLine.find_first_of("\"",p1);
          } else { // not quoted
            p1 = ptest;
            p2 = CurrentLine.find_first_of(" ",p1);
          }
          if (p2 != CurrentLine.npos) {
            return CurrentLine.substr(p1,p2-p1);
          } else {
            p2 = CurrentLine.length();
            return CurrentLine.substr(p1,p2-p1);
          }
        }
      }
    }
  }

  return string("");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGConfigFile::GetValue(void)
{
  return GetValue("");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGConfigFile::GetLine(void)
{
  string scratch = "";
  int test;

  while ((test = cfgfile.get()) != EOF) {
    if (test >= 0x20 || test == 0x09) {
      if (test == 0x09) {
        scratch += (char)0x20;
      } else {
        scratch += (char)test;
      }
    } else {
      if ((test = cfgfile.get()) != EOF) { // get *next* character
#if defined ( sgi ) && !defined( __GNUC__ ) && (_COMPILER_VERSION < 740) || defined (_MSC_VER)
        if (test >= 0x20 || test == 0x09) cfgfile.putback(test);
#else
        if (test >= 0x20 || test == 0x09) cfgfile.unget();
#endif
        break;
      }
    }
  }

  int index = scratch.find_last_not_of(" ");
  if (index != string::npos && index < (scratch.size()-1)) {
    scratch = scratch.substr(0,index+1);
  }

  if (cfgfile.eof() && scratch.empty()) return string("EOF");
  return scratch;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGConfigFile& FGConfigFile::operator>>(double& val)
{
  string::size_type pos, end;

  pos = CurrentLine.find_first_not_of(", ",CurrentIndex);
  if (pos == CurrentLine.npos) pos = CurrentLine.length();
  end = CurrentLine.find_first_of(", ",pos+1);
  if (end == CurrentLine.npos) end = CurrentLine.length();
  string str = CurrentLine.substr(pos, end - pos);
  val = strtod(str.c_str(),NULL);
  CurrentIndex = end+1;
  if (end == pos) {
    GetNextConfigLine();
    *this >> val;
  } else {
    if (CurrentIndex >= CurrentLine.length()) GetNextConfigLine();
  }
  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGConfigFile& FGConfigFile::operator>>(int& val)
{
  string::size_type pos, end;

  pos = CurrentLine.find_first_not_of(", ",CurrentIndex);
  if (pos == CurrentLine.npos) pos = CurrentLine.length();
  end = CurrentLine.find_first_of(", ",pos+1);
  if (end == CurrentLine.npos) end = CurrentLine.length();
  string str = CurrentLine.substr(pos, end - pos);
  val = atoi(str.c_str());
  CurrentIndex = end+1;
  if (end == pos) {
    GetNextConfigLine();
    *this >> val;
  } else {
    if (CurrentIndex >= CurrentLine.length()) GetNextConfigLine();
  }
  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGConfigFile& FGConfigFile::operator>>(string& str)
{
  string::size_type pos, end;

  pos = CurrentLine.find_first_not_of(", ",CurrentIndex);
  if (pos == CurrentLine.npos) pos = CurrentLine.length();
  end = CurrentLine.find_first_of(", ",pos+1);
  if (end == CurrentLine.npos) end = CurrentLine.length();
  str = CurrentLine.substr(pos, end - pos);
  CurrentIndex = end+1;
  if (end == pos) {
    GetNextConfigLine();
    *this >> str;
  } else {
    if (CurrentIndex >= CurrentLine.length()) GetNextConfigLine();
  }
  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGConfigFile::ResetLineIndexToZero(void)
{
  CurrentIndex = 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//    The bitmasked value choices are as follows:
//    unset: In this case (the default) JSBSim would only print
//       out the normally expected messages, essentially echoing
//       the config files as they are read. If the environment
//       variable is not set, debug_lvl is set to 1 internally
//    0: This requests JSBSim not to output any messages
//       whatsoever.
//    1: This value explicity requests the normal JSBSim
//       startup messages
//    2: This value asks for a message to be printed out when
//       a class is instantiated
//    4: When this value is set, a message is displayed when a
//       FGModel object executes its Run() method
//    8: When this value is set, various runtime state variables
//       are printed out periodically
//    16: When set various parameters are sanity checked and
//       a message is printed out when they go out of bounds

void FGConfigFile::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGConfigFile" << endl;
    if (from == 1) cout << "Destroyed:    FGConfigFile" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}
}
