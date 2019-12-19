#ifndef             APP_DSCADA_DROP_FUSE_H
#define             APP_DSCADA_DROP_FUSE_H

#include            "SysUtils.h"
#include            "SysBaseTypes.h"
#include            "AppDScadaComData.h"
#include            "AppDScadaDataDef.h"
#include            "AppDScadaMsgQueue.h"
#include            "AppDScadaFLConfig.h"
#include            "SysDBSrvModelClient.h"
#include            "AppDScadaDataToDBWorker.h"

#define             DEF_MIN_FAULT_INTERVAL        60*60

#define             OVERLOAD_IS_ZERO(value)     (value < AppScadaFLConfigurationMgr::getInstance()->Df_OverloadAsZeroValue)

typedef enum
{
    FAULT_INFO_SECTION = 0,
    FAULT_INFO_GPS = 1,
    FAULT_INFO_NAME = 2,
    FAULT_INFO_SIGNAL = 3,
    FAULT_INFO_TYPE = 4,
    LONGITUDE_GPS = 5,
    LATITUDE_GPS= 6
}FAULT_INFO;

class AppDScadaFTreeNode;

class AppDScadaDropFuseFault
{
public:

    enum
    {
        DF_INIT = 0,

        DF_OFF = 1,

        DF_SC_OFF,

        DF_SC_ONLY,

        INVALID_STATUS = 0x111111
    };

    AppDScadaDropFuseFault():m_lastFaultStamp(0),m_status(DF_INIT), m_sig(NULL),m_counter(0)
    {

    }

    ~AppDScadaDropFuseFault()
    {
        if (m_sig)
        {
            delete m_sig;
            m_sig = NULL;
        }
    }

    SysULong   m_lastFaultStamp; // counter
    
    SysString  m_devCode;

    SysUInt    m_status;

    AppDScadaSignal* m_sig;

    SysUInt    m_counter;
};

class AppDScadaFl4DFWorker : public SysThread
{
public:

    AppDScadaFl4DFWorker():m_minFaultInterval(DEF_MIN_FAULT_INTERVAL)
    {

    }

    SYS_HRESULT init();

    SYS_HRESULT run();

    SysBool isDropFuse(AppDScadaSignal* sig);
	//处理故障跌落信号
    SYS_HRESULT processDropFuseOff(AppDScadaSignal* sig);
	//*@brief 获取设备状态
	//* 状态000000 0-否 1-是 
	//*由左到右 是否拆除/是否退出运行/是否离线/电流/电压/定位 是否异常
    SYS_HRESULT getDeviceStatus(SysString & deviceCode,SysString & statusValue);
	//获取设备所在线路状态
	SYS_HRESULT getDeviceLineStatus(SysString & deviceCode, SysString & statusValue);
	//故障未跌落 短路信号
    SYS_HRESULT processDropFuseSC(AppDScadaSignal* sig);
	//处理第一次短路信号
    SYS_HRESULT processDropFuseSCFirst(AppDScadaSignal* sig);
	//循环处理故障树中数据，等待召测结果
    SYS_HRESULT updateStatus();
	//处理召测回来的数据
    SYS_HRESULT analysisCallData(AppDScadaDropFuseFault* faultPtr);
	//故障入库
    SYS_HRESULT dispatchSignalAsWarnMsg(AppDScadaSignal* sig);
	//同一设备短路信号与跌落信号合并
    SysBool combineAndSplitSignal(AppDScadaDropFuseFault* preSig, AppDScadaSignal* curSig);
	//由前置发送过来的故障信息
    AppDScadaMsgQueue<AppDScadaSignal> m_queue;

private:
	//处理跌落信号
    SYS_HRESULT processDropFuseSignal(AppDScadaSignal* sig);
	//根据召测结果判断线路载荷是否变化
    SysBool overloadIsChanged(SysFloat ia, SysFloat ib, SysFloat ic);
	//总召请求
    SYS_HRESULT deliverCallData(SysString& devCode);
	//故障类型描述
    SYS_HRESULT getDropFuseDescription(SysUInt faultType, SysString& faultName ,SysString& faultID);
	//故障设备经纬度信息
    SYS_HRESULT getDropFuseFaultInfo(AppDScadaSignal* sig, std::vector<SysString>& faultInfoVec);
	//跌落保险实时数据 三相电流
    SYS_HRESULT getDropFuseRtData(AppDScadaDropFuseFault* faultPtr, SysDBModelSrvRsp& rsp);

	//获取设备上次故障时间戳，故障抑制用
	SYS_HRESULT getDropFuseLastFaultData(AppDScadaSignal* faultPtr, SysDBModelSrvRsp& rsp);
	
    ACE_Recursive_Thread_Mutex     m_guard; 
	//故障跌落故障表
    std::map<SysString, AppDScadaDropFuseFault*> m_DropFaultMap;
	//短路故障（故障未跌落）故障表
    std::map<SysString, AppDScadaDropFuseFault*> m_ScFaultMap;
	//这个变量被初始化成一小时，但是没有被用过，所以我也不知道它是干嘛的。
    SysULong  m_minFaultInterval;   // seconds;
	//待删除信号
    std::vector<SysString>          m_removeSigVec;

    SysDBSrvModelClient             m_modelDB;

    SysDBSrvModelClient *m_pModelDB;

    SYS_CLASS_LOGGER;
};

typedef SysSingleton<AppDScadaFl4DFWorker>  AppDScadaFl4DFWorkerMgr;

class AppDScadaDropFusePolicy
{

public:

    AppDScadaDropFusePolicy():m_minFaultInterval(DEF_MIN_FAULT_INTERVAL)
    {

    }

    void init();

public:

    SysUInt   m_minFaultInterval;

private:

};

typedef SysSingleton<AppDScadaDropFusePolicy> AppDScadaDropFusePolicyMgr;
#endif