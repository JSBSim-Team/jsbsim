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

static const char *IdSrc = "$Id: FGConfigFile.cpp,v 1.31 2001/11/28 00:20:18 jberndt Exp $";
static const char *IdHdr = ID_CONFIGFILE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGConfigFile::FGConfigFile(string cfgFileName)
{
#if defined ( sgi ) && !defined( __GNUC__ )
  cfgfile.open(cfgFileName.c_str(), ios::in );
#else
  cfgfile.open(cfgFileName.c_str(), ios::in | ios::binary );
#endif
  CommentsOn = false;
  CurrentIndex = 0;
  Opened = true;
#if defined ( sgi ) && !defined( __GNUC__ )
   if (!cfgfile.fail() && !cfgfile.eof())  GetNextConfigLine();
#else
  if (cfgfile.is_open()) GetNextConfigLine();
#endif
  else Opened = false;

  if (debug_lvl & 2) cout << "Instantiated: FGConfigFile" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGConfigFile::~FGConfigFile()
{
  cfgfile.close();
  if (debug_lvl & 2) cout << "Destroyed:    FGConfigFile" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGConfigFile::GetNextConfigLine(void)
{
  int deblank, not_found = string::npos;
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

  if (CurrentLine.length() == 0) GetNextConfigLine();
  CurrentIndex = 0;
  return CurrentLine;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGConfigFile::GetValue(string val)
{
  unsigned int pos, p1, p2, ptest;

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
#if defined ( sgi ) && !defined( __GNUC__ )
        if (test >= 0x20 || test == 0x09) cfgfile.putback(test);
#else
        if (test >= 0x20 || test == 0x09) cfgfile.unget();
#endif
        break;
      }
    }
  }
  if (cfgfile.eof()) return string("EOF");
  return scratch;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/*
FGConfigFile& FGConfigFile::operator>>(double& val)
{
  unsigned int pos, end;

  pos = CurrentLine.find_first_not_of(", ",CurrentIndex);
  if (pos == CurrentLine.npos) pos = CurrentLine.length();
  end = CurrentLine.find_first_of(", ",pos+1);
  if (end == CurrentLine.npos) end = CurrentLine.length();
  string str = CurrentLine.substr(pos, end - pos);
  val = strtod(str.c_str(),NULL);
  CurrentIndex = end+1;
  if (CurrentIndex >= CurrentLine.length()) GetNextConfigLine();
  return *this;
}
*/
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGConfigFile& FGConfigFile::operator>>(double& val)
{
  unsigned int pos, end;

  pos = CurrentLine.find_first_not_of(", ",CurrentIndex);
  if (pos == CurrentLine.npos) pos = CurrentLine.length();
  end = CurrentLine.find_first_of(", ",pos+1);
  if (end == CurrentLine.npos) end = CurrentLine.length();
  string str = CurrentLine.substr(pos, end - pos);
  val = strtod(str.c_str(),NULL);
  CurrentIndex = end+1;
  if (CurrentIndex >= CurrentLine.length()) GetNextConfigLine();
  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGConfigFile& FGConfigFile::operator>>(int& val)
{
  unsigned int pos, end;

  pos = CurrentLine.find_first_not_of(", ",CurrentIndex);
  if (pos == CurrentLine.npos) pos = CurrentLine.length();
  end = CurrentLine.find_first_of(", ",pos+1);
  if (end == CurrentLine.npos) end = CurrentLine.length();
  string str = CurrentLine.substr(pos, end - pos);
  val = atoi(str.c_str());
  CurrentIndex = end+1;
  if (CurrentIndex >= CurrentLine.length()) GetNextConfigLine();
  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGConfigFile& FGConfigFile::operator>>(eParam& val)
{
  unsigned int pos, end;

  pos = CurrentLine.find_first_not_of(", ",CurrentIndex);
  if (pos == CurrentLine.npos) pos = CurrentLine.length();
  end = CurrentLine.find_first_of(", ",pos+1);
  if (end == CurrentLine.npos) end = CurrentLine.length();
  string str = CurrentLine.substr(pos, end - pos);
  val = (eParam)atoi(str.c_str());
  CurrentIndex = end+1;
  if (CurrentIndex >= CurrentLine.length()) GetNextConfigLine();
  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGConfigFile& FGConfigFile::operator>>(string& str)
{
  unsigned int pos, end;

  pos = CurrentLine.find_first_not_of(", ",CurrentIndex);
  if (pos == CurrentLine.npos) pos = CurrentLine.length();
  end = CurrentLine.find_first_of(", ",pos+1);
  if (end == CurrentLine.npos) end = CurrentLine.length();
  str = CurrentLine.substr(pos, end - pos);
  CurrentIndex = end+1;
  if (CurrentIndex >= CurrentLine.length()) GetNextConfigLine();
  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGConfigFile::ResetLineIndexToZero(void)
{
  CurrentIndex = 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGConfigFile::Debug(void)
{
    //TODO: Add your source code here
}

