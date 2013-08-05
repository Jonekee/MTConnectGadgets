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

#include "config.hpp"
#include "agent.hpp"
#include "options.hpp"
#include "device.hpp"
#include "xml_printer.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <dlib/config_reader.h>
#include <dlib/logger.h>
#include <stdexcept>
#include <sys/stat.h>

using namespace std;
using namespace dlib;

static logger sLogger("init.config");

static inline const char *get_with_default(const config_reader::kernel_1a &reader, 
      const char *aKey, const char *aDefault) {
  if (reader.is_key_defined(aKey))
    return reader[aKey].c_str();
  else
    return aDefault;
}

static inline int get_with_default(const config_reader::kernel_1a &reader, 
      const char *aKey, int aDefault) {
  if (reader.is_key_defined(aKey))
    return atoi(reader[aKey].c_str());
  else
    return aDefault;
}

static inline const string &get_with_default(const config_reader::kernel_1a &reader, 
      const char *aKey, const string &aDefault) {
  if (reader.is_key_defined(aKey))
    return reader[aKey];
  else
    return aDefault;
}

static inline bool get_bool_with_default(const config_reader::kernel_1a &reader, 
      const char *aKey, bool aDefault) {
  if (reader.is_key_defined(aKey))
    return reader[aKey] == "true" || reader[aKey] == "yes";
  else
    return aDefault;
}

AgentConfiguration::AgentConfiguration() :
  mAgent(new Agent())
{
}

void AgentConfiguration::initialize(int aArgc, const char *aArgv[])
{
  MTConnectService::initialize(aArgc, aArgv);
  
  const char *configFile = "agent.cfg";

  OptionsList optionList;
  optionList.append(new Option(0, configFile, "The configuration file", "file", false));  
  optionList.parse(aArgc, (const char**) aArgv);
  
  mConfigFile = configFile;

  try
  {
    configureLogger();
    ifstream file(mConfigFile.c_str());
    loadConfig(file);
  }
  catch (std::exception & e)
  {
    sLogger << LFATAL << "Agent failed to load: " << e.what();
    cerr << "Agent failed to load: " << e.what() << std::endl;
    optionList.usage();
  }
}

AgentConfiguration::~AgentConfiguration()
{
  if (mAgent != NULL)
    delete mAgent;
}

void AgentConfiguration::start()
{
  mAgent->start();
}

void AgentConfiguration::stop()
{
  mAgent->clear();
}

Device *AgentConfiguration::defaultDevice()
{
  const vector<Device*> &devices = mAgent->getDevices();
  if (devices.size() == 1)
    return devices[0];
  else
    return NULL;
}

static const char *timestamp(char *aBuffer)
{
#ifdef _WINDOWS
  SYSTEMTIME st;
  GetSystemTime(&st);
  sprintf(aBuffer, "%4d-%02d-%02dT%02d:%02d:%02d.%04dZ", st.wYear, st.wMonth, st.wDay, st.wHour, 
          st.wMinute, st.wSecond, st.wMilliseconds);
#else
  struct timeval tv;
  struct timezone tz;

  gettimeofday(&tv, &tz);

  strftime(aBuffer, 64, "%Y-%m-%dT%H:%M:%S", gmtime(&tv.tv_sec));
  sprintf(aBuffer + strlen(aBuffer), ".%06dZ", tv.tv_usec);
#endif
  
  return aBuffer;
}

static void print_ts_logger_header (
    std::ostream& out,
    const std::string& logger_name,
    const dlib::log_level& l,
    const uint64 thread_id
)
{
  char buffer[64];
  timestamp(buffer);
  out << buffer << ": " << l.name << " [" << thread_id << "] " << logger_name << ": ";
}

void AgentConfiguration::configureLogger()
{
  // Load logger configuration
  set_all_logging_levels(LINFO);
  set_all_logging_headers((dlib::logger::print_header_type) print_ts_logger_header);
  configure_loggers_from_file(mConfigFile.c_str());
  
  if (mIsDebug) {
    set_all_logging_output_streams(cout);
    set_all_logging_levels(LDEBUG);
  } else if (mIsService && sLogger.output_streambuf() == cout.rdbuf()) {
    ostream *file = new std::ofstream("agent.log");
    set_all_logging_output_streams(*file);
  }
}

inline static void trim(std::string &str)
{
  size_t index = str.find_first_not_of(" \r\t");
  if (index != string::npos && index > 0)
    str.erase(0, index);
  index = str.find_last_not_of(" \r\t");
  if (index != string::npos)
    str.erase(index + 1);
}

void AgentConfiguration::loadConfig(std::istream &aFile)
{
  // Now get our configuration
  config_reader::kernel_1a reader(aFile);

  bool defaultPreserve = get_bool_with_default(reader, "PreserveUUID", false);
  int port = get_with_default(reader, "Port", 5000);
  int bufferSize = get_with_default(reader, "BufferSize", DEFAULT_SLIDING_BUFFER_EXP);
  int maxAssets = get_with_default(reader, "MaxAssets", DEFAULT_MAX_ASSETS);
  int checkpointFrequency = get_with_default(reader, "CheckpointFrequency", 1000);
  int legacyTimeout = get_with_default(reader, "LegacyTimeout", 600);
  int reconnectInterval = get_with_default(reader, "ReconnectInterval", 10 * 1000);
  
  mPidFile = get_with_default(reader, "PidFile", "agent.pid");
  const char *probe;
  struct stat buf;
  if (reader.is_key_defined("Devices")) {
    probe = reader["Devices"].c_str();
    if (stat(probe, &buf) != 0) {
      throw runtime_error(((string) "Please make sure the Devices XML configuration "
			   "file " + probe + " is in the current path ").c_str());
    }
  } else {
    probe = "probe.xml";
    if (stat(probe, &buf) != 0)
      probe = "Devices.xml";    
    if (stat(probe, &buf) != 0) {
      throw runtime_error(((string) "Please make sure the configuration "
			   "file probe.xml or Devices.xml is in the current "
			   "directory or specify the correct file "
			   "in the configuration file " + mConfigFile +
			   " using Devices = <file>").c_str());
    }
  }

  mName = get_with_default(reader, "ServiceName", "MTConnect Agent");
    
  sLogger << LINFO << "Starting agent on port " << port;
  mAgent->Init(probe, bufferSize, maxAssets, checkpointFrequency);
  mAgent->set_listening_port(port);
  mAgent->setLogStreamData(get_bool_with_default(reader, "LogStreams", false));

  for (size_t i = 0; i < mAgent->getDevices().size(); i++)
    mAgent->getDevices()[i]->mPreserveUuid = defaultPreserve;
    
  loadAllowPut(reader);
  loadAdapters(reader, defaultPreserve, legacyTimeout, reconnectInterval);
  
  // Files served by the Agent... allows schema files to be served by
  // agent.
  loadFiles(reader);
  
  // Load namespaces, allow for local file system serving as well.
  loadNamespace(reader, "DevicesNamespaces", &XmlPrinter::addDevicesNamespace);
  loadNamespace(reader, "StreamsNamespaces", &XmlPrinter::addStreamsNamespace);
  loadNamespace(reader, "AssetsNamespaces", &XmlPrinter::addAssetsNamespace);
  loadNamespace(reader, "ErrorNamespaces", &XmlPrinter::addErrorNamespace);
}

void AgentConfiguration::loadAdapters(dlib::config_reader::kernel_1a &aReader,
                                      bool aDefaultPreserve, int aLegacyTimeout,
                                      int aReconnectInterval)
{
  Device *device;
  if (aReader.is_block_defined("Adapters")) {
    const config_reader::kernel_1a &adapters = aReader.block("Adapters");
    vector<string> blocks;
    adapters.get_blocks(blocks);
    
    vector<string>::iterator block;
    for (block = blocks.begin(); block != blocks.end(); ++block)
    {      
      const config_reader::kernel_1a &adapter = adapters.block(*block);
      string deviceName;      
      if (adapter.is_key_defined("Device")) {
        deviceName = adapter["Device"].c_str();
        if (deviceName == "*")
          device = defaultDevice();
        else
          device = mAgent->getDeviceByName(deviceName);
      } else {
        deviceName = *block;
        device = mAgent->getDeviceByName(deviceName);
        
        if (device == NULL && mAgent->getDevices().size() == 1)
          device = defaultDevice();
      }
      
      if (device == NULL) {
        throw runtime_error(static_cast<string>("Can't locate device '") + deviceName + "' in XML configuration file.");
      }
      
      const string host = get_with_default(adapter, "Host", (string)"localhost");
      int port = get_with_default(adapter, "Port", 7878);
      int legacyTimeout = get_with_default(adapter, "LegacyTimeout", aLegacyTimeout);
      int reconnectInterval = get_with_default(adapter, "ReconnectInterval", aReconnectInterval);

      
      sLogger << LINFO << "Adding adapter for " << device->getName() << " on "
              << host << ":" << port;
      Adapter *adp = mAgent->addAdapter(device->getName(), host, port, false, legacyTimeout);
      device->mPreserveUuid = get_bool_with_default(adapter, "PreserveUUID", aDefaultPreserve);
      
      // Add additional device information
      if (adapter.is_key_defined("UUID"))
        device->setUuid(adapter["UUID"]);
      if (adapter.is_key_defined("Manufacturer"))
        device->setManufacturer(adapter["Manufacturer"]);
      if (adapter.is_key_defined("Station"))
        device->setStation(adapter["Station"]);
      if (adapter.is_key_defined("SerialNumber"))
        device->setSerialNumber(adapter["SerialNumber"]);
      
      adp->setDupCheck(get_bool_with_default(adapter, "FilterDuplicates", adp->isDupChecking()));
      adp->setAutoAvailable(get_bool_with_default(adapter, "AutoAvailable", adp->isAutoAvailable()));
      adp->setIgnoreTimestamps(get_bool_with_default(adapter, "IgnoreTimestamps", adp->isIgnoringTimestamps()));
      adp->setReconnectInterval(reconnectInterval);
      
      if (adapter.is_key_defined("AdditionalDevices")) {
        istringstream devices(adapter["AdditionalDevices"]);
        string name;
        while (getline(devices, name, ',')) 
        {
          size_t index = name.find_first_not_of(" \r\t");
          if (index != string::npos && index > 0)
            name.erase(0, index);
          index = name.find_last_not_of(" \r\t");
          if (index != string::npos)
            name.erase(index + 1);
          
          adp->addDevice(name);
        }
      }
    }
  }
  else if ((device = defaultDevice()) != NULL)
  {
    sLogger << LINFO << "Adding default adapter for " << device->getName() << " on localhost:7878";
    mAgent->addAdapter(device->getName(), "localhost", 7878, false, aLegacyTimeout);
  }
  else
  {
    throw runtime_error("Adapters must be defined if more than one device is present");
  }  
}

void AgentConfiguration::loadAllowPut(dlib::config_reader::kernel_1a &aReader)
{
  bool putEnabled = get_bool_with_default(aReader, "AllowPut", false);
  mAgent->enablePut(putEnabled);
  
  string putHosts = get_with_default(aReader, "AllowPutFrom", "");
  if (!putHosts.empty())
  {
    istringstream toParse(putHosts);
    string putHost;
    do {
      getline(toParse, putHost, ',');
      trim(putHost);
      if (!putHost.empty()) {
        string ip;
        int n;
        for (n = 0; dlib::hostname_to_ip(putHost, ip, n) == 0 && ip == "0.0.0.0"; n++)
          ip = "";
        if (!ip.empty()) {
          mAgent->enablePut();
          mAgent->allowPutFrom(ip);
        }
      }
    } while (!toParse.eof());
  }
}

void AgentConfiguration::loadNamespace(dlib::config_reader::kernel_1a &aReader,
                                       const char *aNamespaceType,
                                       NamespaceFunction *aCallback)
{
  // Load namespaces, allow for local file system serving as well.
  if (aReader.is_block_defined(aNamespaceType)) {
    const config_reader::kernel_1a &namespaces = aReader.block(aNamespaceType);
    vector<string> blocks;
    namespaces.get_blocks(blocks);
    
    vector<string>::iterator block;
    for (block = blocks.begin(); block != blocks.end(); ++block)
    {
      const config_reader::kernel_1a &ns = namespaces.block(*block);
      if (!ns.is_key_defined("Urn"))
      {
        sLogger << LERROR << "Name space must have a Urn: " << *block;
      } else {
        string location;
        if (ns.is_key_defined("Location"))
          location = ns["Location"];
        (*aCallback)(ns["Urn"], location, *block);
        if (ns.is_key_defined("Path") && !location.empty())
          mAgent->registerFile(location, ns["Path"]);        
      }
    }
  }
}

void AgentConfiguration::loadFiles(dlib::config_reader::kernel_1a &aReader)
{
  if (aReader.is_block_defined("Files")) {
    const config_reader::kernel_1a &files = aReader.block("Files");
    vector<string> blocks;
    files.get_blocks(blocks);
    
    vector<string>::iterator block;
    for (block = blocks.begin(); block != blocks.end(); ++block)
    {
      const config_reader::kernel_1a &file = files.block(*block);
      if (!file.is_key_defined("Uri") || !file.is_key_defined("Path"))
      {
        sLogger << LERROR << "Name space must have a Uri and Path: " << *block;
      } else {
        mAgent->registerFile(file["Uri"], file["Path"]);
      }
    }
  }
}

