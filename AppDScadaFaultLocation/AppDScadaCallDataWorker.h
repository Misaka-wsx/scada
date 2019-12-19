#ifndef            TAS_APP_DSCADA_FAULT_LOCATION_CALL_DATA_SERVER
#define            TAS_APP_DSCADA_FAULT_LOCATION_CALL_DATA_SERVER

#include        "SysUtils.h"
#include        "SysTcpChannel.h"
#include        "AppDScadaMsgQueue.h"
#include        "AppDScadaComData.h"
#include        "SysDBSrvModelClient.h"

typedef std::map<SysString, SysString> DEV_ACCESS_MAP; //索引为设备编号、保存设备召测接口IP信息

typedef std::map<SysString, SysTcpChannel*> FES_LINK_MAP; //索引为前置WebService召测接口， 保存TCP链路结构

class  AppDScadaFLCallDataWorker : public SysThread
{
public:

    AppDScadaFLCallDataWorker()
    {

    }

    ~AppDScadaFLCallDataWorker()
    {
        m_devAccessMap.clear();

        if (m_fesLinkMap.size())
        {
            FES_LINK_MAP::iterator iter = m_fesLinkMap.begin();
            while(iter != m_fesLinkMap.end())
            {
                if (iter->second)
                {
                    iter->second->closeChannel();
                    delete iter->second;
                    iter->second = NULL;
                }

                m_fesLinkMap.erase(iter++);
            }
        }
    }

    SYS_HRESULT init();

    SYS_HRESULT run();

private:

    SysChar getChkSum(SysChar* buf, SysUInt size);

    SysBool checkMagic(SysChar* ptr);

    SYS_HRESULT connectSrv(SysTcpChannel* linkChannel, SysString& ipInfo);

    SYS_HRESULT loadDevAccessMap();

    SYS_HRESULT addNewConnection(SysString& ipInfo, SysTcpChannel* &channel);

    SYS_HRESULT shakeHand(SysTcpChannel* linkChannel);

    SYS_HRESULT sendCmd(webServiceReq* reqPtr);

    SYS_HRESULT transData(SysTcpChannel* linkChannel, webServiceReq* reqPtr);

    SysDBSrvModelClient m_modelDB;

    DEV_ACCESS_MAP m_devAccessMap;

    FES_LINK_MAP m_fesLinkMap;

    SYS_CLASS_LOGGER;
};

typedef SysSingleton<AppDScadaFLCallDataWorker> AppDScadaFLCallDataWorkerMgr;
#endif