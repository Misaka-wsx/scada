#include "AppRPC_FaultLocationSrv_Server.h"

SYS_CLASS_LOGGER_DEF(AppRPC_FaultLocationSrv_Server);

#define  SYS_RPC_CALL_FUNCTION(func)	hr = func(req, rsp); \
    upCall.m_returnValue = hr; \
    upCall.m_compeleteStatus = TAS_SYS_RPC_COMPELETE;\
    upCall.marshalToCDR(upCall.m_out);\
    rsp.marshalToCDR(upCall.m_out);\

#define IS_FUNCTION(name)	0 == strncmp(name, upCall.m_callFuncName, SYS_RPC_CALL_MAX_LEN)	

#define	IS_MOUDLE(name)	    (m_moudleName == SysString(upCall.m_moudleName))

SYS_HRESULT AppRPC_FaultLocationSrv_Server::init(SysRPCProxyAttr & attr)
{
    SYS_HRESULT hr = SYS_OK;

    ACE_NEW_NORETURN(m_proxy, SysRPCProxy(attr.m_addr));
    if (NULL == m_proxy)
    {
        return BAD_ALLOC_MEM;
    }

    hr = m_proxy->init(attr);
    if (SYS_OK != hr)
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "Init proxy failed. errno: " << hr);
        return hr;
    }

    m_proxy->setThreadNum(attr.m_threads);
    m_proxy->setSkeleton(this);

    return hr;
}

SYS_HRESULT AppRPC_FaultLocationSrv_Server::start()
{
    SYS_HRESULT hr = SYS_OK;

    hr = m_proxy->start();

    return hr;
}

SYS_HRESULT AppRPC_FaultLocationSrv_Server::dispatch(SysRPCMgsBlock & upCall)
{
    SYS_HRESULT hr = SYS_OK;
    if (IS_MOUDLE(m_moduleName.c_str()))
    {
        SYS_LOG_MESSAGE(m_logger, ll_debug, "CALL FUNC: " << upCall.m_callFuncName << ". CALL FROM: " << upCall.m_ipInfo.c_str());
        if (IS_FUNCTION("deliverAppDScadaSignalData"))
        {
            AppRPC_FaultLocationSrv_Meta req;
            AppRPC_FaultLocationSrv_Meta rsp;

            hr = req.unMarshalToObj(upCall.m_in);
            if (SYS_OK != hr)
            {
                return hr;
            }

            SYS_RPC_CALL_FUNCTION(deliverAppDScadaSignalData);
        }
		else if (IS_FUNCTION("deliverAppDScadaSignalData_R"))
		{
			AppDScadaSignal  req;
			AppFaultLocationRsp rsp;
			
			hr = req.unMarshalToObj(upCall.m_in);
			if(SYS_OK != hr)
			{
				return hr;
			}
			SYS_RPC_CALL_FUNCTION(deliverAppDScadaSignalData_R);
		}
    }

    return hr;
}

SYS_HRESULT AppRPC_FaultLocationSrv_Server::deliverAppDScadaSignalData(AppRPC_FaultLocationSrv_Meta& req, AppRPC_FaultLocationSrv_Meta& rsp)
{
    SYS_HRESULT hr = SYS_OK;
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, m_guard, -1);
    AppDScadaSignal *pSignal = new AppDScadaSignal;
    *pSignal = req.m_signal;

    if (pSignal->m_devType == "05") // 小T上送信号
    {
        AppDScadaTinyWorkerMgr::getInstance()->m_queue.putQ(pSignal);
    }
    else if (pSignal->m_devType == "11") // 跌落保险上送信号
    {
        AppDScadaFl4DFWorkerMgr::getInstance()->m_queue.putQ(pSignal);
    }
    else
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "未知设备类型 devType:" << pSignal->m_devType);
        delete pSignal;
    }

    return hr;
}


SYS_HRESULT AppRPC_FaultLocationSrv_Server::deliverAppDScadaSignalData_R(AppDScadaSignal& req, AppFaultLocationRsp& rsp)
{
	SYS_HRESULT hr = SYS_OK;
	
	AppDScadaSignal *pSignal = NULL;
	rsp.callRsp = SYS_FAIL;
	SYS_NEW_RETURN(pSignal, AppDScadaSignal, m_logger);
	*pSignal = req;
	AppDScadaTinyWorkerMgr::getInstance()->m_queue.putQ(pSignal);
	rsp.callRsp = SYS_OK;
	return hr;
}
