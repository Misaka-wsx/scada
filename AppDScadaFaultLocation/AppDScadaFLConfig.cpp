
#include	"SysUtils.h"
#include    "AppDScadaFLConfig.h"
#include    "SysXmlConfig.h"

SYS_CLASS_LOGGER_DEF(AppScadaFLConfiguration);


SYS_HRESULT AppScadaFLConfiguration::getParams(SysString& nodeName, SysRtdbAny& nodeValue, RTDB_DATA_TYPE valueType)
{
	SYS_HRESULT hr = SYS_OK;
	SysString configFilePath = TAS_BACKEND_CONFIG;
	SysXmlConfig config;
	hr = config.openConfigFile(configFilePath);
	if ( SYS_OK != hr)
	{
		SYS_LOG_MESSAGE(m_logger, ll_error, "Open Config file error. hr : " << hr);
		return hr;
	}
	XML_NODE_PATH nodePath;
	nodePath.push_back(SysString("AppDScadaFaultLocation"));
	//nodePath.push_back(nodeName);

	SysString valueStr;
	hr = config.locatToNode(nodePath); 
	if ( SYS_OK != hr)
	{
		SYS_LOG_MESSAGE(m_logger, ll_error, "Parse configuration file error when get : " << nodeName.c_str());
		return hr;
	}
	hr = config.getNodeValue(nodeName, valueStr);
	if (SYS_OK  != hr)
	{
		return hr;
	}
	switch(valueType)
	{
	case SYS_RTDB_INT:
	case SYS_RTDB_UINT:
		{
			SysInt tmp = 0;
			tmp = atoi(valueStr.c_str());
			nodeValue = tmp;
			break;
		}
	case SYS_RTDB_FLOAT:
		{
			SysFloat tmp = 0.0f;
			tmp = atof(valueStr.c_str());
			nodeValue = tmp;
			break;
		}
    case SYS_RTDB_CHAR_PTR:
        {
            nodeValue.m_dataLen = valueStr.size();
            nodeValue.m_dataValue.valueCharPtr = new SysChar[nodeValue.m_dataLen+1]();
            strcpy(nodeValue.m_dataValue.valueCharPtr, valueStr.c_str());
            break;
        }
	default:
		{
			nodeValue = -1;
			break;
		}
	}

	return hr;
}


SYS_HRESULT AppScadaFLConfiguration::getConfigStr(SysString& nodeName, SysString& nodeValue)
{
    SYS_HRESULT hr = SYS_OK;
    SysString configFilePath = TAS_BACKEND_CONFIG;
    SysXmlConfig config;
    hr = config.openConfigFile(configFilePath);
    if ( SYS_OK != hr)
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "Open Config file error. hr : " << hr);
        return hr;
    }
    XML_NODE_PATH nodePath;
    nodePath.push_back(SysString("AppDScadaFaultLocation"));
    hr = config.locatToNode(nodePath); 
    if ( SYS_OK != hr)
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "Parse configuration file error when get : " << nodeName.c_str());
        return hr;
    }
    hr = config.getNodeValue(nodeName, nodeValue);
    if (SYS_OK  != hr)
    {
        return hr;
    }
    return hr;
}


void AppScadaFLConfiguration::showParameters()
{
	SYS_LOG_MESSAGE(m_logger, ll_waring, "SC_MAX_VALID_SIGNAL_TIMEOUT : " << Sc_MaxWaitValidSignalTimeout);
	SYS_LOG_MESSAGE(m_logger, ll_waring, "SC_MAX_INVALID_SIGNAL_TIMEOUT : " << Sc_MaxWaitInvalidSignalTimeout);
	SYS_LOG_MESSAGE(m_logger, ll_waring, "GR_MAX_VALID_SIGNAL_TIMEOUT : " << Gr_MaxWaitValidSignalTimeout);
	SYS_LOG_MESSAGE(m_logger, ll_waring, "GR_MAX_INVALID_SIGNAL_TIMEOUT : " << Gr_MaxWaitInvalidSignalTimeout);
	SYS_LOG_MESSAGE(m_logger, ll_waring, "GR_MAX_UC_DATA_TIMEOUT : " << Gr_MaxWaitUcRtDataTimeout);
	SYS_LOG_MESSAGE(m_logger, ll_waring, "GR_FTU_ZERO_CURRENT_MIN_LIMIT : " << Gr_MinFtuZeroCurrent);
	SYS_LOG_MESSAGE(m_logger, ll_waring, "GR_NORMAL_DEBICE_RATIO : " << Gr_NormalUcDevRatio);
	SYS_LOG_MESSAGE(m_logger, ll_waring, "DEF_GR_DENIAL_SIGNAL_DOE: " << Gr_MaxDenialDOE);
	SYS_LOG_MESSAGE(m_logger, ll_waring, "IS_STATUS_REPORT: " << isBreakerStatusReport);
    SYS_LOG_MESSAGE(m_logger, ll_waring, "SMS_DB_SERVER_IP: " << SMS_DBSrv_IP);
    SYS_LOG_MESSAGE(m_logger, ll_waring, "SMS_DB_SERVER_PORT:" << SMS_DBSrv_Port);
}


SYS_HRESULT AppScadaFLConfiguration::initConfiguration()
{
	SYS_HRESULT hr = SYS_OK;
	SysRtdbAny nodeValue;
	SysFloat tmpValue = 0.0f;
	hr = getParams(SysString("GR_FTU_ZERO_CURRENT_MIN_LIMIT"), nodeValue, SYS_RTDB_FLOAT);
	if (SYS_OK == hr)
	{
		tmpValue = nodeValue.m_dataValue.valueFloat;
		if (tmpValue > Gr_MinFtuZeroCurrent)
		{
			Gr_MinFtuZeroCurrent = tmpValue;
		}
	}
	else
	{
		SYS_LOG_MESSAGE(m_logger, ll_waring, "Read parameter: FTU_ZERO_CURRENT_MIN_LIMIT failed. HR : " << hr);
	}

	hr = getParams(SysString("GR_NORMAL_DEBICE_RATIO"), nodeValue, SYS_RTDB_FLOAT);
	if (SYS_OK == hr)
	{
		tmpValue = nodeValue.m_dataValue.valueFloat;
		if ((tmpValue > 1.0f) || (tmpValue <= 0))
		{
			tmpValue = DEF_NORMAL_DEV_RATIO;
		}
		Gr_NormalUcDevRatio = tmpValue;
	}
	else
	{
		SYS_LOG_MESSAGE(m_logger, ll_waring, "Read parameter: DEF_NORMAL_DEV_RATIO failed. HR : " << hr);
	}

	hr = getParams(SysString("GR_MAX_VALID_SIGNAL_TIMEOUT"), nodeValue, SYS_RTDB_INT);
	if (SYS_OK == hr)
	{
		tmpValue = nodeValue.m_dataValue.valueUInt;
		(tmpValue > 600)?(tmpValue = DEF_GR_MAX_WAIT_VALID_SIGNAL_TIMEOUT):(tmpValue *= 1000);
		Gr_MaxWaitValidSignalTimeout = tmpValue;
	}
	else
	{
		SYS_LOG_MESSAGE(m_logger, ll_waring, "Read parameter: GR_MAX_VALID_SIGNAL_TIMEOUT failed. HR : " << hr);
	}

	hr = getParams(SysString("GR_MAX_INVALID_SIGNAL_TIMEOUT"), nodeValue, SYS_RTDB_INT);
	if (SYS_OK == hr)
	{
		tmpValue = nodeValue.m_dataValue.valueUInt;
		(tmpValue > 60)?(tmpValue = DEF_GR_MAX_WAIT_INVALID_SIGNAL_TIMEOUT):(tmpValue *= 1000);
		Gr_MaxWaitInvalidSignalTimeout = tmpValue;
	}
	else
	{
		SYS_LOG_MESSAGE(m_logger, ll_waring, "Read parameter: GR_MAX_INVALID_SIGNAL_TIMEOUT failed. HR : " << hr);
	}

	hr = getParams(SysString("GR_MAX_UC_DATA_TIMEOUT"), nodeValue, SYS_RTDB_INT);
	if (SYS_OK == hr)
	{
		tmpValue = nodeValue.m_dataValue.valueUInt;
		(tmpValue > 60)?(tmpValue = DEF_GR_MAX_WAIT_UC_TIMEOUT):(tmpValue *= 1000);
		Gr_MaxWaitUcRtDataTimeout = tmpValue;
	}
	else
	{
		SYS_LOG_MESSAGE(m_logger, ll_waring, "Read parameter: GR_MAX_UC_DATA_TIMEOUT failed. HR : " << hr);
	}

	// SC fault.
	hr = getParams(SysString("SC_MAX_VALID_SIGNAL_TIMEOUT"), nodeValue, SYS_RTDB_INT);
	if (SYS_OK == hr)
	{
		tmpValue = nodeValue.m_dataValue.valueUInt;
		(tmpValue > 300)?(tmpValue = DEF_SC_MAX_WAIT_VALID_SIGNAL_TIMEOUT):(tmpValue *= 1000); // 5 mins
		Sc_MaxWaitValidSignalTimeout = tmpValue;
	}
	else
	{
		SYS_LOG_MESSAGE(m_logger, ll_waring, "Read parameter: SC_MAX_VALID_SIGNAL_TIMEOUT failed. HR : " << hr);
	}

	hr = getParams(SysString("SC_MAX_INVALID_SIGNAL_TIMEOUT"), nodeValue, SYS_RTDB_INT);
	if (SYS_OK == hr)
	{
		tmpValue = nodeValue.m_dataValue.valueUInt;
		(tmpValue > 60)?(tmpValue = DEF_SC_MAX_WAIT_INVALID_SIGNAL_TIMEOUT):(tmpValue *= 1000); // 1 mins
		Sc_MaxWaitInvalidSignalTimeout = tmpValue;
	}
	else
	{
		SYS_LOG_MESSAGE(m_logger, ll_waring, "Read parameter: DEF_SC_MAX_WAIT_INVALID_SIGNAL_TIMEOUT failed. HR : " << hr);
	}

	hr = getParams(SysString("GR_TEMPORARY_AS_SIGNAL"), nodeValue, SYS_RTDB_INT);
	if (SYS_OK == hr)
	{
		tmpValue = nodeValue.m_dataValue.valueUInt;
		if (tmpValue != 0)
		{
			tmpValue = 1;
		}
		Gr_TemporaryAsSignal = tmpValue;
	}
	else
	{
		SYS_LOG_MESSAGE(m_logger, ll_waring, "Read parameter: GR_TEMPORARY_AS_SIGNAL failed. HR : " << hr);
	}

	nodeValue.m_dataType = SYS_RTDB_INVALID;
	hr = getParams(SysString("DEF_GR_DENIAL_SIGNAL_DOE"), nodeValue, SYS_RTDB_INT);
	if (SYS_OK == hr)
	{
		tmpValue = nodeValue.m_dataValue.valueUInt;
		Gr_MaxDenialDOE = tmpValue;
	}
	else
	{
		SYS_LOG_MESSAGE(m_logger, ll_waring, "Read parameter: DEF_GR_DENIAL_SIGNAL_DOE failed. HR : " << hr);
	}

    nodeValue.m_dataType = SYS_RTDB_INVALID;
    hr = getParams(SysString("MIN_ONLINE_RATE"), nodeValue, SYS_RTDB_INT);
    if (SYS_OK == hr)
    {
        tmpValue = nodeValue.m_dataValue.valueUInt;
        Min_Online_Rate = tmpValue;
    }
    else
    {
        SYS_LOG_MESSAGE(m_logger, ll_waring, "Read parameter: DEF_GR_DENIAL_SIGNAL_DOE failed. HR : " << hr);
    }

    hr = getConfigStr(SysString("ADMINISTRATOR_NAME"), Administrator_Name);
    if (SYS_OK != hr)
    {
        SYS_LOG_MESSAGE(m_logger, ll_waring, "Read parameter: DEF_GR_DENIAL_SIGNAL_DOE failed. HR : " << hr);
    }

    hr = getConfigStr(SysString("ADMINISTRATOR_PHONE_NUMBER"), Administrator_Phone_Number);
    if (SYS_OK != hr)
    {
        SYS_LOG_MESSAGE(m_logger, ll_waring, "Read parameter: DEF_GR_DENIAL_SIGNAL_DOE failed. HR : " << hr);
    }
	// 
	nodeValue.m_dataType = SYS_RTDB_INVALID;
	hr = getParams(SysString("IS_REPORT_BREAKER_STATUS"), nodeValue, SYS_RTDB_INT);
	if (SYS_OK == hr)
	{
		tmpValue = nodeValue.m_dataValue.valueInt;
		if (1 == tmpValue)
		{
			isBreakerStatusReport = true;
		}
		else
		{
			isBreakerStatusReport = false;
		}
	}
    else
    {
        SYS_LOG_MESSAGE(m_logger, ll_waring, "Read parameter: IS_REPORT_BREAKER_STATUS failed. HR : " << hr);
    }

	nodeValue.m_dataType = SYS_RTDB_INVALID;
	hr = getParams(SysString("IS_GR_DEVICE_BRAND"), nodeValue, SYS_RTDB_INT);
	if (SYS_OK == hr)
	{
		tmpValue = nodeValue.m_dataValue.valueInt;
		if (1 == tmpValue)
		{
			isGRDeviceBrand = true;
		}
		else
		{
			isGRDeviceBrand = false;
		}
	}
	else
	{
		SYS_LOG_MESSAGE(m_logger, ll_waring, "Read parameter: IS_GR_DEVICE_BRAND failed. HR : " << hr);
	}

    nodeValue.m_dataType = SYS_RTDB_INVALID;
    hr = getParams(SysString("SMS_DB_SERVER_IP"), nodeValue, SYS_RTDB_CHAR_PTR);
    if (SYS_OK == hr)
    {
        SMS_DBSrv_IP = nodeValue.m_dataValue.valueCharPtr;
        delete[] nodeValue.m_dataValue.valueCharPtr;
    }
    else
    {
        SYS_LOG_MESSAGE(m_logger, ll_waring, "Read paramter: SMS_DB_SERVER_IP failed. HR : " << hr);
    }

    nodeValue.m_dataType = SYS_RTDB_INVALID;
    hr = getParams(SysString("SMS_DB_SERVER_PORT"), nodeValue, SYS_RTDB_CHAR_PTR);
    if (SYS_OK == hr)
    {
        SMS_DBSrv_Port = nodeValue.m_dataValue.valueCharPtr;
        delete[] nodeValue.m_dataValue.valueCharPtr;
    }
    else
    {
        SYS_LOG_MESSAGE(m_logger, ll_waring, "Read paramter: SMS_DB_SERVER_PORT failed. HR : " << hr);
    }

	showParameters();
	return hr;
}