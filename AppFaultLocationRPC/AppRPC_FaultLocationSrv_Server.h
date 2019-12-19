#ifndef APPRPC_FAULTLOCATIONSRV_SERVER_H
#define APPRPC_FAULTLOCATIONSRV_SERVER_H

#include    "SysRPC.h"
#include    "AppDScadaComData.h"
#include    "AppDScadaFL4DF.h"
#include    "AppDScadaFL4Tiny.h"
#include    "AppRPC_FaultLocationSrv_Meta.h"
class AppRPC_FaultLocationSrv_Server : public SysRPCSkeletonBase
{
public:

    AppRPC_FaultLocationSrv_Server(SysString& name): m_moudleName(name),
        m_proxy(NULL)
    {

    }

    ~AppRPC_FaultLocationSrv_Server()
    {

    }

    SYS_HRESULT regSkeleton(SysRPCSkeletonBase *);

    SYS_HRESULT dispatch(SysRPCMgsBlock & upcall);

    SysString &getModuleName()
    {
        return m_moudleName;
    }

    void setModuleName(SysString& name)
    {
        m_moudleName = name;
    }

    SYS_HRESULT init(SysRPCProxyAttr &);

    SYS_HRESULT start();

    void setProxy(SysRPCProxy *proxy)
    {
        if (m_proxy)
        {
            m_proxy->release();
        }
        m_proxy = proxy;
    }

    SysRPCProxy* getProxy()
    {
        return m_proxy;
    }

    SYS_HRESULT deliverAppDScadaSignalData(AppRPC_FaultLocationSrv_Meta& req, AppRPC_FaultLocationSrv_Meta& rsp);

	SYS_HRESULT deliverAppDScadaSignalData_R(AppDScadaSignal & req, AppFaultLocationRsp& rsp);

private:

    SysRPCProxy * m_proxy;

    SysString m_moudleName;

    SYS_CLASS_LOGGER;

    ACE_Recursive_Thread_Mutex m_guard;
};
#endif