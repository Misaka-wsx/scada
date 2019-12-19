#ifndef			TAS_APP_DSCADA_UTILS_H
#define			TAS_APP_DSCADA_UTILS_H

#include        "SysUtils.h"
#include        "SysACE.h"
#include        "AppDScadaMsgQueue.h"
#include        "SysDBSrvModelClient.h"

class AppDimDevInfo
{
public:
    SysString   m_strDevCode;
    SysString   m_strProID;
    SysString   m_strProName;
    SysString   m_strAorID;
    SysString   m_strAorName;
    SysString   m_strStationID;
    SysString   m_strStationName;
    SysString   m_strAreaID;
    SysString   m_strAreaName;
    SysString   m_strLineID;
    SysString   m_strLineName;
    SysString   m_strOperName;
    SysString   m_strDevType;
    SysString   m_strPole; //��װ��Ϣ
};

class AppDevProtectInfo
{
public:
    SysString   m_strDevCode;
    SysFloat    m_fISoftStrap;
    SysFloat    m_fIConstValue;
    SysFloat    m_fIDelay;
    SysFloat    m_fIISoftStrap;
    SysFloat    m_fIIConstValue;
    SysFloat    m_fIIDelay;
    SysFloat    m_fIIISoftStrap;
    SysFloat    m_fIIIConstValue;
    SysFloat    m_fIIIDelay;
    SysFloat    m_fZeroSoftStrap;
    SysFloat    m_fZeroConstValue;
    SysFloat    m_fZeroDelay;
};

class AppXHDevProtectInfo
{
public:
    SysString   m_strDevCode;
    SysInt      m_iQuickBreakProtectThreshold;
    SysInt      m_iQuickBreakTrip;
    SysInt      m_iQuickBreakDelay;

    SysInt      m_iOverCurrentProtectThreshold;
    SysInt      m_iOverCurrentTrip;
    SysInt      m_iOverCurrentDelay;

    SysInt      m_iZeroCurrentThreshold;
};

class AppDevRelatedTableName
{
public:
    AppDevRelatedTableName():m_strHisTable(""),
        m_strRtTable(""),
        m_strProtocolTable(""),
        m_strModelTable("")
    {
    }
    SysString m_strHisTable;
    SysString m_strRtTable;
    SysString m_strProtocolTable;
    SysString m_strModelTable;
};

class AppDScadaUtils
{
public:

    /**
        @name getDimDevInfoByDevCodeType
        @brief ��ȡ�豸��ͨ����Ϣ
        @param devCode �豸���
        @param AppDimDevInfo �豸��Ϣ����·����������λ�õȣ�
    */
    static SYS_HRESULT  getDimDevInfoByDevCodeType(SysString &devCode, AppDimDevInfo *pDimDevInfo);

    /**
        @name getDevProtectInfoByDevCodeType
        @brief ��ȡ104Э���豸�ı�����Ϣ
        @param devCode �豸���
        @param devType �豸����
        @param pDevProInfo ������Ϣ
    */
    static SYS_HRESULT  getDevProtectInfoByDevCodeType(SysString &devCode, SysString &devType, AppDevProtectInfo *pDevProInfo);

    /**
        @name getXHDevProtectInfoByDevCodeType
        @brief ��ȡ�˻�Э���豸�ı�����Ϣ
        @param devCode �豸���
        @param devType �豸����
        @param pXHDevProInfo ������Ϣ
    */
    static SYS_HRESULT  getXHDevProtectInfoByDevCodeType(SysString &devCode, SysString &devType, AppXHDevProtectInfo *pXHDevProInfo);

    static SYS_HRESULT  getFaultNameByType(SysUInt faultType, SysString& faultName, AppDScadaSignal* sig = NULL);

    /**
        @name getTableNameByDevType
        @brief �����豸���ͻ�ȡ�豸�ı���
        @param strDevType �豸����
        @param tablePtr ������Ϣ
    */
    static SYS_HRESULT  getTableNameByDevType(SysString &strDevType,AppDevRelatedTableName *tablePtr);

private:

    static SysDBSrvModelClient   s_dbClient;

    static  ACE_Recursive_Thread_Mutex   s_guard;

    static std::map<SysString,AppDimDevInfo> s_dimDevInfoMap;

    static std::map<SysString,AppDevProtectInfo> s_devProInfoMap;

    static std::map<SysString,AppXHDevProtectInfo> s_XHdevProInfoMap;

    SYS_CLASS_LOGGER;
};

#endif