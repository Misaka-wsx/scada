#include "AppRPC_FaultLocationSrv_Meta.h"

SYS_HRESULT AppRPC_FaultLocationSrv_Meta::marshalToCDR(SysCDROutputStreamPtr out)
{
    SYS_HRESULT hr = SYS_OK;

    if (out)
    {
        m_signal.marshalToCDR(out);
    }
    else
    {
        hr = TAS_SYS_RPC_MARSHAL_ERROR;
    }

    return hr;
}

SYS_HRESULT AppRPC_FaultLocationSrv_Meta::unMarshalToObj(SysCDRInputStreamPtr in)
{
    SYS_HRESULT hr = SYS_OK;

    if (in)
    {
        m_signal.unMarshalToObj(in);
    }
    else
    {
        hr = TAS_SYS_RPC_MARSHAL_ERROR;
    }

    return hr;
}




SYS_HRESULT AppFaultLocationRsp::marshalToCDR(SysCDROutputStreamPtr out)
{
	SYS_HRESULT hr = SYS_OK;
	// todo

	return hr;
}

SYS_HRESULT AppFaultLocationRsp::unMarshalToObj(SysCDRInputStreamPtr in)
{
	SYS_HRESULT hr = SYS_OK;
	// todo

	return hr;
}
