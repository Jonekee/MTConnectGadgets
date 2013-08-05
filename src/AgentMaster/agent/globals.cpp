/*
* Copyright (c) 2008, AMT – The Association For Manufacturing Technology (“AMT”)
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the AMT nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* DISCLAIMER OF WARRANTY. ALL MTCONNECT MATERIALS AND SPECIFICATIONS PROVIDED
* BY AMT, MTCONNECT OR ANY PARTICIPANT TO YOU OR ANY PARTY ARE PROVIDED "AS IS"
* AND WITHOUT ANY WARRANTY OF ANY KIND. AMT, MTCONNECT, AND EACH OF THEIR
* RESPECTIVE MEMBERS, OFFICERS, DIRECTORS, AFFILIATES, SPONSORS, AND AGENTS
* (COLLECTIVELY, THE "AMT PARTIES") AND PARTICIPANTS MAKE NO REPRESENTATION OR
* WARRANTY OF ANY KIND WHATSOEVER RELATING TO THESE MATERIALS, INCLUDING, WITHOUT
* LIMITATION, ANY EXPRESS OR IMPLIED WARRANTY OF NONINFRINGEMENT,
* MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 

* LIMITATION OF LIABILITY. IN NO EVENT SHALL AMT, MTCONNECT, ANY OTHER AMT
* PARTY, OR ANY PARTICIPANT BE LIABLE FOR THE COST OF PROCURING SUBSTITUTE GOODS
* OR SERVICES, LOST PROFITS, LOSS OF USE, LOSS OF DATA OR ANY INCIDENTAL,
* CONSEQUENTIAL, INDIRECT, SPECIAL OR PUNITIVE DAMAGES OR OTHER DIRECT DAMAGES,
* WHETHER UNDER CONTRACT, TORT, WARRANTY OR OTHERWISE, ARISING IN ANY WAY OUT OF
* THIS AGREEMENT, USE OR INABILITY TO USE MTCONNECT MATERIALS, WHETHER OR NOT
* SUCH PARTY HAD ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.
*/

/* Dlib library */
#include "../lib/dlib/all/source.cpp"

#include "globals.hpp"

using namespace std;

string int64ToString(uint64_t i)
{
  ostringstream stm;
  stm << i;
  return stm.str();
}

string intToString(unsigned int i)
{
  ostringstream stm;
  stm << i;
  return stm.str();
}



string floatToString(double f)
{
  char s[32];
  sprintf(s, "%.7g", f);
  return (string) s;
}

string toUpperCase(string& text)
{
  for (unsigned int i = 0; i < text.length(); i++)
  {
    text[i] = toupper(text[i]);
  }
  
  return text;
}

bool isNonNegativeInteger(const string& s)
{
  for (unsigned int i = 0; i < s.length(); i++)
  {
    if (!isdigit(s[i]))
    {
      return false;
    }
  }
  
  return true;
}

string getCurrentTime(TimeFormat format)
{
#ifdef _WINDOWS
  SYSTEMTIME st;
  char timestamp[64];
  GetSystemTime(&st);
  sprintf(timestamp, "%4d-%02d-%02dT%02d:%02d:%02d", st.wYear, st.wMonth,
    st.wDay, st.wHour, st.wMinute, st.wSecond);
  
  if (format == GMT_UV_SEC)
  {
    sprintf(timestamp + strlen(timestamp), ".%04dZ", st.wMilliseconds);
  }
  else
  {
    strcat(timestamp, "Z");
  }
  
  return timestamp;
#else
  
  
  char timeBuffer[50];
  struct tm * timeinfo;
  struct timeval tv;
  struct timezone tz;

  gettimeofday(&tv, &tz);
  timeinfo = (format == LOCAL) ? localtime(&tv.tv_sec) : gmtime(&tv.tv_sec);
  
  switch (format)
  {
    case HUM_READ:
      strftime(timeBuffer, 50, "%a, %d %b %Y %H:%M:%S %Z", timeinfo);
      break;
    case GMT:
      strftime(timeBuffer, 50, "%Y-%m-%dT%H:%M:%SZ", timeinfo);
      break;
    case GMT_UV_SEC:
      strftime(timeBuffer, 50, "%Y-%m-%dT%H:%M:%S", timeinfo);
      break;
    case LOCAL:
      strftime(timeBuffer, 50, "%Y-%m-%dT%H:%M:%S%z", timeinfo);
      break;
  }
  
  
  if (format == GMT_UV_SEC)
  {
    sprintf(timeBuffer + strlen(timeBuffer), ".%06dZ", tv.tv_usec);
  }
  
  return string(timeBuffer);
#endif
}

unsigned int getCurrentTimeInSec()
{
  return time(NULL);
}

void replaceIllegalCharacters(string& data)
{
  for (unsigned int i=0; i<data.length(); i++)
  {
    char c = data[i];
    switch (c) {
      case '&': data.replace(i, 1, "&amp;"); break;
      case '<': data.replace(i, 1, "&lt;"); break;
      case '>': data.replace(i, 1, "&gt;"); break;
    }
  }
}

int getEnumeration(const string& name, const string *array, unsigned int size)
{
  for (unsigned int i = 0; i < size; i++)
  {
    if (name == array[i])
    {
       return i;
    }
  }
  
  return ENUM_MISS;
}

static string::size_type insertPrefix(string &aPath, string::size_type &aPos,
				      const string aPrefix)
{
  aPath.insert(aPos, aPrefix);
  aPos += aPrefix.length();
  aPath.insert(aPos++, ":");

  return aPos;
}

static bool hasNamespace(const string &aPath, string::size_type aStart)
{
  string::size_type len = aPath.length(), pos = aStart;
  while (pos < len) 
  {
    if (aPath[pos] == ':')
      return true;
    else if (!isalpha(aPath[pos]))
      return false;
    pos++;
  }

  return false;
}

string addNamespace(const string aPath, const string aPrefix)
{
  if (aPrefix.empty())
    return aPath;

  string newPath = aPath;
  string::size_type pos = 0;
  // Special case for relative pathing
  if (newPath[pos] != '/')
  {
    if (!hasNamespace(newPath, pos))
      insertPrefix(newPath, pos, aPrefix);
  }
  
  
  while ((pos = newPath.find('/', pos)) != string::npos && pos < newPath.length() - 1)
  {
    pos ++;
    if (newPath[pos] == '/')
      pos++;

    // Make sure there are no existing prefixes
    if (newPath[pos] != '*' && newPath[pos] != '\0' && !hasNamespace(newPath, pos))
      insertPrefix(newPath, pos, aPrefix);
  }

  pos = 0;
  while ((pos = newPath.find('|', pos)) != string::npos)
  {
    pos ++;
    if (newPath[pos] != '/' && !hasNamespace(newPath, pos))
      insertPrefix(newPath, pos, aPrefix);
  }
  
  return newPath;
}

bool isMTConnectUrn(const char *aUrn)
{
  return strncmp(aUrn, "urn:mtconnect.org:MTConnect", 27) == 0;
}

