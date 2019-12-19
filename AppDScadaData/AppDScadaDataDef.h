#ifndef		APP_DSCADA_DATA_DEF_H
#define		APP_DSCADA_DATA_DEF_H


#include "SysRtdbmsCommon.h"
/************************************************************************/
/*  Define  devType 
/************************************************************************/
#define     XHQD_620    "01"
#define     XHQD_B20    "02"
#define     XHQD_320    "03"
#define     XHQD_520    "05"
#define     XHQD_220    "06"
#define     XHQD_120    "11"
#define     XHQD_A21    "12"
#define     XHQD_B30    "17"
#define     XHQD_521_1    "18"

#define     IS_XHQD_620(type)       (XHQD_620 == type)
#define     IS_XHQD_B20(type)       (XHQD_B20 == type)
#define     IS_XHQD_320(type)       (XHQD_320 == type)
#define     IS_XHQD_520(type)       (XHQD_520 == type)
#define     IS_XHQD_220(type)       (XHQD_220 == type)
#define     IS_XHQD_120(type)       (XHQD_120 == type)
#define     IS_XHQD_A21(type)       (XHQD_A21 == type)
#define     IS_XHQD_B30(type)       (XHQD_B30 == type)
#define     IS_XHQD_521_1(type)       (XHQD_521_1 == type)
/************************************************************************/
/*  Define  the  remote signals macros
/************************************************************************/

#define		TAS_SINGLE_TELESIGNAL_1BYTES						0x01
#define		TAS_DOUBLE_TELESIGNAL_1BYTES						0x03
#define		TAS_GROUP_SINGLE_TELESIGNAL_1BYTE					0x14

/************************************************************************/
/*  Define  the telemetering macros
/************************************************************************/

#define		TAS_NORMALIZE_TELEMETRY_NA_3BYTES					0x09
#define		SAMPLE_QUA_TELEMETERING_6BYTES						0x0a
#define		TAS_SCALE_TELEMETRY_NB_3BYTES						0x0b
#define		SAMPLE_NVA_TELEMETERING_6BYTES						0x0c
#define		TAS_SHORT_FLOAT_TELEMETRY_NC_5BYTES					0x0d
#define		SAMPLE_QUA_FLOAT_TELEMETERING_8BYTES				0x0e
#define		TELEMETERING_SIMPLE_2BYTES							0x15
#define		TAS_NORMALIZE_TELEMETRY_TD_10BYTES					0x22
#define		TAS_SCALE_TELEMETRY_TE_10BYTES						0x23
#define		TAS_SHORT_FLOAT_TELEMETRY_TF_12BYTES				0x24
/************************************************************************/
/*  Define  the SOE macros
/************************************************************************/

#define		SHORT_SAMPLE_SINGLE_SOE								0x02
#define		SHORT_SAMPLE_DOUBLE_SOE								0x04
#define		TAS_SINGLE_TELESIGNAL_SOE							0x1e
#define		TAS_DOUBLE_TELESIGNAL_SOE							0x1f

#define		MAX_DSCADA_LEN										1024 * 1024
#define		CELL_NUM_LEN										20
#define		APP_WARNING_CDR_SIZE								4000
// Define Service Name const.
#define		WARN_IS_FAULT(warnmsg)								(warnmsg & 0xA0000000)


#define		APP_DSCADA_FAULT_NOTIFY_SERVER			"AppDScadaCtrlServer"

#define		PROCESS_DEV_RT_DATA(attr, lvalue, offset)			{SysRtdbAny & data = fieldValueVec[0].m_record[offset]; \
	                                                             if(data.m_dataType != SYS_RTDB_INVALID) \
																{  \
																attr = data.m_dataValue.##lvalue;\
																}}

#define			IS_LINE(id)     (GET_TAB_NO(id) == 202)
#define			IS_BUS(id)		(GET_TAB_NO(id) == 201)

enum
{
	TAS_APP_DSCADA_WARNING_EVENT_BEGIN = 0x6000,

	TAS_APP_DSCADA_LINE_WRNING,

	TAS_APP_DSCADA_WARNING_EVENT_END
};

enum
{
	TAS_SWITCH_INVALID = 0,
	TAS_SWITCH_OFF = 0x1,
	TAS_SWITCH_ON  = 0x11
};

typedef enum
{
	TAS_NORMAL_DATA,           // XH and 104

	TAS_SINGLE_TELESIGNAL = 1, // 1

	TAS_DOUBLE_TELESIGNAL,     //2

	TAS_SOE,     // 3

	TAS_TELEMETRY, //4
	
	TAS_TELECONTROL, //5

	TAS_READ,

	TAS_WRITE,

	TAS_CALL_ALL,

	TAS_SIGNAL,

	TAS_SYS_SOE,

	TAS_CALL_SPECIAL_FREQUENCY,

	TAS_CALL_ZERO_CURRENT,

	TAS_READ_EXECUTE_SUCCESS,

	TAS_READ_EXECUTE_FAILED,

	TAS_READ_SUCCESSFULL,

	TAS_READ_FAILED,

	TAS_WRITE_EXECUTE_SUCCESS,

	TAS_WRITE_EXECUTE_FAILED,

	TAS_WRITE_SUCCESSFULL,

	TAS_WRITE_FAILED,

	TAS_TELECONTROL_EXECUTE_SUCCESS,

	TAS_TELECONTROL_EXECUTE_FAILED,

	TAS_SHORT_WARNING_1,   // for FTU

	TAS_SHORT_WARNING_2,

	TAS_SHORT_WARNING_3,

	TAS_GROUNDING_ALARM, // for FTU

	TAS_GROUNDING_DISAPPEAR,  // for FTU

	TAS_RECLOSING_SUCCESS,   // for FTU

	TAS_SHORT_WARNING_PDI,	 //zhong ji qi

	TAS_GROUNDING_ALARM_PDI,

	TAS_GROUNDING_ALARM_DISAPPEAR_PDI,

	TAS_SHORT_WARNING_SFS, //te ping yuan

	TAS_GROUNDING_ALARM_SFS,

	TAS_GROUNDING_ALARM_DISAPPEAR_SFS,

	TAS_FTU_SWITCH_ON_SUCCESS,

	TAS_FTU_SWITCH_ON_FAILED,

	TAS_FTU_SWITCH_OFF_SUCCESS,

	TAS_FTU_SWITCH_OFF_FAILED,

	TAS_PARAMS,

	TAS_CALL_FORCE_OUTPUT,

	TAS_CALL_LOAD,

	TAS_LOAD,

	TAS_SPECIAL_FREQUENCY,

	TAS_ZERO_CURRENT,
	
	TAS_TEMPORARY_GROUNDING_ALARM,

	TAS_104_SOE,

	TAS_104_DOE,

	TAS_RECLOSING_FAILED,

	TAS_SWITCH_STATUS_CHG,

	TAS_DOE,

	TAS_TINY_GROUNDING_LOST_VOLTAGE,

	TAS_DROP_FUSE_SC_OFF,  // 跌落保险故障跌落

	TAS_DROP_FUSE_OFF,      // 跌落保险自然跌落

	TAS_LOST_VOLTAGE,       // 失压信号；

    TAS_IN_COMINGT          //失压恢复信号

}TAS_INTERNAL_DATA_TYPE;


typedef enum 
{

	OVER_CURRENT_FAULT = 1,		// 过流故障；
	 
	QUICK_BREAK_FAULT = 2,      // 短路故障；

	GROUND_FAULT = 4,			// 接地故障

	OVER_LOAD_FAULT = 8,		// 过负荷故障

	DF_DROP_FAULT  = 16,			// 跌落保险跌落故障

	NOTE_NONE_TYPE = 0x1000000000000000  // 未知类型

}TAS_WARNING_NOTE_TYPE;

typedef enum
{
	PRO_I_SEC = 1,

	PRO_II_SEC = 2,

	PRO_III_SEC = 4,

	PRO_ZERO = 8,

    NOVOLTAGE = 10,

    INCOMINGT = 20,
};

typedef enum
{
	FAULT_NO_DIR = 1,

	FAULT_DIR_POSITIVE = 2,

	FAULT_DIR_NEGATIVE = 4
};

typedef	enum 
{
	APP_DSCADA_FL_EVENT_BEGIN = 30,

	APP_DSCADA_FL_EVENT_XHDL_DEV_CODE = 36,

	APP_DSCADA_FL_EVENT_END
};

typedef enum
{
	APP_DSCADA_WARNING_MSG = 0xA0000000
};

// phase.

enum
{
	NO_FAULT = 0,
	A_PHASE = 1,
	B_PHASE = 2,
	C_PHASE = 4,
	AB_PHASE = 3,
	AC_PHASE = 5,
	BC_PHASE = 6,
	ABC_PHASE = 7,
	NO_PHASE = 8
};
enum
{
	SC_OVER_CURRENT = 0,
	SC_QUICK_BREAK = 1
};


inline void getFaultPhaseStr(SysUInt phaseType, SysString& phaseName)
{
	switch(phaseType)
	{
		case A_PHASE:
			{
				phaseName = "A相"; break;
			}
		case B_PHASE:
			{
				phaseName = "B相"; break;
			}
		case C_PHASE:
			{
				phaseName = "C相"; break;
			}
		case AB_PHASE:
			{
				phaseName = "AB相"; break;
			}
		case AC_PHASE:
			{
				phaseName = "AC相"; break;
			}
		case BC_PHASE:
			{
				phaseName = "BC相"; break;
			}
		case ABC_PHASE:
			{
				phaseName = "ABC相"; break;
			}
		case NO_PHASE:
			{
				phaseName = "无相别"; break;
			}
		default: 
			{
				phaseName = "未知相别"; break;
			}
	}
}



inline void getSection(SysInt secType, SysString& str)
{
	switch(secType)
	{
	case PRO_I_SEC:
		{
			str = "I段保护";
			break;
		}
	case PRO_II_SEC:
		{
			str = "II段保护";
			break;
		}
	case PRO_III_SEC:
		{
			str = "III段保护";
			break;
		}
	case PRO_ZERO:
		{
			str = "零序保护";
			break;
		}
    case NOVOLTAGE:
        {
            str = "失压保护";
            break;
        }
    case INCOMINGT:
        {
            str = "供电恢复";
            break;
        }
	default:
		{
			break;
		}

	}
}

inline void  getEventMsg(SysUInt signalEvent, SysString& eventMsg)
{
	switch(signalEvent & 0x0FFFFFFF)
	{
	case TAS_SOE:
	case TAS_SYS_SOE:
		{
			eventMsg = "开关变位";
			break;
		}
	case TAS_SHORT_WARNING_1:
		{
			eventMsg = "速断信号";
			break;
		}
	case TAS_SHORT_WARNING_2:
		{
			eventMsg = "过流信号";
			break;
		}
	case TAS_SHORT_WARNING_3:
		{
			eventMsg = "过负荷信号";
			break;
		}
	//case TAS_GROUNDING_ALARM,TAS_GROUNDING_ALARM_PDI,TAS_GROUNDING_ALARM_SFS:
	case TAS_GROUNDING_ALARM:
	case TAS_GROUNDING_ALARM_PDI:
	case TAS_GROUNDING_ALARM_SFS:
		{
			eventMsg = "接地信号";
			break;
		}
	case TAS_GROUNDING_DISAPPEAR:
		{
			eventMsg = "接地消失";
			break;
		}
	case TAS_RECLOSING_SUCCESS:
	case TAS_RECLOSING_FAILED:
		{
			eventMsg = "重合信号";
			break;
		}
	case TAS_104_SOE:
		{
			eventMsg = "104_SOE信号";
			break;
		}
	case TAS_DROP_FUSE_OFF:
		{
			eventMsg = "异常跌落";
			break;
		}
	case TAS_DROP_FUSE_SC_OFF:
		{
			eventMsg = "故障未跌落";
			break;
		}
	case TAS_LOST_VOLTAGE:
		{
			eventMsg = "失压信号";
			break;
		}
	default:
		{
			SysString str;
			SysOSUtils::numberToString(signalEvent, str);
			eventMsg = "未知信号:"; 
			eventMsg += str;
			break;
		}
	}
}

inline void  getFaultName(SysUInt faultType, SysString& faultName)
{
	switch(faultType & 0x0FFFFFFF)
	{
	case TAS_SHORT_WARNING_1:
		{
			faultName = "速断故障";
			break;
		}
	case TAS_SHORT_WARNING_2:
		{
			faultName = "过流故障";
			break;
		}
	case TAS_SHORT_WARNING_3:
		{
			faultName = "过负荷";
			break;
		}
	//case TAS_GROUNDING_ALARM,TAS_GROUNDING_ALARM_PDI,TAS_GROUNDING_ALARM_SFS:
	case TAS_GROUNDING_ALARM:
	case TAS_GROUNDING_ALARM_PDI:
	case TAS_GROUNDING_ALARM_SFS:
	case TAS_TINY_GROUNDING_LOST_VOLTAGE:
		{
			faultName = "接地故障";
			break;
		}
	case TAS_TEMPORARY_GROUNDING_ALARM:
		{
			faultName = "瞬时接地故障";
			break;
		}
	case TAS_DROP_FUSE_OFF:
		{
			faultName = "异常跌落";
			break;
		}
	case TAS_DROP_FUSE_SC_OFF:
		{
			faultName = "故障跌落";
		}
	default:
		{
			faultName = "未知类型故障";
			break;
		}
	}
}

inline void  get520FaultName(SysUInt faultType, SysString& faultName ,SysString& faultID)
{
    switch(faultType & 0x0FFFFFFF)
    {
        case TAS_SHORT_WARNING_3:
        case TAS_SHORT_WARNING_2:
        {
            faultName = "过载";
            faultID = "23";
            break;
        }
        case TAS_SHORT_WARNING_1:
            {
                faultName = "严重过载";
                faultID = "23";
                break;
            }
        case TAS_GROUNDING_ALARM:
        {
            faultName = "接地故障";
            faultID = "26";
            break;
        }
        default:
        {
            faultName = "未知类型故障";
            break;
        }
    }
}

inline void  getFaultDirection(SysUInt faultdir, SysString& dirName)
{
    switch(faultdir)
    {
    case 1:
        dirName = "无方向";
        break;
    case 2:
        dirName = "正方向";
        break;
    case 4:
        dirName = "负方向";
        break;
    default:
        dirName = "未知方向";
        break;
    }
}

inline void getDevName(SysULong devID, SysString & devName)
{
	SysUInt devType = (SysUInt)GET_TAB_NO(devID);
	switch(devType)
	{
	case  FTU_II_620:
		{
			devName = "FTU";
			break;
		}
	case REP_I_120:
		{
			devName = "中继器";
			break;
		}
	case SFS_I_220:
		{
			devName = "特频源";
			break;
		}
	case PN_FTU_DEVICE:
		{
			devName = "PN_FTU";
			break;
		}
	case DTU_II_820:
	case DTU_II_BREAKER:
		{
			devName = "DTU";
			break;
		}
	case FTU_II_B20:
		{
			devName = "B20";
			break;
		}
	case TINY_II_520:
		{
			devName = "低压0.4kV设备";
			break;
		}
	default:
		{
			devName = "未知设备"; break;
		}
	}
}

inline SysUInt FaultType2NoteType(SysUInt faultType)
{

	switch(faultType & 0x0FFFFFFF)
	{
	case TAS_SHORT_WARNING_1:
		{
			return QUICK_BREAK_FAULT;
			break;
		}
	case TAS_SHORT_WARNING_2:
		{
			return QUICK_BREAK_FAULT;
			break;
		}
	case TAS_SHORT_WARNING_3:
		{
			return OVER_LOAD_FAULT;
			break;
		}
	case TAS_GROUNDING_ALARM:
	case TAS_GROUNDING_ALARM_PDI:
	case TAS_GROUNDING_ALARM_SFS:
		{
			return GROUND_FAULT;
			break;
		}
	case TAS_DROP_FUSE_OFF:
	case TAS_DROP_FUSE_SC_OFF:
		{
			return DF_DROP_FAULT;
			break;
		}
	default:
		{
			return NOTE_NONE_TYPE;
			break;
		}
	}
}
inline SysBool isSCProtectSignal(SysInt faultType)
{
	if ((TAS_SHORT_WARNING_1 == faultType) ||
		(TAS_SHORT_WARNING_2 == faultType) ||
		(TAS_SHORT_WARNING_2 == faultType))
	{
		return true;
	}
	return false;
}

inline SysBool isGroundingProtectSignal(SysInt faultType)
{
	if ((TAS_GROUNDING_ALARM == faultType) ||
		(TAS_GROUNDING_ALARM_PDI == faultType) ||
		(TAS_GROUNDING_ALARM_SFS == faultType) ||
		(TAS_GROUNDING_DISAPPEAR == faultType))
	{
		return true;
	}
	return false;
}

inline SysBool isBreaker(SysULong devId)
{
	SysInt tabNo = (SysUInt)GET_TAB_NO(devId);
	if( (397 == tabNo) ||
		(398 == tabNo) ||
		(394 == tabNo))
	{
		return true;
	}

	return false;
}

inline SysBool isNormalPhaseVoltage(SysFloat value)
{
	return ((value >= 4000) && (value <= 8000))?true:false;
}

inline SysBool is400Dev(SysULong devID)
{
	return (402 == GET_TAB_NO(devID))?true:false;
}


inline SysBool isDropFuseDev(SysString& devCode)
{
	if (('1' == devCode[0]) && ('1' == devCode[1]) && ('2' == devCode[2]) && ('3' == devCode[3]))
	{
		return true;
	}
	return false;
}

class FtuZeroObj
{
public:
	std::vector<SysFloat> m_zeroCurVec;
	SysFloat     m_maxValue;
	FtuZeroObj():m_maxValue(0.0f)
	{

	}

};


class FtuZeroCurrentUnit
{
public:

	std::vector<SysULong>  m_devIDVec;
	std::vector<FtuZeroObj>  m_zeroCurrentVec;
	std::vector<SysString> m_devLongNameVec;
};
#endif