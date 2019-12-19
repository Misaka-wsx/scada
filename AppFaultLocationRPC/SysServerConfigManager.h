#ifndef SYS_SERVER_CONFIG_MANAGER_H
#define SYS_SERVER_CONFIG_MANAGER_H
#include "SysUtils.h"
#include "SysXmlConfig.h"

#define CONFIG_LOCATION "..\\config\\config.xml"

class SysServerConfigManager
{
public:

    SysServerConfigManager(void);

    ~SysServerConfigManager(void);

    SYS_HRESULT init(SysString& filePath);

    SysString& getServerIP() {return m_srvIP;}

    SysString& getServerPort() {return m_srvPort;}

    SYS_HRESULT getServerAttributeInfo(SysString& attrType, SysString& attrName, SysString& attrValue);

private:

    SysString m_srvIP;

    SysString m_srvPort;

    ACE_Recursive_Thread_Mutex m_guard;

    SysXmlConfig m_config;

    SYS_CLASS_LOGGER;
};

typedef SysSingleton<SysServerConfigManager> SysSrvConfigMgr;
#endif 