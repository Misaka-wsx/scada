#ifndef		APP_DSCADA_HIS_DATA_PERSISTENCE_SERVER_H
#define	    APP_DSCADA_HIS_DATA_PERSISTENCE_SERVER_H
#include	"SysUtils.h"
#include    "SysFileOp.h"
#include    "SysRtdbmsMetaData.h"
/************************************************************************/
/*  Persist SOE and Signal data in his db or disk
/************************************************************************/
class SysDBSrvModelClient;
class AppDataPersisitPkg;
class AppDScadaSignal;
class SysRtdbmsTableOp;

class AppDScadaDataToDBWorker
{
public:

    AppDScadaDataToDBWorker() :m_modelDbClient(NULL),
                               m_isEnableSkyKeeper(false)
	{

	}
	~AppDScadaDataToDBWorker()
	{
		if (m_modelDbClient)
		{
			delete m_modelDbClient;
		}
	}

	SYS_HRESULT init();

	SYS_HRESULT start();

	SYS_HRESULT run();

	SYS_HRESULT loadConfig();

    SYS_HRESULT persisDataPkgToDB(AppDataPersisitPkg* pkgPtr);

	SYS_HRESULT retryOperation(AppDataPersisitPkg* pkgPtr);

    SYS_HRESULT persisSqlIntoDisk(SysString & sql);
	

    static SYS_HRESULT parseSoeSignal2Sql(AppDataPersisitPkg* soePtr);

    static SYS_HRESULT parseDoeSignal2Sql(AppDataPersisitPkg* soePtr, AppDScadaSignal* signalPtr);

    static SYS_HRESULT parseSampleData2Sql(AppDataPersisitPkg* pkgPtr);

    static SYS_HRESULT parseModelData2Sql(AppDataPersisitPkg* pkgPtr);

    static SYS_HRESULT parseMsgData2Sql(AppDataPersisitPkg* pkgPtr);

    static SYS_HRESULT parsesg2Sql(AppDataPersisitPkg* pkgPtr);

    static SYS_HRESULT parseShortMsgFaultData2Sql(AppDScadaSignal* faultPtr, AppDataPersisitPkg* pkgPtr, std::vector<SysString> & sqlVec, std::vector<SysString> & phoneNumber, std::vector<SysString> & warningType, std::vector<SysString> & userName);

    static SYS_HRESULT putFaultDatabase2Sql(AppDScadaSignal* faultPtr, AppDataPersisitPkg* pkgPtr, std::vector<SysString> & sqlVec);

    static SYS_HRESULT doePutMontior(AppDScadaSignal* faultPtr, AppDataPersisitPkg* pkgPtr);

private:

	SysDBSrvModelClient *		m_modelDbClient;

	SysFileOperator             m_fileOp;

	static log4cxx::LoggerPtr   m_sqlLogger;

	SysBool						m_isEnableSkyKeeper;

	SysString					m_SkyKeeperWorkingDir;

	SysString					m_SkyKeeperDestDir;
};

typedef			SysSingleton<AppDScadaDataToDBWorker>				AppDScadaDataToDBWorkerMgr;
#endif