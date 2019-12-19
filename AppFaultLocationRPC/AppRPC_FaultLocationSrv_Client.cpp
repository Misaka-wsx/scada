#include "StdAfx.h"
#include "AppRPC_FaultLocationSrv_Client.h"
SYS_CLASS_LOGGER_DEF(AppRPC_FaultLocationSrv_Client);
SYS_HRESULT AppRPC_FaultLocationSrv_Client::refreshMaster()
{
    SYS_HRESULT hr = SYS_OK;
    ACE_INET_Addr addr;
    hr = SysSrvManager::getInstance()->getServiceMasterByName(SysString(APP_DSCADA_FAULT_LOCATION_SERVICE), addr);
    if (SYS_OK != hr)
    {
        return hr;
    }
    else
    {
        if (m_currentSrvAddr != addr)
        {
            m_currentSrvAddr = addr;
            hr = connectToFaultLocationSrv();
        }
    }
    return hr;
}

SYS_HRESULT AppRPC_FaultLocationSrv_Client::reConnectMaster()
{
    SYS_HRESULT hr = SYS_OK;

    m_stub.stubCloseChannel();

    hr = m_stub.stubConnect(m_currentSrvAddr);

    return hr;
}

SYS_HRESULT AppRPC_FaultLocationSrv_Client::connectToFaultLocationSrv()
{
    SYS_HRESULT hr = SYS_OK;
    m_stub.init(m_currentSrvAddr);
    hr = m_stub.stubConnect(m_currentSrvAddr);
    return hr;
}

SYS_HRESULT AppRPC_FaultLocationSrv_Client::processDisconnect( SYS_HRESULT outHr )
{
    SYS_HRESULT hr = SYS_OK;
    if (SysOSUtils::isDisconnect(outHr))
    {
        hr = reConnectMaster();
        return hr;
    }
    return hr;
}

SYS_HRESULT AppRPC_FaultLocationSrv_Client::deliverAppDScadaSignalData( AppRPC_FaultLocationSrv_Meta &faultSignal )
{
    SYS_HRESULT hr = SYS_OK;
    AppRPC_FaultLocationSrv_Meta rsp;
    hr = m_stub.deliverAppDScadaSignalData(faultSignal,rsp);
    return hr;
}


SYS_HRESULT AppRPC_FaultLocationSrv_Client::deliverAppDScadaSignalData_R(AppDScadaSignal& faultSingle)
{
	SYS_HRESULT hr = SYS_OK;

	AppFaultLocationRsp rsp;
	return m_stub.deliverAppDScadaSignalData_R(faultSingle, rsp);
}

