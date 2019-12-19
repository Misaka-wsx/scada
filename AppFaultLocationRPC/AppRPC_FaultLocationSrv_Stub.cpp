#include "AppRPC_FaultLocationSrv_Stub.h"
#include "SysFuncCall.h"
SYS_CLASS_LOGGER_DEF(AppRPC_FaultLocationSrv_Stub);

SysString AppRPC_FaultLocationSrv_Stub::m_moduleName = "AppRPC_FaultLocationSrv_Server";
SYS_HRESULT AppRPC_FaultLocationSrv_Stub::init(ACE_INET_Addr& addr)
{
    SYS_HRESULT hr = SYS_OK;
    m_srvLocation = addr;
    return hr;
}

SYS_HRESULT AppRPC_FaultLocationSrv_Stub::deliverAppDScadaSignalData( AppRPC_FaultLocationSrv_Meta& req, AppRPC_FaultLocationSrv_Meta& rsp )
{
    SYS_HRESULT hr = SYS_OK;
    static SysChar funcName[] = "deliverAppDScadaSignalData";

    hr = callStubImpl(SysString(funcName), req, rsp);
    if (SYS_OK != hr)
    {
        SYS_LOG_MESSAGE(m_logger, ll_waring, "RPC failed. Function : " << funcName << " HR code : " << hr);
        PROCESS_DISCONNECT_RETRY(hr, m_logger);
    }
    return hr;
}


SYS_HRESULT AppRPC_FaultLocationSrv_Stub::deliverAppDScadaSignalData_R(AppDScadaSignal& req, AppFaultLocationRsp& rsp)
{
	SYS_HRESULT hr = SYS_OK;
	static SysChar funcName[] = "deliverAppDScadaSignalData_R";

	hr = callStubImpl(SysString(funcName), req, rsp);
	if (SYS_OK != hr)
	{
		SYS_LOG_MESSAGE(m_logger, ll_waring, "RPC failed. Function : " << funcName << " HR code : " << hr);
		PROCESS_DISCONNECT_RETRY(hr, m_logger);
	}
	return hr;
}

SYS_HRESULT AppRPC_FaultLocationSrv_Stub::callStubImpl(SysString& funcName, SysCDRable& req, SysCDRable& rsp)
{
    SYS_HRESULT hr = SYS_OK;

    SysCDROutputStream out;
    SysCDRInputStreamPtr inPtr = NULL;
    SysUInt transferred = 0;
    SysRPCMgsBlock msgOut;
    SysRPCMgsBlock msgIn;
    ACE_Time_Value timeOut(1, 0);

    msgOut.m_callDirection = SYS_RPC_CALL_CLIENT_TO_SRV;
    SET_STR_WITH_LEN(msgOut.m_moudleName, m_moduleName.c_str(), SYS_RPC_CALL_MAX_LEN);
    SET_STR_WITH_LEN(msgOut.m_callFuncName, funcName.c_str(), SYS_RPC_CALL_MAX_LEN);
    SYS_CALL_LOG_ERROR(msgOut.marshalToCDR(&out), m_logger ,ll_error ,"RPC MsgBlock Marshal to CDR error.", true, TAS_SYS_RPC_MARSHAL_ERROR);

    SYS_CALL_LOG_ERROR(req.marshalToCDR(&out), m_logger , ll_error ,"Req Marshal to CDR error.", true, TAS_SYS_RPC_MARSHAL_ERROR);

    SYS_CALL_LOG_ERROR(stubSend(&out, transferred), m_logger ,ll_error,"Send data to server error. errno: " << errno, true, hr);

    hr = stubRecv(inPtr,  NULL);
    if (SYS_OK != hr)
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "Recv data to server error. errno : " << errno);
        return hr;
    }

    hr = msgIn.unMarshalToObj(inPtr);
    if (SYS_OK != hr)
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "Recv data to server error. errno : " << errno);
        return hr;
    }

    RPC_CALL_EQUAL(msgIn, msgOut);
    if (SYS_OK != hr)
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "Rsp and Req not match");
        return hr;
    }

    hr = rsp.unMarshalToObj(inPtr);
    if (SYS_OK != hr)
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "Unmarshal error.");
        return hr;
    }

    if (inPtr)
    {
        ACE_Message_Block * normalPtr = const_cast<ACE_Message_Block*>(inPtr->start());
        normalPtr->clr_flags(ACE_Message_Block::DONT_DELETE);
        delete inPtr;
        inPtr = NULL;
    }

    return msgIn.m_returnValue;
}