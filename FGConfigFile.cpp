/*******************************************************************************

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

********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGConfigFile.h"
#include <stdlib.h>
#include <math.h>

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/

FGConfigFile::FGConfigFile(string cfgFileName)
{
  cfgfile.open(cfgFileName.c_str());
  CommentsOn = false;
  CurrentIndex = 0;
  Opened = true;
  if (cfgfile.is_open()) GetNextConfigLine();
  else Opened = false;
}


FGConfigFile::~FGConfigFile(void)
{
  cfgfile.close();
}


string FGConfigFile::GetNextConfigLine(void)
{
  do {
    CurrentLine = GetLine();
    if (CurrentLine.find("<COMMENT>") != CurrentLine.npos) CommentsOn = true;
    if (CurrentLine.find("</COMMENT>") != CurrentLine.npos) {
      CommentsOn = false;
      GetNextConfigLine();
    }
  } while (IsCommentLine());
  if (CurrentLine.length() == 0) GetNextConfigLine();
  CurrentIndex = 0;
  return CurrentLine;
}


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


string FGConfigFile::GetValue(void)
{
  return GetValue("");
}


bool FGConfigFile::IsCommentLine(void)
{
  if (CurrentLine[0] == '/' && CurrentLine[1] == '/') return true;
  if (CommentsOn) return true;

  return false;
}


string FGConfigFile::GetLine(void)
{
  string scratch = "";
  unsigned int test;

  while ((test = cfgfile.get()) != EOF) {
    if (test >= 0x20) {
      scratch += (char)test;
    } else {
      if ((test = cfgfile.get()) != EOF) {
        if (test >= 0x20) cfgfile.unget();
        break;
      }
    }
  }
  if (cfgfile.eof()) return string("EOF");
  return scratch;
}

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

FGConfigFile& FGConfigFile::operator>>(float& val)
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


void FGConfigFile::ResetLineIndexToZero(void)
{
  CurrentIndex = 0;
}

