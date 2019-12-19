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
    SysString   m_strPole; //安装信息
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
        @brief 获取设备的通用信息
        @param devCode 设备编号
        @param AppDimDevInfo 设备信息（线路、责任区、位置等）
    */
    static SYS_HRESULT  getDimDevInfoByDevCodeType(SysString &devCode, AppDimDevInfo *pDimDevInfo);

    /**
        @name getDevProtectInfoByDevCodeType
        @brief 获取104协议设备的保护信息
        @param devCode 设备编号
        @param devType 设备类型
        @param pDevProInfo 保护信息
    */
    static SYS_HRESULT  getDevProtectInfoByDevCodeType(SysString &devCode, SysString &devType, AppDevProtectInfo *pDevProInfo);

    /**
        @name getXHDevProtectInfoByDevCodeType
        @brief 获取兴汇协议设备的保护信息
        @param devCode 设备编号
        @param devType 设备类型
        @param pXHDevProInfo 保护信息
    */
    static SYS_HRESULT  getXHDevProtectInfoByDevCodeType(SysString &devCode, SysString &devType, AppXHDevProtectInfo *pXHDevProInfo);

    static SYS_HRESULT  getFaultNameByType(SysUInt faultType, SysString& faultName, AppDScadaSignal* sig = NULL);

    /**
        @name getTableNameByDevType
        @brief 根据设备类型获取设备的表名
        @param strDevType 设备类型
        @param tablePtr 表名信息
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