//
// iSeries.h
//

#pragma once
#include <string>

#ifdef iSERIESLAN
#pragma message("Compile Iseries with Fwlib/e1  - Lan")
#include "Fwlib/e1/fwlib32.h"
#endif
#ifdef iSERIESHSSB
#pragma message("Compile Iseries with 30i  - HSSB")
#include "Fwlib/30i/fwlib32.h"
#endif

class CCmdHandler;
#ifdef iSERIES
class CiSeries
{
public:
	CiSeries(CCmdHandler*);
	~CiSeries(void);
	int getStatus();
	int getSpeeds();
	int getPositions();
	int getLine();
	int getInfo();
	int getVersion();
	int getAlarmInfo();
	int LogErrorMessage(std::string msg, short errcode);
	int connect();
	void disconnect();
	CCmdHandler* _adapter;
	int nBlockNum;
	std::string lastprognum;
	std::string lastFeed,lastX, lastY, lastZ, lastA, lastB;
	int mLag;
	void saveStateInfo();
};


#include "iSeries.cpp"
#endif