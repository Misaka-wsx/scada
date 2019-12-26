
#ifndef				APP_DSCADA_FL_CONFIG_HE
#define				APP_DSCADA_FL_CONFIG_HE


#include            "SysUtils.h"
#include			"SysRtdbmsCommon.h"
#include            "SysRtdbmsMetaData.h"

// SC fault signal wait timeout windows size
#define			DEF_SC_MAX_WAIT_VALID_SIGNAL_TIMEOUT					30 * 1000
#define			DEF_SC_MAX_WAIT_INVALID_SIGNAL_TIMEOUT					30 * 1000
#define			DEF_SC_FA_SYS_INTERVAL									3600

// GR fault signal wait timeout windows size
#define			DEF_GR_MAX_WAIT_VALID_SIGNAL_TIMEOUT					300 * 1000
#define			DEF_GR_MAX_WAIT_INVALID_SIGNAL_TIMEOUT					30 * 1000
#define			DEF_GR_MAX_WAIT_UC_TIMEOUT                              60 * 1000
#define			DEF_GR_MIN_ZERO_CURRENT									0.5f
#define			DEF_GR_NORAM_UC_DATA_RATIO								0.5f
#define			DEF_GR_DENIAL_SIGNAL_DOE								7200 // seconds.
#define			DEF_TINY_DENIAL_SIGNAL_AS_ONE_FAULT                     7200 * 1000L

#define			MIN_NORMAL_DEV_RATIO									0.1f
#define			MAX_NORMAL_DEV_RATIO									1.0f
#define			DEF_NORMAL_DEV_RATIO									0.5f



#define			DEF_LOST_VOLTAGE										40		//失压默认值
#define         DEF_UNDER_VOLTAGE										150		//欠压默认值
#define			DEF_OVER_VOLTAGE										280     //过压默认值
#define         DEF_OVER_CURRENT										500     //过流默认值
#define		    MIN_DROP_FUSE_DUP_FAULT                                 7200	//故障抑制时间s
#define			DEF_RECV_SC_WAIT_DF_OFF_DELAY							60      // 跌落接收到短路信号后，多久报送故障信息
#define			DEF_SC_WAIT_CALL_DATA_DELAY								60      // 等待召测的时延；
#define			DEF_OVERLOAD_AS_ZERO_VALUE								3.0     // 视0门限；

class AppScadaFLConfiguration
{
public:

	SysUInt   Sc_MaxWaitValidSignalTimeout; ;  //					30 * 1000  seconds.
	SysUInt	  Sc_MaxWaitInvalidSignalTimeout;   ; //					30 * 1000
	SysUInt   Sc_MinFAInterval;

	SysUInt   Gr_MaxWaitValidSignalTimeout;   ;					//300 * 1000
	SysUInt	  Gr_MaxWaitInvalidSignalTimeout;  ;             //30 * 1000
	SysUInt   Gr_MaxWaitUcRtDataTimeout;                     // 
	SysFloat  Gr_NormalUcDevRatio;

	SysFloat  Gr_MinFtuZeroCurrent;

	SysUInt   Gr_TemporaryAsSignal;

	SysUInt   Gr_MaxDenialDOE;

    SysUInt   Min_Online_Rate;

    SysString   Administrator_Phone_Number;

    SysString   Administrator_Name;

	SysUInt   Tiny_SC_MaxWaitValidSignalTimeout; 
	SysUInt   Tiny_SC_MaxWaitInvalidSignalTimeout;  
	SysUInt   Tiny_SC_MAXInervalB2Fault;

	SysUInt   Tiny_Lost_Voltage;
	SysUInt   Tiny_Under_Voltage;
	SysUInt   Tiny_Over_Voltage;

	SysUInt   Tiny_Lost_Current;
	SysUInt   Tiny_Over_Current;

	SysBool   isBreakerStatusReport;

	SysBool   isGRDeviceBrand;


	SysUInt   Df_MinDupFaultInterval;
	SysUInt   Df_RecvSCWaitDFOffDelay;
	SysUInt   Df_ScWaitCallDataDelay;
	SysFloat  Df_OverloadAsZeroValue;

    SysString SMS_DBSrv_IP;
    SysString SMS_DBSrv_Port;

	AppScadaFLConfiguration(): Sc_MaxWaitValidSignalTimeout(DEF_SC_MAX_WAIT_VALID_SIGNAL_TIMEOUT),
		Sc_MaxWaitInvalidSignalTimeout(DEF_SC_MAX_WAIT_INVALID_SIGNAL_TIMEOUT),
		Sc_MinFAInterval(DEF_SC_FA_SYS_INTERVAL),
		Gr_MaxWaitValidSignalTimeout(DEF_GR_MAX_WAIT_VALID_SIGNAL_TIMEOUT),
		Gr_MaxWaitInvalidSignalTimeout(DEF_GR_MAX_WAIT_INVALID_SIGNAL_TIMEOUT),
		Gr_MaxWaitUcRtDataTimeout(DEF_GR_MAX_WAIT_UC_TIMEOUT),
		Gr_MinFtuZeroCurrent(DEF_GR_MIN_ZERO_CURRENT),
		Gr_NormalUcDevRatio(DEF_GR_NORAM_UC_DATA_RATIO),
		Gr_TemporaryAsSignal(1),
		Gr_MaxDenialDOE(DEF_GR_DENIAL_SIGNAL_DOE),
		Tiny_SC_MaxWaitValidSignalTimeout(DEF_SC_MAX_WAIT_VALID_SIGNAL_TIMEOUT),
		Tiny_SC_MaxWaitInvalidSignalTimeout(DEF_SC_MAX_WAIT_INVALID_SIGNAL_TIMEOUT),
		Tiny_SC_MAXInervalB2Fault(DEF_TINY_DENIAL_SIGNAL_AS_ONE_FAULT),
		Tiny_Lost_Voltage(DEF_LOST_VOLTAGE),
		Tiny_Under_Voltage(DEF_UNDER_VOLTAGE),
		Tiny_Over_Voltage(DEF_OVER_VOLTAGE),
		Tiny_Lost_Current(5),
	    Tiny_Over_Current(DEF_OVER_CURRENT),
		isBreakerStatusReport(false),

		Df_MinDupFaultInterval(MIN_DROP_FUSE_DUP_FAULT),
		Df_RecvSCWaitDFOffDelay(DEF_RECV_SC_WAIT_DF_OFF_DELAY),
		Df_ScWaitCallDataDelay(DEF_SC_WAIT_CALL_DATA_DELAY),
		Df_OverloadAsZeroValue(DEF_OVERLOAD_AS_ZERO_VALUE),
		isGRDeviceBrand(false),
        SMS_DBSrv_IP(""),
		SMS_DBSrv_Port("")

	{

	}

	SYS_HRESULT initConfiguration();

	SYS_HRESULT getParams(SysString& nodeName, SysRtdbAny& nodeValue, RTDB_DATA_TYPE valueType);

    SYS_HRESULT getConfigStr(SysString& nodeName, SysString& nodeValue);

	void showParameters();
private:

	SYS_CLASS_LOGGER;
};

typedef   SysSingleton<AppScadaFLConfiguration>   AppScadaFLConfigurationMgr;

#endif