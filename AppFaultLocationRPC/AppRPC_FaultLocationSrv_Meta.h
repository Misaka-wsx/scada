#ifndef APPRPC_FAULTLOCATIONSRV_META_H
#define APPRPC_FAULTLOCATIONSRV_META_H
#include "SysUtils.h"
#include    "AppDScadaComData.h"
#include "SysRPCBaseCall.h"

//RPC传输数据接口类定义
class AppRPC_FaultLocationSrv_Meta :public SysCDRable
{
public:

    AppDScadaSignal m_signal;

    SYS_HRESULT marshalToCDR(SysCDROutputStreamPtr out);

    SYS_HRESULT unMarshalToObj(SysCDRInputStreamPtr in);
};



class AppFaultLocationRsp: public SysCDRable
{
public:

	SYS_HRESULT  callRsp;

	SYS_HRESULT marshalToCDR(SysCDROutputStreamPtr out);

	SYS_HRESULT unMarshalToObj(SysCDRInputStreamPtr in);

};
#endif