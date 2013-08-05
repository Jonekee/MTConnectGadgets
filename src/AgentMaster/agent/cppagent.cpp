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
#include "sys/stat.h"
#include "string.h"
#include "fcntl.h"

#include "agent.hpp"
#include "config.hpp"

#include "dlib/config_reader.h"
#include <dlib/logger.h>
#include "dlib/timeout/timeout_kernel_1.h"

using namespace std;
using namespace dlib;


static logger sLogger("main");
static   const log_level LENTRY(400,"ENTRY");

static const char INI_FILENAME[] = "agent.cfg";
static const char LOG_FILENAME[] = "debug.log";


extern SERVICE_STATUS          gSvcStatus; 
extern VOID ReportSvcStatus( DWORD dwCurrentState, //    The current state (see SERVICE_STATUS)
					 DWORD dwWin32ExitCode, //   The system error code
					 DWORD dwWaitHint); //    Estimated time for pending operation, 

#include "jsmtconnect.txt"

class AgentEx : public Agent
{
public:
	std::string contents; 
	AgentEx() 
	{
	}
	virtual std::string handleExtensionCall(const std::string& call, const std::string& device)
	{ 
	//	::ReadFile( ::ExeDirectory() + "jsmtconnect.html", contents);
		return monitorHtml ; 
	}
};
#ifdef _WINDOWS

//Handles Win32 exceptions (C structured exceptions) as C++ typed exceptions
static void trans_func( unsigned int u, EXCEPTION_POINTERS* pExp )
{
	std::stringstream outbuf;
	outbuf << "ResetAtMidnightThread  trans_func - Code =" << pExp->ExceptionRecord->ExceptionCode << std::endl;
	OutputDebugString( outbuf.str().c_str() );
	throw std::exception();
}
#endif
int main(int aArgc, const char *aArgv[])
{
#ifdef _WINDOWS
	_set_se_translator( trans_func );   //Handles Win32 exceptions (C structured exceptions) as C++ typed exceptions
	// MICHALOSKI ADDED
	TCHAR buf[1000];
	GetModuleFileName(NULL, buf, 1000);
	std::string path(buf);
	path=path.substr( 0, path.find_last_of( '\\' ) +1 );
	SetCurrentDirectory(path.c_str());
#endif
	AgentConfiguration config;
	config.setAgent(new AgentEx());


	Adapter::enummapping["execution.READY"]="IDLE";
	Adapter::enummapping["execution.ACTIVE"]="EXECUTING";
	Adapter::enummapping["execution.INTERRUPTED"]="PAUSED";
	Adapter::enummapping["execution.STOPPED"]="PAUSED";
	Adapter::keymapping["mode"]="controllermode";
	Adapter::keymapping["Fovr"]="path_feedrateovr";
	Adapter::keymapping["Fact"]="path_feedratefrt";


	return config.main(aArgc, (const char **) aArgv);
}


