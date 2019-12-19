#ifndef APPRPC_FAULTLOCATIONSRV_STUB_H
#define APPRPC_FAULTLOCATIONSRV_STUB_H
#include "SysUtils.h"
#include "SysRPC.h"
#include "AppRPC_FaultLocationSrv_Meta.h"

class AppRPC_FaultLocationSrv_Stub : public SysRPCStubCore
{
public:

    AppRPC_FaultLocationSrv_Stub(SysUInt mode = SysRPCStubCore::SYS_RPC_AUTO_RECONNECT) {m_mode = mode;}

    ~AppRPC_FaultLocationSrv_Stub(void){}

    SYS_HRESULT init(ACE_INET_Addr& addr);

    SYS_HRESULT deliverAppDScadaSignalData(AppRPC_FaultLocationSrv_Meta& req, AppRPC_FaultLocationSrv_Meta& rsp);

	SYS_HRESULT deliverAppDScadaSignalData_R(AppDScadaSignal& req, AppFaultLocationRsp& rsp);

private:

    SYS_HRESULT callStubImpl(SysString& funcName, SysCDRable& req, SysCDRable& rsp);

private:

    ACE_INET_Addr   m_srvLocation;

    static SysString m_moduleName;

    SYS_CLASS_LOGGER;
};
#endif