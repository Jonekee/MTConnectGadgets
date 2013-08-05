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
#include "cutting_tool.hpp"
#include "xml_printer.hpp"

#include <sstream>

using namespace std;

CuttingTool::~CuttingTool()
{
}

void CuttingTool::addValue(const CuttingToolValuePtr aValue)
{
  // Check for keys...
  if (aValue->mKey == "Location") {
    mKeys[aValue->mKey] = aValue->mValue;
  }
  
  mValues[aValue->mKey] = aValue;
}

inline static bool splitKey(string &key, string &sel, string &val) 
{
  size_t found = key.find_first_of('@');
  if (found != string::npos) {
    sel = key;
    key.erase(found);
    sel.erase(0, found + 1);
    size_t found = sel.find_first_of('=');
    if (found != string::npos) {
      val = sel;
      sel.erase(found);
      val.erase(0, found + 1);
      return true;
    }
  }
  return false;
}

void CuttingTool::updateValue(const std::string &aKey, const std::string &aValue)
{
  if (aKey == "Location") {
    mKeys[aKey] = aValue;
  }
  
  // Split into path and parts and update the asset bits.
  string key = aKey, sel, val;
  if (splitKey(key, sel, val)) {
    if (key == "ToolLife") {
      for (size_t i = 0; i < mLives.size(); i++)
      {
        CuttingToolValuePtr life = mLives[i];
        if (life->mProperties.count(sel) > 0 && life->mProperties[sel] == val)
        {
          life->mValue = aValue;
          break;
        }
      }
    } else {
      for (size_t i = 0; i < mItems.size(); i++)
      {
        CuttingItemPtr item = mItems[i];
        if (item->mIdentity.count(sel) > 0 && val == item->mIdentity[sel])
        {
          if (item->mValues.count(key) > 0)
            item->mValues[key]->mValue = aValue;
          else if (item->mMeasurements.count(key) > 0)
            item->mMeasurements[key]->mValue = aValue;
          break;
        }
      }      
    }
  } else {
    if (aKey == "CutterStatus") {
      mStatus.clear();
      istringstream stream(aValue);
      string val;
      while (getline(stream, val, ',')) {
        mStatus.push_back(val);
      }
    } else {
      if (mValues.count(aKey) > 0)
        mValues[aKey]->mValue = aValue;
      else if (mMeasurements.count(aKey) > 0)
        mMeasurements[aKey]->mValue = aValue;
    }
  }
}

void CuttingTool::addIdentity(const std::string &aKey, const std::string &aValue)
{
  if (aKey == "toolId") {
    mKeys[aKey] = aValue;
    mIdentity[aKey] = aValue;
  } else if (aKey == "deviceUuid") {
    mDeviceUuid = aValue;
  } else if (aKey == "timestamp") {
    mTimestamp = aValue;
  } else {
    mIdentity[aKey] = aValue;
  }
}

std::string &CuttingTool::getContent() 
{ 
  if (mContent.empty())
    mContent = XmlPrinter::printCuttingTool(this);
  
  return mContent; 
}


CuttingToolValue::~CuttingToolValue()
{
}

CuttingItem::~CuttingItem()
{
}
