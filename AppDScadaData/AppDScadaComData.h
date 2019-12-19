#ifndef				TAS_APP_DSCADA_COMMON_DATA_H
#define				TAS_APP_DSCADA_COMMON_DATA_H

#include  "SysRtdbmsMetaData.h"
#include  "AppDScadaDataDef.h"

class SysCDRStreamReader
{
public:

	SysCDRStreamReader(SysCDRInputStreamPtr in)
	{
		m_in = in;
	}

	~SysCDRStreamReader()
	{

	}

public:

	void operator >> (SysString &str_val)
	{
		if (m_in)
		{
			read(*m_in, str_val);
		}
	}

public:

	static SysBool read(SysCDRInputStream &in, SysString &str_val)
	{
		SysInt size = 0;
		in >> size;
		if (0 == size)
		{
			return false;
		}
		str_val.resize(size);
		in.read_char_array((SysChar*)str_val.c_str(), size);
		
		return true;
	}

private:

	SysCDRInputStreamPtr m_in;


};

class SysCDRStreamWriter
{
public:

	SysCDRStreamWriter(SysCDROutputStreamPtr out)
	{
		m_out = out;
	}

	~SysCDRStreamWriter()
	{

	}

public:

	void operator << (SysString &str_val)
	{
		if (m_out)
		{
			write(*m_out, str_val);
		}
	}

public:

	static SysBool write(SysCDROutputStream &out, SysString &str_val)
	{
		out << str_val.length();
		out.write_char_array(str_val.c_str(), str_val.length());
		return true;
	}

private:

	SysCDROutputStreamPtr m_out;
};

class  DScadaDataHeader
{
public:

	SysUInt		m_dummy1;
	SysUInt		m_marshedDatalen;
	SysUInt		m_dummy2;
	SysUInt		m_dummy3;
	SysUInt		m_dummy4;
};

class  DScadaData : public SysCDRable
{

public:

	SysUInt			m_type;     ///<TAS_INTERNAL_DATA_TYPE类型，TAS_SINGLE_TELESIGNAL、TAS_SOE、TAS_TELEMETRY，TAS_READ等枚举值
    SysUInt         m_specialType;      ///<特殊类型，实时数据、定值数据等
    SysString       m_devCode;
    SysString       m_fieldName;
	SysULong		m_timeStamp;
	SysRtdbAny		m_value;
	SysShort		m_quality;
	SysShort        m_phase;
	SysShort        m_scType;
	SysString       m_desc;

	DScadaData(): m_type(0),m_fieldName(""), m_timeStamp(0), m_quality(0),m_devCode("")
	{

	}
public:

	SYS_HRESULT marshalToCDR(SysCDROutputStreamPtr out)
	{
		SYS_HRESULT hr = SYS_OK;

		if (out)
		{
			*out << m_type;
			*out << m_timeStamp;

			hr = m_value.marshalToCDR(out);
			*out << m_quality;
			*out << m_phase;
			*out << m_scType;

			WRITE_STRING_IN_CDR(m_desc, out);
            WRITE_STRING_IN_CDR(m_fieldName,out);
            WRITE_STRING_IN_CDR(m_devCode,out);
		}
		else
		{
			hr = TAS_SYS_RPC_MARSHAL_ERROR;
		}
		return hr;
	}

	SYS_HRESULT unMarshalToObj(SysCDRInputStreamPtr in)
	{
		SYS_HRESULT hr = SYS_OK;

		if (in)
		{
			*in >> m_type;
			*in >> m_timeStamp;
			hr = m_value.unMarshalToObj(in);
			*in >> m_quality;
			*in >> m_phase;
			*in >> m_scType;
			READ_STRING_FROM_CDR(m_desc, in);
            READ_STRING_FROM_CDR(m_fieldName,in);
            READ_STRING_FROM_CDR(m_devCode,in);
		}
		else
		{
			hr = TAS_SYS_RPC_UNMARSHAL_ERROR;
		}

		return hr;

	}

};

/**
    @class DScadaDevPoint
    @brief 兴汇协议设备的点表数据
*/
class DScadaDevPoint : public SysCDRable
{
public:

	DScadaDevPoint()
	{
		this->clear();
	}

	~DScadaDevPoint()
	{

	}

public:

	void clear()
	{
        m_decCode = EMPTY_STRING;
        m_devType = EMPTY_STRING;
		m_fieldName = EMPTY_STRING;
        m_pointAddr = INVALID_NUMBER;
		m_value = INVALID_NUMBER;
	}

public:
    SysString m_decCode;

    SysString m_devType;

	SysString m_fieldName;

    SysULong m_pointAddr;
	
	SysRtdbAny m_value;

public:

	SYS_HRESULT marshalToCDR(SysCDROutputStreamPtr out)
	{
		HRESULT_INIT;
		
		if (out)
		{
            WRITE_STRING_IN_CDR(m_devType,out);
            WRITE_STRING_IN_CDR(m_decCode,out);
            WRITE_STRING_IN_CDR(m_fieldName,out);
            *out << m_pointAddr;
			m_value.marshalToCDR(out);
		}
		else
		{
			hr = TAS_SYS_RPC_MARSHAL_ERROR;
		}


		HRESULT_RETURN;
	}

	SYS_HRESULT unMarshalToObj(SysCDRInputStreamPtr in)
	{
		HRESULT_INIT;
		
		if (in)
		{
            READ_STRING_FROM_CDR(m_devType,in);
            READ_STRING_FROM_CDR(m_decCode,in);
			READ_STRING_FROM_CDR(m_fieldName,in);
            *in >> m_pointAddr;
			m_value.unMarshalToObj(in);
		}
		else
		{
			hr = TAS_SYS_RPC_UNMARSHAL_ERROR;
		}

		HRESULT_RETURN;
	}

};

/**
    @class DScadaDevData
    @brief 兴汇协议设备DScada数据
*/
class DScadaDevData : public SysCDRable
{
public:

	DScadaDevData()
	{
		this->clear();
	}

	~DScadaDevData()
	{

	}

public:

	void clear()
	{
		m_fieldName = EMPTY_STRING;
		m_devCode = EMPTY_STRING;
        m_devType = EMPTY_STRING;
		m_password = EMPTY_STRING;
		m_switchState = INVALID_NUMBER;
		m_pointVec.clear();
	}

public:

	SysString m_fieldName;

	SysString m_devCode;

    SysString m_devType;

	SysString m_password;
			 
	SysUInt m_switchState;

	std::vector<DScadaDevPoint> m_pointVec;

public:

	SYS_HRESULT marshalToCDR(SysCDROutputStreamPtr out)
	{
		HRESULT_INIT;

		if(out)
		{
			SysCDRStreamWriter wr(out);
            wr << m_fieldName;
			wr << m_devCode;
            wr << m_devType;
			wr << m_password;
			*out << m_switchState;

			SysUInt index = 0;
			SysUInt size = m_pointVec.size();
			*out << size;
			for (; index < size; index++)
			{
				m_pointVec[index].marshalToCDR(out);
			}
		}
		else
		{
			hr = TAS_SYS_RPC_MARSHAL_ERROR;
		}

		HRESULT_RETURN;
	}

	SYS_HRESULT unMarshalToObj(SysCDRInputStreamPtr in)
	{
		HRESULT_INIT;

		if(in)
		{
			SysCDRStreamReader rd(in);
            rd >> m_fieldName;
			rd >> m_devCode;
            rd >> m_devType;
			rd >> m_password;
			*in >> m_switchState;

			DScadaDevPoint dev_point;
			SysUInt size = 0;
			*in >> size;
			SysUInt index = 0;
			for (; index < size; index++)
			{
				dev_point.unMarshalToObj(in);
				m_pointVec.push_back(dev_point);
			}
		}
		else
		{
			hr = TAS_SYS_RPC_UNMARSHAL_ERROR;
		}

		HRESULT_RETURN;
	}
};



class DScadaSampleObj
{

public:	
	DScadaSampleObj();
	~DScadaSampleObj();

};

class DScadaDataPkg : public SysCDRable
{
public:
	SysString   m_devType;
	SysString	m_devCode;
	SysUInt		m_pkgType;
	SysULong    m_id;  ///<前置当前时间戳

	std::vector<DScadaData>  m_dataVec;

public:
	DScadaDataPkg():m_devType(""), m_id(SysOSUtils::getTimeNowMsec())
	{

	}


	SYS_HRESULT marshalToCDR(SysCDROutputStreamPtr out)
	{
		SYS_HRESULT hr = SYS_OK;
		if (out)
		{
			WRITE_STRING_IN_CDR(m_devType,out);
			SysUInt len = m_devCode.length();
			*out << len;
			out->write_char_array(m_devCode.c_str(), len);

			SysUInt size = m_dataVec.size();
			*out << size;
			for (SysUInt i = 0; i < size; i++)
			{
				m_dataVec[i].marshalToCDR(out);
			}

			*out << m_pkgType;
		}
		else
		{
			hr = TAS_SYS_RPC_MARSHAL_ERROR;
		}
		return hr;
	}

	SYS_HRESULT unMarshalToObj(SysCDRInputStreamPtr in)
	{
		SYS_HRESULT hr = SYS_OK;
		if (in)
		{
			READ_STRING_FROM_CDR(m_devType,in);
			SysUInt len = 0;
			*in >> len;
			m_devCode.resize(len);

			in->read_char_array(const_cast<SysChar*>(m_devCode.c_str()), len);

			SysUInt size = 0;
			*in >> size;
			m_dataVec.resize(size);
			for (SysUInt i = 0; i < size; i++)
			{
				m_dataVec[i].unMarshalToObj(in);
			}

			*in >> m_pkgType;
		}
		else
		{
			hr = TAS_SYS_RPC_UNMARSHAL_ERROR;
		}

		return hr;
	}
};


class DScadaCmdPkg : public SysCDRable
{
public:

	DScadaCmdPkg()
	{
		m_pkgType = INVALID_NUMBER;
		m_timestamp = INVALID_NUMBER;
		m_devCode = EMPTY_STRING;
        m_fieldName = EMPTY_STRING;
		m_isNeedLog = true;
	}

	~DScadaCmdPkg()
	{

	}

public:	
	SysUInt m_pkgType;
	SysULong m_timestamp;
	SysString m_operatorName;
	SysString m_monitorName;
	SysString m_operation;
	SysString m_devCode;
    SysString m_fieldName;
	SysString m_ipInfo;
	SysString m_operationTag;
	SysBool   m_isNeedLog;		
	std::vector<DScadaDevData> m_devVec;

public:

	SYS_HRESULT marshalToCDR(SysCDROutputStreamPtr out)
	{
		HRESULT_INIT;

		if (out)
		{
			WRITE_STRING_IN_CDR(m_devCode,out);
            WRITE_STRING_IN_CDR(m_fieldName,out);
			*out << m_pkgType;
			*out << m_timestamp;

			SysUInt index = 0;
			SysUInt size = m_devVec.size();

			*out << size;
			for (; index < size; index++)
			{
				m_devVec[index].marshalToCDR(out);
			}

		}
		else
		{
			hr = TAS_SYS_RPC_MARSHAL_ERROR;
		}

		HRESULT_RETURN;
	}

	SYS_HRESULT unMarshalToObj(SysCDRInputStreamPtr in)
	{
		HRESULT_INIT;

		if (in)
		{
			READ_STRING_FROM_CDR(m_devCode,in);
            READ_STRING_FROM_CDR(m_fieldName,in);
			*in >> m_pkgType;
			*in >> m_timestamp;

			SysUInt index = 0;
			SysUInt size = 0;

			*in >> size;
			m_devVec.resize(size);
			for (; index < size; index++)
			{
				m_devVec[index].unMarshalToObj(in);
			}
		}
		else
		{
			hr = TAS_SYS_RPC_UNMARSHAL_ERROR;
		}

		HRESULT_RETURN;
	}
};

class DScadaCmdRsp:public SysCDRable
{
public:

	DScadaCmdRsp():m_timestamp(0), m_hr(SYS_OK)
	{

	}

	SysULong m_timestamp;
	SYS_HRESULT m_hr;

	SysString m_error;


	SYS_HRESULT marshalToCDR(SysCDROutputStreamPtr out)
	{
		SYS_HRESULT hr = SYS_OK;
		*out << m_timestamp;
		*out << m_hr;

		WRITE_STRING_IN_CDR(m_error, out);

		return hr;
	}

	SYS_HRESULT unMarshalToObj(SysCDRInputStreamPtr in)
	{
		SYS_HRESULT  hr = SYS_OK;

		*in >> m_timestamp;
		*in >> m_hr;

		READ_STRING_FROM_CDR(m_error, in);

		return hr;
	}

};

class DScadaRspPkg : public SysCDRable
{
public:

	DScadaRspPkg()
	{
		m_pkgType = INVALID_NUMBER;
		m_hr = INVALID_NUMBER;
	}

	~DScadaRspPkg()
	{

	}

public:

	SysInt m_pkgType;

	SYS_HRESULT m_hr;
	
	DScadaDevPoint m_point;

public:

	SYS_HRESULT marshalToCDR(SysCDROutputStreamPtr out)
	{
		HRESULT_INIT;
		
		*out << m_pkgType;
		*out << m_hr;
		m_point.marshalToCDR(out);
		
		HRESULT_RETURN;
	}

	SYS_HRESULT unMarshalToObj(SysCDRInputStreamPtr in)
	{
		HRESULT_INIT;
		
		*in >> m_pkgType;
		*in >> m_hr;
		m_point.unMarshalToObj(in);
		
		HRESULT_RETURN;
	}
};



class AppDScadaSignal : public SysCDRable
{

public:
	enum
	{
		SIGNAL_NONE = 0,

		SIGNAL_TO_TAS_CLIENTS = 1,

		SIGNAL_TO_WX_ACCOUNTS = 2,
	};
public:

	SysULong	m_signalID;
	SysULong    m_faultID;
	SysTimet    m_occurDevTime;
	SysTimet    m_occurTasTime;
	SysUInt		m_phase;
	SysUInt     m_faultType;
	std::vector<SysFloat>	m_value;
	SysChar     m_status;  
	SysUInt		m_QuickBreakLimit;//速断定值
    SysUInt		m_QuickBreakDurance;//
	SysUInt		m_OverFlowLimit;//过流定值
	SysUInt		m_OverFlowDurance;
	SysUInt		m_transRatioFirst;
	SysUInt		m_transRatioSecond;
	SysInt		m_faultDirection;
	SysDouble   m_gpsLongitude;
	SysDouble   m_gpsLatitude;
	SysULong    m_specialAttribute;

    SysString   m_telNum;
    SysString   m_devCode;
    SysString   m_devType;
    SysString	m_aorID;
    SysString   m_aorName;
    SysString   m_lineID;
    SysString   m_lineName;
    SysString   m_projectID;
    SysString   m_projectName;
    SysString   m_areaID;
    SysString   m_areaName;
    SysString   m_pole;
    SysULong     m_dataOrigin;
//	std::vector<SysULong> m_faultRelatedDevIDVec;

public:

	AppDScadaSignal():  m_signalID(0),
						m_faultID(0),
						m_occurTasTime(0),
						m_occurDevTime(0),
						m_faultType(0),
						m_status(0),
						m_QuickBreakLimit(0),
                        m_QuickBreakDurance(0),
						m_OverFlowLimit(0),
						m_OverFlowDurance(0),
						m_transRatioFirst(0),
						m_transRatioSecond(0),
						m_faultDirection(0),
						m_gpsLongitude(0),
						m_gpsLatitude(0),
						m_specialAttribute(SIGNAL_TO_TAS_CLIENTS)
	{		
	}

public:

	SYS_HRESULT marshalToCDR(SysCDROutputStreamPtr out)
	{
		SYS_HRESULT hr = SYS_OK;
		if (out)
		{
			*out << m_signalID;
			*out << m_faultID;
			*out << m_occurDevTime;
			*out << m_occurTasTime;
			*out << m_phase;
			*out << m_faultType;
			*out << m_value.size();
			for (SysUInt i = 0; i < m_value.size(); i++)
			{
				*out << m_value[i];
			}
			*out << m_status;
			*out <<	m_QuickBreakLimit;
            *out << m_QuickBreakDurance;
			*out <<	m_OverFlowLimit;
			*out <<	m_OverFlowDurance;
			*out <<	m_transRatioFirst;
			*out <<	m_transRatioSecond;
			*out <<	m_faultDirection;
			*out << m_gpsLongitude;
			*out << m_gpsLatitude;
			*out << m_specialAttribute;

            *out << m_telNum.length();
            out->write_char_array(m_telNum.c_str(), m_telNum.length());
            *out << m_devCode.length();
            out->write_char_array(m_devCode.c_str(), m_devCode.length());
            WRITE_STRING_IN_CDR(m_devType,out);
            WRITE_STRING_IN_CDR(m_aorID,out);
            WRITE_STRING_IN_CDR(m_aorName,out);
            WRITE_STRING_IN_CDR(m_lineID,out);
            WRITE_STRING_IN_CDR(m_lineName,out);
            WRITE_STRING_IN_CDR(m_projectID,out);
            WRITE_STRING_IN_CDR(m_projectName,out);
            WRITE_STRING_IN_CDR(m_areaID,out);
            WRITE_STRING_IN_CDR(m_areaName,out);
            WRITE_STRING_IN_CDR(m_pole,out);			
		}
		else
		{
			hr = TAS_SYS_RPC_MARSHAL_ERROR;
		}
		return hr;
	}

	SYS_HRESULT unMarshalToObj(SysCDRInputStreamPtr in)
	{
		SYS_HRESULT hr = SYS_OK;
		if (in)
		{
            SysUInt size = 0;
			*in >> m_signalID;
			*in >> m_faultID;
			*in >> m_occurDevTime;
			*in >> m_occurTasTime;
			*in >> m_phase;
			*in >> m_faultType;
			
			size = 0;
			*in >> size;
			m_value.resize(size);
			for (SysUInt i = 0; i < size; i++)
			{
				*in >> m_value[i];
			}
			*in >> m_status;

			*in >>	m_QuickBreakLimit;
            *in >>  m_QuickBreakDurance;
			*in >>	m_OverFlowLimit;
			*in >>	m_OverFlowDurance;
			*in >>	m_transRatioFirst;
			*in >>	m_transRatioSecond;
			*in >>	m_faultDirection;
			*in >>  m_gpsLongitude;
			*in >>  m_gpsLatitude;
			*in >>  m_specialAttribute;

            *in >> size;
            m_telNum.resize(size);
            in->read_char_array(const_cast<SysChar*>(m_telNum.c_str()), size);

            size = 0;
            *in >> size;
            m_devCode.resize(size);
            in->read_char_array(const_cast<SysChar*>(m_devCode.c_str()), size);
            READ_STRING_FROM_CDR(m_devType,in);
            READ_STRING_FROM_CDR(m_aorID,in);
            READ_STRING_FROM_CDR(m_aorName,in);
            READ_STRING_FROM_CDR(m_lineID,in);
            READ_STRING_FROM_CDR(m_lineName,in);
            READ_STRING_FROM_CDR(m_projectID,in);
            READ_STRING_FROM_CDR(m_projectName,in);
            READ_STRING_FROM_CDR(m_areaID,in);
            READ_STRING_FROM_CDR(m_areaName,in);
            READ_STRING_FROM_CDR(m_pole,in);
		}
		else
		{
			hr = TAS_SYS_RPC_UNMARSHAL_ERROR;
		}

		return hr;
	}

	void accValueToString(SysString& accValue)
	{
		
		switch(m_faultType)
		{
		case  TAS_SYS_SOE:
			{
				break;
			}
		case  TAS_SHORT_WARNING_1:
			{
				if (isDropFuseDev(m_devCode))
				{
					accValue = "短路故障";
					break;
				}
				if (IS_XHQD_620(m_devType))
				{
					
					SysString tmp;
					SysOSUtils::floatToStr(m_QuickBreakLimit, tmp);
					accValue += "大于";
					accValue += tmp;
					accValue += "A";
				}
				else
				{
					SysString tmp;
					SysOSUtils::floatToStr(m_value[0], tmp);
					accValue += tmp;
					accValue += "A";
				}

				break;
			}
		case TAS_SHORT_WARNING_2:
			{
				if (isDropFuseDev(m_devCode))
				{
					accValue = "短路故障";
					break;
				}
				SysOSUtils::floatToStr(m_value[0], accValue);
				accValue += "A";
				break;
			}
		case TAS_SHORT_WARNING_3:
			{
				SysOSUtils::floatToStr(m_value[0], accValue);
				accValue += "A";
				break;
			}
		case TAS_GROUNDING_ALARM:  // FTU
			{
				if (m_value.size() > 0)
				{
					SysOSUtils::floatToStr(m_value[0], accValue);
					accValue += "A";
				}
				break;
			}
		case TAS_TINY_GROUNDING_LOST_VOLTAGE:
		{
			if (m_value.size() > 0)
			{
				SysOSUtils::floatToStr(m_value[0], accValue);
				accValue += "V";
			}
			break;
		}
		case TAS_GROUNDING_ALARM_PDI: // PDI
			{
				if (NO_FAULT == m_phase)
				{
					accValue = "无接地.";
				}
				if (m_value.size() >= 5)
				{
					accValue += "异频Ia:";
					SysString tmp;
					SysOSUtils::floatToStr(m_value[0], tmp, "%.3f");
					accValue += tmp;
					accValue += "A 异频Ib:";
					SysOSUtils::floatToStr(m_value[1], tmp, "%.3f");
					accValue += tmp;
					accValue += "A 异频Ic:";
					SysOSUtils::floatToStr(m_value[2], tmp, "%.3f");
					accValue += tmp;

					accValue += "A Uc: ";
					tmp = "";
					SysOSUtils::floatToStr(m_value[3], tmp, "%.2f");
					tmp += "V ";
					accValue += tmp;
				}
				break;
			}
		case TAS_GROUNDING_ALARM_SFS:
			{
				SysOSUtils::floatToStr(m_value[0], accValue);
				accValue += "A";
				break;
			}
		case TAS_RECLOSING_FAILED:
			{
				accValue = "重合闸失败.";

				if (m_status)
				{
					accValue += "状态：闭锁.";
				}
				else
				{
					accValue += "状态：正常.";
				}
				break;
			}
		case TAS_RECLOSING_SUCCESS:
			{
				accValue = "重合闸成功.";
				break;
			}
		// 104 protocol

		case TAS_DROP_FUSE_OFF:
			{
				SysString phase;
				getFaultPhaseStr(m_phase, phase);
				accValue = phase +"异常跌落";
				break;
			}
		case TAS_DROP_FUSE_SC_OFF:
			{
				SysString phase;
				getFaultPhaseStr(m_phase, phase);
				accValue = phase + "故障跌落";
				break;
			}

		default:
			{
				if (m_value.size() > 0)
				{
					SysOSUtils::floatToStr(m_value[0], accValue);
				}
				break;
			}
		}
		
	 }

    void signalToBuffer(SysChar* bufferPtr, SysUInt bufferSize);

	void bufferToSignal(SysChar* bufferPtr, SysUInt bufferSize, AppDScadaSignal & signalobj);

	SysBool isSCFault();

	SysBool isGroundingFault();

	SYS_HRESULT toWarnMessage(SysString& msg);

};

class AppDataPersistUnit
{
public:
	SysTimet		m_tasTimeStamp;
	SysTimet		m_devTimeStamp;
	SysString       m_fieldName;
	SysRtdbAny		m_value;
};

class AppDataPersisitPkg
{
public:

	enum
	{
		TAS_NONE_TYPE = 0,

		TAS_SAMPLE_DATA = 0x01,

        TAS_MODEL_DATA = 0x02,

		TAS_SIGNAL_DATA = 0x04,

        TAS_FAULT_DATA = 0x08,

        TAS_SMS_MSG = 0x10,

		TAS_SUPPORT_SkyKeeper= 0x12,
	};

	AppDataPersisitPkg():pkgType(TAS_NONE_TYPE),devCode(""),devType("")
	{

	}

	SysString	devCode;
    SysString   devType;
    SysString   lineName;
    SysString   lineID;
    SysString   installPole;
    SysString   desc;
	SysUInt		pkgType;
    SysULong    m_timeStamp;


	std::vector<AppDataPersistUnit> sampleDataVec; //to his or rt
	std::vector<AppDataPersistUnit> persistDataVec; //to DB
    std::vector<SysString> sqlVec; //soe or Fault sql
};

#endif