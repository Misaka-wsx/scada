#ifndef APPRPC_FAULTLOCATIONSRV_CLIENT_H
#define APPRPC_FAULTLOCATIONSRV_CLIENT_H
#include "AppRPC_FaultLocationSrv_Stub.h"
#include "SysServiceMgr.h"

class AppRPC_FaultLocationSrv_Client
{
public:
    AppRPC_FaultLocationSrv_Client(void) {};

    AppRPC_FaultLocationSrv_Client(ACE_INET_Addr& addr, SysUInt mode = SysRPCStubCore::SYS_RPC_AUTO_RECONNECT) { m_currentSrvAddr = addr;}

    ~AppRPC_FaultLocationSrv_Client(void) {};

    SYS_HRESULT refreshMaster();

    SYS_HRESULT processDisconnect(SYS_HRESULT hr);

    SYS_HRESULT deliverAppDScadaSignalData(AppRPC_FaultLocationSrv_Meta &faultSignal);

	SYS_HRESULT deliverAppDScadaSignalData_R(AppDScadaSignal& faultSingle);

private:

    SYS_HRESULT reConnectMaster();

    SYS_HRESULT connectToFaultLocationSrv();

private:

    ACE_INET_Addr m_currentSrvAddr;
    ACE_Recursive_Thread_Mutex   m_guard;
    AppRPC_FaultLocationSrv_Stub m_stub;
    SYS_CLASS_LOGGER;
};
#endif
