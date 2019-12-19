

#ifndef			APP_DSCADA_FL_TINY_H
#define         APP_DSCADA_FL_TINY_H

#include		"SysUtils.h"
#include		"SysBaseTypes.h"
#include		"AppDScadaComData.h"
#include		"AppDScadaDataDef.h"
#include		"AppDScadaMsgQueue.h"
#include		"SysRtdbmsTableOperator.h"
#include	"AppDScadaDataToDBWorker.h"
#include        "SysDBSrvModelClient.h"

#define			DEALY_WAIT_VALID_SIGNAL		20
#define			DEALY_WAIT_INVALID_SIGNAL   20



#define			LOST_DEVICE_INVERVAL_MIN	600 //seconds
#define			LOST_CURRENT				2		
#define            OVERLOAD_DATA_INVERVAL_HOUR                  43200000


#define			PHASE_A			0
#define			PHASE_B			1
#define			PHASE_C			2
#define			PHASE_N			3

class AppDeviceInfo
{
public:

    typedef enum  
	{
        DEVICE_NORMAL = 0x0,				 //装置正常

        DEVICE_DETRUSION = 0x01,			// 装置拆除
		
        DEVICE_QUIT = 0x10,		// 装置退出运行

		INVALID_STATUS = 0x111111          // 无效状态；

	};

private:

	SysInt status[4];

	static SysString  PHASE_DESC[4];
};

class AppTinyFault
{
public:

	typedef enum
	{

        SERIOUS_OVERLOAD =23,           //严重过载

        LOST_VOLTAGE =54,               //失压故障

        GROUNDING = 26,                 //接地故障

        IN_COMINGT = 55,                 //电压回复

        DEVICE_QUIT = 0x10,             // 装置退出运行

        INVALID_STATUS = 0x80000000,    // 无效状态；

        OVERLOAD_DATA_INVERVAL_TIME = 86400000,//过载

        SERIOUS_OVERLOAD_DATA_INVERVAL_TIME = 43200000,//严重过载

        LOST_VOLTAGE_DATA_INVERVAL_TIME = 21600000  //失压


        //OVERLOAD_DATA_INVERVAL_TIME = 45*60*1000,//过载

        //SERIOUS_OVERLOAD_DATA_INVERVAL_TIME = 45*60*1000,//严重过载

        //LOST_VOLTAGE_DATA_INVERVAL_TIME = 10*60*1000  //失压

    };

    SysString m_deviceCode;

    SysString m_transID;

    SysString m_pole;

    SysInt m_ftypeID;

    SysInt m_phase;

    SysInt m_ftypeDuration;

    SysFloat m_currenta;

    std::vector<SysString> devCode;

    std::vector<SysString> pole;

    std::vector<SysFloat> currenta;

    std::vector<SysInt> install;

    std::vector<SysInt> phase;

    std::vector<SysULong> devTime;

    SysULong m_putStoragetTime;

    SysULong m_updateDevTime;



	//ACE_Recursive_Thread_Mutex          m_guard;

	SYS_CLASS_LOGGER;
};


// 故障列表
typedef  std::map<SysString, AppTinyFault>     GLOBAL_TINY_FAULT_MAP;
typedef  GLOBAL_TINY_FAULT_MAP::iterator     GLOBAL_TINY_FAULT_MAP_IT;

typedef  std::map<SysString, AppDScadaSignal>     GLOBAL_TINY_MSG_MAP;
typedef  GLOBAL_TINY_MSG_MAP::iterator     GLOBAL_TINY_MSG_MAP_IT;

typedef  std::map<SysString, AppTinyFault*>     GLOBAL_TINY_FAULT_MAP1;
typedef  GLOBAL_TINY_FAULT_MAP::iterator     GLOBAL_TINY_FAULT_MAP_IT1;

class  AppDScadaTinyWorker : public SysThread
{
public:

	SYS_HRESULT init();


	SYS_HRESULT run();


	static AppDScadaMsgQueue<AppDScadaSignal> m_queue;

private:


	SYS_HRESULT processProtectionSig(AppDScadaSignal* sigPtr);

	SYS_HRESULT processSCSignal(AppDScadaSignal* sigPtr);

    SYS_HRESULT processLostVoltageSignal(AppDScadaSignal* sigPtr);

    SYS_HRESULT processInComingtSignal(AppDScadaSignal* sigPtr);

    SYS_HRESULT refreshInComingtMsgMap(AppDScadaSignal* sigPtr,AppTinyFault* newFault);

	SYS_HRESULT updateSCFaultMap();

    SYS_HRESULT refreshSCFaultMap(AppDScadaSignal* sigPtr,AppTinyFault* newFault);

    SYS_HRESULT updateSCMsgMap();

    SYS_HRESULT updateCSendMap();

    SYS_HRESULT refreshSCMsgMap(AppDScadaSignal* sigPtr,AppTinyFault* newFault);

    //SYS_HRESULT refreshSCMsgMap(AppDScadaSignal* sigPtr);

    SYS_HRESULT updateLostVoltageFaultMap();

    SYS_HRESULT updateLostVoltageMsgMap();

    SYS_HRESULT updateInComingtMsgMap();

    SYS_HRESULT refreshLostVoltageFaultMap(AppDScadaSignal* sigPtr,AppTinyFault* newFault);

    SYS_HRESULT refreshLostVoltageMsgMap(AppDScadaSignal* sigPtr,AppTinyFault* newFault);

    SYS_HRESULT putNewFault(AppDScadaSignal* sigPtr,AppTinyFault* newFault);

    SYS_HRESULT putDatabaseStr(AppDScadaSignal* sigPtr,SysInt databaseName);

    SYS_HRESULT refreshDataAndReprotMsg(AppDScadaSignal* sigPtr);

    SYS_HRESULT refreshDataAndPutFaultDatabase(AppDScadaSignal* sigPtr);

    SYS_HRESULT getDeviceStatus(SysString & deviceCode,SysString & statusValue);

    SYS_HRESULT overloadPutMsg(SysString & transID);

    SYS_HRESULT getLineAndPole(AppDScadaSignal* sigPtr);

    SYS_HRESULT getMsgDatabase(SysString & recInID);

    SYS_HRESULT sqlit(SysString & sql ,SysString & strc ,std::vector<string> & str);

    SYS_HRESULT getTransID(AppDScadaSignal* sigPtr,SysInt* install);

    SYS_HRESULT devNormalTest();


    GLOBAL_TINY_FAULT_MAP			m_curSCFaultMap;

    GLOBAL_TINY_FAULT_MAP			m_curSCMsgMap;

    GLOBAL_TINY_FAULT_MAP			m_curSCSendMap;

    GLOBAL_TINY_FAULT_MAP			m_curLostVoltageFaultMap;

    GLOBAL_TINY_FAULT_MAP			m_curLostVoltageMsgMap;

    GLOBAL_TINY_FAULT_MAP			m_curInComingtMsgMap;

    GLOBAL_TINY_MSG_MAP             m_curLostVoltageMap;

    GLOBAL_TINY_MSG_MAP             m_curInComingtMap;

    GLOBAL_TINY_MSG_MAP             m_curSCMsgSendMap;

	//friend class AppTinyFault;

    SysDBSrvModelClient *m_pModelDB;

    SysULong m_updateDataTime;

	ACE_Recursive_Thread_Mutex     m_guard; 

	SYS_CLASS_LOGGER;
};


typedef SysSingleton<AppDScadaTinyWorker>  AppDScadaTinyWorkerMgr;


#endif 