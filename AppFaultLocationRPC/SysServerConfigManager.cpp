#include "SysServerConfigManager.h"

SYS_CLASS_LOGGER_DEF(SysServerConfigManager);

SysServerConfigManager::SysServerConfigManager(void)
{
    SYS_LOG_MESSAGE(m_logger, ll_debug,"this is the constructor of SysServerConfigManager!");
}

SysServerConfigManager::~SysServerConfigManager(void)
{
    SYS_LOG_MESSAGE(m_logger, ll_debug, "this is the destructor of SysServerConfigManager!");
}

SYS_HRESULT SysServerConfigManager::init(SysString& filePath)
{
    SYS_HRESULT hr = SYS_OK;
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, m_guard, SYS_FAIL);

    SYS_LOG_MESSAGE(m_logger, ll_debug, "��ʼ���������ļ�config.xml");
    hr = m_config.openConfigFile(filePath);
    if (SYS_OK != hr)
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "���ļ�ʧ��!");
        return SYS_FAIL;
    }

    hr = getServerAttributeInfo(SysString("SERVER_INFO"), SysString("SERVER_IP"), m_srvIP);
    if (SYS_OK != hr)
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "��ȡ�����������˿�IPʧ��!");
        return SYS_FAIL;
    }

    hr = getServerAttributeInfo(SysString("SERVER_INFO"), SysString("SERVER_PORT"), m_srvPort);
    if (SYS_OK != hr)
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "��ȡ�����������˿���Ϣʧ��!");
        return SYS_FAIL;
    }

    return hr;
}

SYS_HRESULT SysServerConfigManager::getServerAttributeInfo(SysString& attrType, SysString& attrName, SysString& attrValue)
{
    SYS_HRESULT hr = SYS_OK;
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, m_guard, SYS_FAIL);

    XML_NODE_PATH nodePath;
    nodePath.push_back("ACE_example1");
    nodePath.push_back(attrType);
    hr = m_config.locatToNode(nodePath);

    if (SYS_OK != hr)
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "�޷���λ��ָ���Ľڵ�!" << attrType << "!");
        return hr;
    }

    hr = m_config.getNodeValue(attrName, attrValue);
    SYS_LOG_MESSAGE(m_logger, ll_debug, attrName << ": " << attrValue);

    return hr;
}