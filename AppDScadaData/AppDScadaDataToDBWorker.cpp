#include	"AppDScadaDataToDBWorker.h"
#include    "SysDBSrvHisDataClient.h"
#include	"SysDBSrvModelClient.h"
#include	"AppDScadaMsgQueue.h"
#include    "AppDScadaError.h"
#include    "AppDScadaUtils.h"
#include    "SysXmlConfig.h"
#include	"SysRtdbmsMgr.h"
#include    "SysRtdbmsTableOperator.h"

log4cxx::LoggerPtr AppDScadaDataToDBWorker::m_sqlLogger = log4cxx::Logger::getLogger("AppDScadaDataToDBWorker");

static void thread_start(void * arg)
{
	AppDScadaDataToDBWorker* pthis = (AppDScadaDataToDBWorker*)arg;
	if (pthis != NULL)
	{
		pthis->run();
	}
}


SYS_HRESULT AppDScadaDataToDBWorker::init()
{
	SYS_HRESULT hr = SYS_OK;
	m_modelDbClient = new SysDBSrvModelClient;
	if (NULL == m_modelDbClient)
	{
		return BAD_ALLOC_MEM;
	}

	hr = m_modelDbClient->refreshMaster();
	if (SYS_OK != hr)
	{
		SYS_LOG_MESSAGE(m_sqlLogger, ll_error, "Connect to DB Master Error.");
	}
	loadConfig();
	return hr;
}

SYS_HRESULT AppDScadaDataToDBWorker::loadConfig()
{
	SYS_HRESULT hr = SYS_OK;
	SysString configFilePath = TAS_BACKEND_CONFIG;
	hr = SYS_CONFIG_MGR::getInstance()->openConfigFile(configFilePath);
	if ( SYS_OK != hr)
	{
		return hr;
	}
	XML_NODE_PATH nodePath;
	nodePath.push_back(SysString("AppSkyKeeperDataTransOutter"));
	SYS_CONFIG_MGR::getInstance()->locatToNode(nodePath);

	SysString workingDir;
	SYS_CONFIG_MGR::getInstance()->getNodeValue(SysString("workingDir"),workingDir);

	if (workingDir.length() == 0)
	{
		workingDir = "..\\data\\rtdata\\";
		SYS_LOG_MESSAGE(m_sqlLogger, ll_waring, "获取工作目录失败，使用默认工作路径:" << workingDir.c_str());

	}
	hr = SysOSUtils::CreateLocalDir(workingDir.c_str());
	if (SYS_OK != hr)
	{
		SYS_LOG_MESSAGE(m_sqlLogger, ll_waring, "创建工作目录失败 hr:" << hr << " 目录：" << workingDir.c_str());
		return hr;
	}
	m_SkyKeeperWorkingDir  = workingDir;
	SysString destDir;
	SYS_CONFIG_MGR::getInstance()->getNodeValue(SysString("DestDir"),destDir);
	if (destDir.length() == 0)
	{
		return SYS_FAIL;
	}
	ACE_DIR* destDirPtr = ACE_OS::opendir(destDir.c_str());  
	if (NULL == destDirPtr)
	{
		SYS_LOG_MESSAGE(m_sqlLogger, ll_error, "移动目录无效，请配置DestDir为隔离装置目录");
		return hr;
	}
	m_SkyKeeperDestDir = destDir;
	m_isEnableSkyKeeper = true;
	m_fileOp.setDir(m_SkyKeeperWorkingDir.c_str());
	return hr;
}

SYS_HRESULT AppDScadaDataToDBWorker::start()
{
	SYS_HRESULT hr = SYS_OK;
	if ( 0 < ACE_Thread_Manager::instance()->spawn((ACE_THR_FUNC)thread_start, this))
	{
		hr = SYS_OK;
	}
	else
	{
		hr = TAS_APP_DSCADA_START_PERSIST_SERVER_FAILED;
	}

	return hr;
}

SYS_HRESULT AppDScadaDataToDBWorker::run()
{
	SYS_HRESULT hr = SYS_OK;
	while(1)
	{
		AppDataPersisitPkg * persistPkg = NULL;
		AppTriggerSampleQueue::getInstance()->getQ(persistPkg);
		if (NULL == persistPkg)
		{
			SYS_LOG_MESSAGE(m_sqlLogger, ll_error, "Get NULL data from AppTriggerSampleQueue");
			ACE_OS::sleep(1);
			continue;
		}
        hr = persisDataPkgToDB(persistPkg);

		// 支持隔离功能；
		if ((m_isEnableSkyKeeper) && ((persistPkg->pkgType) & (AppDataPersisitPkg::TAS_SUPPORT_SkyKeeper)))
		{
            SysString strSql;
			persisSqlIntoDisk(strSql);
            persistPkg->sqlVec.push_back(strSql);
		}

		if(persistPkg)
		{
			delete persistPkg;
			persistPkg = NULL;
		}
	}

	return hr;
}

SYS_HRESULT AppDScadaDataToDBWorker::parseSoeSignal2Sql( AppDataPersisitPkg* soePtr)
{
    SYS_HRESULT hr = SYS_OK;
    if (soePtr)
    {
        SysChar buffer[1024] = {0};
        SysString soeTabName = "FACT_MSG_SOE";
        SysString soeDateTime = SysOSUtils::transEpochToTime(soePtr->m_timeStamp);
        sprintf(buffer,"insert into %s (SOE_DATE, UPDATE_DATE,DEVICE_CODE,LINE_ID,LINE_NAME,SOE_TYPE,SOE_RANGE,POLE,TIMESTAMP) values ( \
                       to_date('%s','yyyy-mm-dd hh24:mi:ss'),to_date('%s','yyyy-mm-dd hh24:mi:ss'),'%s','%s','%s',%d,'%s','%s','%lld')",
                       soeTabName.c_str(),
                       soeDateTime.c_str(),
                       soeDateTime.c_str(),
                       soePtr->devCode.c_str(),
                       soePtr->lineID.c_str(),
                       soePtr->lineName.c_str(),
                       TAS_104_SOE,
                       soePtr->desc.c_str(),
                       soePtr->installPole.c_str(),
                       soePtr->m_timeStamp);
       soePtr->sqlVec.push_back(buffer);
       SYS_LOG_MESSAGE(m_sqlLogger, ll_debug , "soe sql:" << buffer);
    }
    return hr;
}

SYS_HRESULT AppDScadaDataToDBWorker::parseDoeSignal2Sql( AppDataPersisitPkg* soePtr, AppDScadaSignal* signalPtr)
{
    SYS_HRESULT hr = SYS_OK;
    if (soePtr && signalPtr)
    {
        AppDimDevInfo devInfo;
        SysChar buffer[1024] = {0};
        SysString soeTabName = "FACT_MSG_SOE";

        switch(signalPtr->m_faultType)
        {
        case TAS_SHORT_WARNING_1:
        case TAS_SHORT_WARNING_2:
        case TAS_SHORT_WARNING_3:
        case TAS_DROP_FUSE_OFF:
        case TAS_DROP_FUSE_SC_OFF:
            {
                SysChar buff[64];
                if (soePtr->desc.length() < 2)
                {
                    signalPtr->accValueToString(soePtr->desc);
                }
                break;
            }
        case TAS_GROUNDING_ALARM:
        case TAS_GROUNDING_ALARM_PDI:
        case TAS_GROUNDING_ALARM_SFS:
            {
                if (signalPtr->m_value.size() != 0)
                {
                    if (soePtr->desc.length() < 2)
                    {
                        signalPtr->accValueToString(soePtr->desc);
                    }
                }
                break;
            }
        case TAS_RECLOSING_FAILED:
        case TAS_RECLOSING_SUCCESS:
            {
                signalPtr->accValueToString(soePtr->desc);
                break;
            }
        default : 
            {
                break;
            }
        }
        SysString strTimeStamp = SysOSUtils::transEpochToTime(soePtr->m_timeStamp);
        sprintf(buffer,"insert into %s (SOE_DATE, UPDATE_DATE,DEVICE_CODE,LINE_ID,LINE_NAME,SOE_TYPE,SOE_RANGE,POLE,TIMESTAMP) values ( \
                       to_date('%s','yyyy-mm-dd hh24:mi:ss'),to_date('%s','yyyy-mm-dd hh24:mi:ss'),'%s','%s','%s',%d,'%s','%s','%lld')",
                       soeTabName.c_str(),
                       strTimeStamp.c_str(),
                       strTimeStamp.c_str(),
                       soePtr->devCode.c_str(),
                       soePtr->lineID.c_str(),
                       soePtr->lineName.c_str(),
                       signalPtr->m_faultType,
                       soePtr->desc.c_str(),
                       soePtr->installPole.c_str(),
                       soePtr->m_timeStamp);
        soePtr->sqlVec.push_back(buffer);
        SYS_LOG_MESSAGE(m_sqlLogger, ll_debug , "soe sql:" << buffer);
    }
    return hr;
}

SYS_HRESULT AppDScadaDataToDBWorker::parseSampleData2Sql(AppDataPersisitPkg* pkgPtr)
{
	SYS_HRESULT hr = SYS_OK;
    SYS_FUNC_DEF( AppDScadaDataToDBWorker::parseSampleData2Sql);
    SYS_ASSERT_RETURN(NULL != pkgPtr, m_sqlLogger, SYS_FAIL);
	SysUInt pkgSize = pkgPtr->sampleDataVec.size();
	SysString insertSql = "";
    SysString updateSql = "";
    AppDevRelatedTableName tableName;
    hr = AppDScadaUtils::getTableNameByDevType(pkgPtr->devType,&tableName);
	SysBool flags = false;//判断是否为经纬度信息标志
	SysBool flags2 = false;//是否更新设备经纬度标志，若为0则不更新
	SYS_LOG_MESSAGE(m_sqlLogger, ll_debug, "Get sample data pakage to insert into his and rt DB. Size is : " << pkgSize);
	insertSql = "insert into " + tableName.m_strHisTable;
    updateSql = "update " + tableName.m_strRtTable + " set UPDATE_DATE = ";
	SysString updateDeviceSql = "update dim_mod_device set ";
	SysString fieldsName = "(DEVICE_CODE,UPDATE_DATE,DEVICE_TYPE,TIMESTAMP,";
    SysULong now = SysOSUtils::getTimeNowMsec();
    SysChar timeBuf[64] = {0};
	sprintf(timeBuf, "to_date('%s','yyyy-mm-dd hh24:mi:ss')", SysOSUtils::transEpochToTime(pkgPtr->m_timeStamp).c_str());
	SysString values = "( ";
	values = values + "'" + pkgPtr->devCode + "'," + timeBuf + ",'" + pkgPtr->devType + "',"; 
    updateSql = updateSql + timeBuf + ",";
    memset(timeBuf, 0, sizeof(timeBuf));
    sprintf(timeBuf, "%lld", now);
    values = values + timeBuf + ",";
    updateSql = updateSql + " DEVICE_TYPE = '" + pkgPtr->devType + "', TIMESTAMP = " + timeBuf + ",";

	for (int i = 0; i < pkgSize; i++)
	{
		SysString insertFileName;
        SysString updateFileInfo = pkgPtr->sampleDataVec[i].m_fieldName + " = ";
		if(pkgPtr->sampleDataVec[i].m_fieldName=="LATITUDE_GPS")
		{
			flags = true;
		}
		SysChar insertValueBuf[128];
        SysChar updateValueBuf[128];
		if ( (pkgSize - 1) == i )
		{
            insertFileName = pkgPtr->sampleDataVec[i].m_fieldName + ")";
			switch(pkgPtr->sampleDataVec[i].m_value.m_dataType)
			{
			case SYS_RTDB_DOUBLE:
                {
                    SysFloat fvalue = pkgPtr->sampleDataVec[i].m_value.m_dataValue.valueDouble;
                    sprintf(insertValueBuf, "%f)", fvalue);
                    sprintf(updateValueBuf, "%f ", fvalue);
                    break;
                }
			case SYS_RTDB_FLOAT:
				{
					SysFloat fvalue = pkgPtr->sampleDataVec[i].m_value.m_dataValue.valueFloat;
					sprintf(insertValueBuf, "%f)", fvalue);
                    sprintf(updateValueBuf, "%f ", fvalue);
					break;
				}
            case SYS_RTDB_INT:
            case SYS_RTDB_UINT:
                {
                    SysInt ivalue = pkgPtr->sampleDataVec[i].m_value.m_dataValue.valueInt;
                    sprintf(insertValueBuf, "%d)", ivalue);
                    sprintf(updateValueBuf, "%d ", ivalue);
                    break;
                }
			case SYS_RTDB_CHAR:
			case SYS_RTDB_UCHAR:
				{
					SysUChar ucvalue = pkgPtr->sampleDataVec[i].m_value.m_dataValue.valueUChar;
					if (pkgPtr->sampleDataVec[i].m_fieldName == "SWITCH_STATUS")
					{
						//开关状态解析
						switch (ucvalue)
						{
						case 0:
							strncpy_s(insertValueBuf, "'000')", 8);
							strncpy_s(updateValueBuf, "'000' ", 8);
							break;
						case 1:
							strncpy_s(insertValueBuf, "'100')", 8);
							strncpy_s(updateValueBuf, "'100' ", 8);
							break;
						case 2:
							strncpy_s(insertValueBuf, "'010')", 8);
							strncpy_s(updateValueBuf, "'010' ", 8);
							break;
						case 3:
							strncpy_s(insertValueBuf, "'110')", 8);
							strncpy_s(updateValueBuf, "'110' ", 8);
							break;
						case 4:
							strncpy_s(insertValueBuf, "'001')", 8);
							strncpy_s(updateValueBuf, "'001' ", 8);
							break;
						case 5:
							strncpy_s(insertValueBuf, "'101')", 8);
							strncpy_s(updateValueBuf, "'101' ", 8);
							break;
						case 6:
							strncpy_s(insertValueBuf, "'011')", 8);
							strncpy_s(updateValueBuf, "'011' ", 8);
							break;
						case 7:
							strncpy_s(insertValueBuf, "'111')", 8);
							strncpy_s(updateValueBuf, "'111' ", 8);
							break;
						default:
							strncpy_s(insertValueBuf, "'1')", 8);
							strncpy_s(updateValueBuf, "'1' ", 8);
							break;
						}
					}
					else
					{
						sprintf(insertValueBuf, "%d)", (SysInt)(ucvalue));
						sprintf(updateValueBuf, "%d ", (SysInt)(ucvalue));
					}
					break;
				}
			default:
				{
					sprintf(insertValueBuf, "%d)", 0);
                    sprintf(updateValueBuf, "%d ", 0);
					break;
				}
			}
            updateFileInfo += updateValueBuf;
			if (flags)
			{
				if (updateValueBuf[0] == '0' || updateValueBuf == nullptr)
				{
					flags2 = true;
				}
			}
		}
		else
		{
            insertFileName = pkgPtr->sampleDataVec[i].m_fieldName + ",";
			switch(pkgPtr->sampleDataVec[i].m_value.m_dataType)
			{
			case SYS_RTDB_DOUBLE:
                {
                    SysFloat fvalue = pkgPtr->sampleDataVec[i].m_value.m_dataValue.valueDouble;
                    sprintf(insertValueBuf, "%f,", fvalue);
                    sprintf(updateValueBuf, "%f,", fvalue);
                    break;
                }
			case SYS_RTDB_FLOAT:
				{
					SysFloat fvalue = pkgPtr->sampleDataVec[i].m_value.m_dataValue.valueFloat;
					sprintf(insertValueBuf, "%f,", fvalue);
                    sprintf(updateValueBuf, "%f,", fvalue);
					break;
				}
            case SYS_RTDB_INT:
            case SYS_RTDB_UINT:
                {
                    SysFloat ivalue = pkgPtr->sampleDataVec[i].m_value.m_dataValue.valueInt;
                    sprintf(insertValueBuf, "%d,", ivalue);
                    sprintf(updateValueBuf, "%d,", ivalue);
                    break;
                }
			case SYS_RTDB_CHAR:
			case SYS_RTDB_UCHAR:
				{
					SysUChar ucvalue = pkgPtr->sampleDataVec[i].m_value.m_dataValue.valueUChar;
					sprintf(insertValueBuf, "%d,", (SysInt)(ucvalue));
                    sprintf(updateValueBuf, "%d,", (SysInt)(ucvalue));
					break;
				}
			default:
				{
					sprintf(insertValueBuf, "%d,", 0);
                    sprintf(updateValueBuf, "%d,", 0);
					break;
				}
			}
            updateFileInfo += updateValueBuf;
			if (flags)
			{
				if (updateValueBuf == 0 || updateValueBuf == nullptr)
				{
					flags2 = true;
				}
			}
		}
		fieldsName += insertFileName;
		values += insertValueBuf;
        updateSql += updateFileInfo;
		updateDeviceSql += updateFileInfo;
	}
	insertSql = insertSql + fieldsName + " values " + values;
    updateSql = updateSql + "where DEVICE_CODE = '" + pkgPtr->devCode  + "'";
	updateDeviceSql=updateDeviceSql+ "where DEVICE_CODE = '" + pkgPtr->devCode + "'";
    pkgPtr->sqlVec.push_back(insertSql);
    pkgPtr->sqlVec.push_back(updateSql);
	//若为经纬度信息且经纬度信息不为0
	if (flags && !flags2)
	{
		pkgPtr->sqlVec.push_back(updateDeviceSql);
		SYS_LOG_MESSAGE(m_sqlLogger, ll_debug, "update SQL is : " << updateDeviceSql);
	}
	SYS_LOG_MESSAGE(m_sqlLogger, ll_debug, "insert SQL is : " << insertSql );
    SYS_LOG_MESSAGE(m_sqlLogger, ll_debug, "update SQL is : " << updateSql );

	return hr;
}

SYS_HRESULT AppDScadaDataToDBWorker::parseModelData2Sql( AppDataPersisitPkg* pkgPtr )
{
    SYS_HRESULT hr = SYS_OK;
    SYS_FUNC_DEF( AppDScadaDataToDBWorker::parseModelData2Sql);
    SYS_ASSERT_RETURN(NULL != pkgPtr, m_sqlLogger, SYS_FAIL);

    SysUInt pkgSize = pkgPtr->persistDataVec.size();
    SysString updateSql = "";
    AppDevRelatedTableName tableName;
    hr = AppDScadaUtils::getTableNameByDevType(pkgPtr->devType,&tableName);

    SYS_LOG_MESSAGE(m_sqlLogger, ll_debug, "Get model data pakage to insert into his and rt DB. Size is : " << pkgSize);
    SysULong now = SysOSUtils::getTimeNowMsec();
    SysChar timeBuf[64] = {0};
    sprintf(timeBuf, "to_date('%s','yyyy-mm-dd hh24:mi:ss')", SysOSUtils::transEpochToTime(pkgPtr->m_timeStamp).c_str());
    updateSql = "update " + tableName.m_strModelTable + " set DONE_TIME = " + timeBuf + ", ";

    for (int i = 0; i < pkgSize; i++)
    {
        SysString updateFileInfo = pkgPtr->persistDataVec[i].m_fieldName + " = ";
        SysChar updateValueBuf[128];
        if ( (pkgSize - 1) == i )
        {
            switch(pkgPtr->persistDataVec[i].m_value.m_dataType)
            {
            case SYS_RTDB_DOUBLE:
            case SYS_RTDB_FLOAT:
                {
                    SysFloat fvalue = pkgPtr->persistDataVec[i].m_value.m_dataValue.valueFloat;
                    sprintf(updateValueBuf, "%f ", fvalue);
                    break;
                }
            case SYS_RTDB_CHAR:
            case SYS_RTDB_UCHAR:
                {
                    SysUChar ucvalue = pkgPtr->persistDataVec[i].m_value.m_dataValue.valueChar;
                    sprintf(updateValueBuf, "'%s' ", ucvalue);
                    break;
                }
            case SYS_RTDB_INT:
            case SYS_RTDB_UINT:
                {
                    SysInt ivalue = pkgPtr->persistDataVec[i].m_value.m_dataValue.valueInt;
                    sprintf(updateValueBuf, "%d ", ivalue);
                    break;
                }
            case SYS_RTDB_DATATIME:
                {
                    SysULong ulvalue = pkgPtr->persistDataVec[i].m_value.m_dataValue.valueULong;
                    sprintf(updateValueBuf, "to_date('%s','yyyy-mm-dd hh24:mi:ss')", SysOSUtils::transEpochToTime(ulvalue).c_str());
                    break;
                }
            default:
                {
                    sprintf(updateValueBuf, "%d ", 0);
                    break;
                }
            }
            updateFileInfo += updateValueBuf;
        }
        else
        {
            switch(pkgPtr->persistDataVec[i].m_value.m_dataType)
            {
            case SYS_RTDB_DOUBLE:
            case SYS_RTDB_FLOAT:
                {
                    SysFloat fvalue = pkgPtr->persistDataVec[i].m_value.m_dataValue.valueFloat;
                    sprintf(updateValueBuf, "%f,", fvalue);
                    break;
                }
            case SYS_RTDB_CHAR:
            case SYS_RTDB_UCHAR:
                {
                    SysUChar ucvalue = pkgPtr->persistDataVec[i].m_value.m_dataValue.valueChar;
                    sprintf(updateValueBuf, "'%s',", ucvalue);
                    break;
                }
            case SYS_RTDB_INT:
            case SYS_RTDB_UINT:
                {
                    SysInt ivalue = pkgPtr->persistDataVec[i].m_value.m_dataValue.valueInt;
                    sprintf(updateValueBuf, "%d,", ivalue);
                    break;
                }
            case SYS_RTDB_DATATIME:
                {
                    SysULong ulvalue = pkgPtr->persistDataVec[i].m_value.m_dataValue.valueULong;
                    sprintf(updateValueBuf, "to_date('%s','yyyy-mm-dd hh24:mi:ss'),", SysOSUtils::transEpochToTime(ulvalue).c_str());
                    break;
                }
            default:
                {
                    sprintf(updateValueBuf, "%d,", 0);
                    break;
                }
            }
            updateFileInfo += updateValueBuf;
        }
        updateSql += updateFileInfo;
    }
    updateSql = updateSql + "where DEVICE_CODE = '" + pkgPtr->devCode  + "'";
    pkgPtr->sqlVec.push_back(updateSql);

    SYS_LOG_MESSAGE(m_sqlLogger, ll_debug, "update SQL is : " << updateSql );
    return hr;
}

SYS_HRESULT AppDScadaDataToDBWorker::persisDataPkgToDB( AppDataPersisitPkg* pkgPtr )
{
    SYS_HRESULT hr = SYS_OK;
    if ( NULL == pkgPtr)
    {
        return hr;
    }
    if (pkgPtr->persistDataVec.size() > 0)
    {
        hr = parseModelData2Sql(pkgPtr);
    }
    if (pkgPtr->sampleDataVec.size() > 0)
    {
        hr = parseSampleData2Sql(pkgPtr);
    }
    if ( m_modelDbClient && pkgPtr->sqlVec.size() > 0)
    {
        SysDBModelSrvRsp rsp;
        hr = m_modelDbClient->execSql(pkgPtr->sqlVec, rsp);
        if (SYS_OK != hr)
        {
            if (SysOSUtils::isDisconnect(hr))
            {
                hr = retryOperation(pkgPtr);
            }
            else
            {
                SYS_LOG_MESSAGE(m_sqlLogger, ll_error, "exec SQL error.hr:" << hr);
                //persisSqlIntoDisk(persistPkg->sqlStr);
            }
        }
        else if (rsp.m_resultsVec.size())
        {
            if (rsp.m_resultsVec[0].m_error.length())
            {
                SYS_LOG_MESSAGE(m_sqlLogger, ll_waring, "SQL failed. SQL: " << pkgPtr->sqlVec[0] << ". Failed reason: " << rsp.m_resultsVec[0].m_error);
                hr = SYS_FAIL;
            }
        }
    }
    return hr;
}

SYS_HRESULT AppDScadaDataToDBWorker::retryOperation(AppDataPersisitPkg* persistPkg)
{
	SYS_HRESULT hr = SYS_OK;

	if (m_modelDbClient && persistPkg->sqlVec.size() > 0)
	{
		for (int i = 0 ; i < 3; i++)
		{
			hr = m_modelDbClient->refreshMaster();
			if (SYS_OK == hr)
			{
				break;
			}
			else
			{
				SYS_LOG_MESSAGE(m_sqlLogger, ll_debug, "Reconnect to Service Maser :" << i );
			}
		}
		if (SYS_OK == hr)
		{
            SysDBModelSrvRsp rsp;
            hr = m_modelDbClient->execSql(persistPkg->sqlVec, rsp);
		    if (SYS_OK != hr)
			{
				SYS_LOG_MESSAGE(m_sqlLogger, ll_error, "Insert SQL error. Error : " << rsp.m_resultsVec[0].m_error.c_str());
				//persisSqlIntoDisk(persistPkg->sqlStr);
			}
		}
		else
		{
			SYS_LOG_MESSAGE(m_sqlLogger, ll_error, "Failed to retry operation. " );
		}
	}

	return hr;
}

SYS_HRESULT AppDScadaDataToDBWorker::persisSqlIntoDisk(SysString & sql)
{
    SYS_HRESULT hr = SYS_OK;
    if (!sql.empty())
    {
        SysString fileName;
        SysOSUtils::getTimeStampAaText(fileName);
        fileName = "RT_SQL_" + fileName;
        hr = m_fileOp.createFile(fileName);
        if (SYS_OK != hr)
        {
            SYS_LOG_MESSAGE(m_sqlLogger, ll_error, "生成RT_SQL文件失败：" << hr << ". " << fileName.c_str());
        }
        else
        {
            m_fileOp.writeData(sql.c_str(), sql.length());
        }
        m_fileOp.close();
        m_fileOp.moveFileToDes(fileName, m_SkyKeeperDestDir);
    }
    else
    {
        SYS_LOG_MESSAGE(m_sqlLogger, ll_error, "the sql string is empty!");
    }
    return hr;
}

SYS_HRESULT AppDScadaDataToDBWorker::parseMsgData2Sql( AppDataPersisitPkg* pkgPtr )
{
    SYS_HRESULT hr = SYS_OK;
    return hr;
}


SYS_HRESULT AppDScadaDataToDBWorker::parseShortMsgFaultData2Sql(AppDScadaSignal* faultPtr, AppDataPersisitPkg* pkgPtr, std::vector<SysString> & sqlVec, std::vector<SysString> & phoneNumber, std::vector<SysString> & warningType, std::vector<SysString> & userName)
{
    SYS_HRESULT hr = SYS_OK;
    SysString occurDevTime,occurTasTime;
    if (pkgPtr)
    {
        SysString faultName;
        SysString signalStr;
        SysString tabName = "FACT_FAULT_SHORTMSG";
        SysString tabName1 = "FACT_MSG_FAULT";
        SysChar faultid[20];
        SysChar tasTime[20];
        SysChar faulttype[20];
        SysChar Attribute[20];
		SysChar phase[20];
		sprintf(phase, "%d", faultPtr->m_phase);
        //sprintf(faultid,"%lld",faultPtr->m_occurDevTime);
		sprintf(faultid, "%lld", faultPtr->m_occurTasTime);
        sprintf(tasTime,"%lld",faultPtr->m_occurTasTime);
        sprintf(faulttype,"%d",faultPtr->m_faultType);
        sprintf(Attribute,"%lld",faultPtr->m_specialAttribute);
		occurDevTime = SysOSUtils::transEpochToTime(faultPtr->m_occurTasTime);
        //occurDevTime = SysOSUtils::transEpochToTime(faultPtr->m_occurDevTime);
        occurTasTime = SysOSUtils::transEpochToTime(faultPtr->m_occurTasTime);
        SysChar buffer[2048] = {0};
        //入故障库
		//sprintf(buffer, "insert into %s (FAULT_ID,FAULT_DATE, UPDATE_DATE, DEVICE_CODE,TRANS_ID,TRANS_NAME,FTYPE_ID,FTYPE_NAME,FAULT_RANGE, SFS_CURRENT, SOE_ARRAY,GPSINFO, FAULT_VALUE ,TIMESTAMP,LINE_ID,LINE_NAME) values (\
  //                      '%s',to_date('%s','yyyy-mm-dd hh24:mi:ss'),to_date('%s','yyyy-mm-dd hh24:mi:ss'),'%s','%s','%s','%s','%s','%s','%s','%s', '%s','%s',%lld,'%s','%s')",
		//	tabName1.c_str(),
		//	faultid,
		//	occurDevTime.c_str(),        //故障时间
		//	occurTasTime.c_str(),        //更新时间
		//	sqlVec[3].c_str(),              //code
		//	faultPtr->m_areaID.c_str(),         //台区id
		//	faultPtr->m_areaName.c_str(),       //台区名
		//	faulttype,                     //故障类型iD
		//	sqlVec[2].c_str(),             //故障类型
		//	sqlVec[0].c_str(),             //故障描述
		//	Attribute,                     //故障点描述
  //                      sqlVec[4].c_str(), 
		//	sqlVec[1].c_str(),             //地理位置
		//	sqlVec[5].c_str(),         //故障数据
		//	faultPtr->m_occurDevTime,       //时间戳
		//	faultPtr->m_lineID.c_str(),    //线路ID
  //                      faultPtr->m_lineName.c_str()  //线路
  //                      );
  //      SysString Str = buffer;
  //      if((faultPtr->m_faultType == 23)||(faultPtr->m_faultType == 24)||(faultPtr->m_faultType == 54)||(faultPtr->m_faultType == 55))
  //      {
		//	SYS_LOG_MESSAGE(m_sqlLogger, ll_waring, Str);
  //          pkgPtr->sqlVec.push_back(Str);
  //      }
        //入短信库
        for(int i = 0; i < phoneNumber.size() ;i++)
        {
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "insert into %s (FAULT_ID, FAULT_DATE, UPDATE_DATE,DEVICE_CODE,LINE_ID,LINE_NAME,FTYPE_ID,FTYPE_NAME, FAULT_RANGE, PHNUMBER,USER_REALNAME, DISPATCH_STATUS ,GPSINFO, SEND_OUT_DATA, SMSID_ARRAY, FAULT_LENGTH, WECHAT_STATUS, TRANS_ID, REC_IN_DATE,PHASE) values (\
                            '%s',to_date('%s','yyyy-mm-dd hh24:mi:ss'),to_date('%s','yyyy-mm-dd hh24:mi:ss'),'%s','%s','%s','%s','%s','%s','%s','%s', %d,' %s',to_date('%s','yyyy-mm-dd hh24:mi:ss'),'%s',%d, %d,'%s' ,'%s','%s')",/////,TRANS_ID,REC_IN_DATE  ,'%s','%s'
                            tabName.c_str(),
                            faultid,
							occurTasTime.c_str(),        //故障时间改为tas时间
                            //occurDevTime.c_str(),        //故障时间
                            occurTasTime.c_str(),        //更新时间
                            faultPtr->m_devCode.c_str(),   //装置编号
                            faultPtr->m_lineID.c_str(),    //线路ID
                            faultPtr->m_lineName.c_str(),  //线路
                            faulttype,                     //故障类型iD
                            sqlVec[2].c_str(),             //故障类型
                            sqlVec[0].c_str(),             //故障描述
                            phoneNumber[i].c_str(),         //电话号码
                            userName[i].c_str(),          //真实名称
                            11,
                            sqlVec[1].c_str(),             //地理位置
                            occurDevTime.c_str(),           //发送时间
                            warningType[i].c_str(),
                            sqlVec[0].size(),
                            11,
                            faultPtr->m_areaID.c_str(),         //台区id
                            tasTime,                             //入库时间戳
							phase								 //相别
                            );
            SysString Str = buffer;
			SYS_LOG_MESSAGE(m_sqlLogger, ll_waring, Str);
            pkgPtr->sqlVec.push_back(Str);
        }
		// occurDevTime = SysOSUtils::transEpochToTime(faultPtr->m_occurDevTime);
		occurDevTime = SysOSUtils::transEpochToTime(faultPtr->m_occurTasTime);
    }
    return hr;
}


SYS_HRESULT AppDScadaDataToDBWorker::putFaultDatabase2Sql(AppDScadaSignal* faultPtr, AppDataPersisitPkg* pkgPtr, std::vector<SysString> & sqlVec)
{
    SYS_HRESULT hr = SYS_OK;
    if (pkgPtr)
    {
        SysString faultName;
        SysString signalStr;
        SysString tabName1 = "FACT_MSG_FAULT";
        SysChar faultid[20];
        SysChar faulttype[20];
        SysChar Attribute[20];
        sprintf(faultid,"%lld",faultPtr->m_occurDevTime);
        sprintf(faulttype,"%d",faultPtr->m_faultType);
        sprintf(Attribute,"%lld",faultPtr->m_specialAttribute);
        SysString m_occurDevTime = SysOSUtils::transEpochToTime(faultPtr->m_occurTasTime);
        SysString m_occurTasTime = SysOSUtils::transEpochToTime(faultPtr->m_occurTasTime);
        SysChar cCurrenta[15];
        sprintf(cCurrenta,"%0.2f",faultPtr->m_value[0]);
        SysString sCurrenta = cCurrenta;
        SysString sfaultNum = sCurrenta + "A";
        SysChar buffer[1024] = {0};
        sprintf(buffer, "insert into %s (FAULT_DATE, UPDATE_DATE, DEVICE_CODE,LINE_ID,LINE_NAME,FTYPE_ID,FTYPE_NAME,FAULT_RANGE, SFS_CURRENT, SOE_ARRAY,GPSINFO, FAULT_VALUE ,TIMESTAMP) values (\
                        to_date('%s','yyyy-mm-dd hh24:mi:ss'),to_date('%s','yyyy-mm-dd hh24:mi:ss'),'%s','%s','%s','%s','%s','%s','%s','%s', '%s','%s',%lld)",
                        tabName1.c_str(),
                       // m_occurDevTime.c_str(),        //故障时间
						m_occurTasTime.c_str(),        //故障时间
                        m_occurTasTime.c_str(),        //更新时间
                        faultPtr->m_devCode.c_str(),   //装置编号
                        faultPtr->m_lineID.c_str(),    //线路ID
                        faultPtr->m_lineName.c_str(),  //线路
                        faulttype,                     //故障类型iD
                        sqlVec[2].c_str(),             //故障类型
                        sqlVec[0].c_str(),             //故障描述
                        Attribute,                     //故障点描述
                        faultid,             //SOE信号
                        sqlVec[1].c_str(),             //地理位置
                        sfaultNum.c_str(),         //故障数据
                        faultPtr->m_occurTasTime       //时间戳
                        );
        pkgPtr->sqlVec.push_back(buffer);
    }

    return hr;
};


SYS_HRESULT AppDScadaDataToDBWorker::doePutMontior(AppDScadaSignal* faultPtr, AppDataPersisitPkg* pkgPtr)
{
    SYS_HRESULT hr = SYS_OK;
    if (pkgPtr)
    {
        SysString tabName = "FACT_DEVICE_520_MONTIOR";
        SysString m_occurDevTime = SysOSUtils::transEpochToTime(faultPtr->m_occurDevTime);
        SysChar buffer[1024] = {0};
        sprintf(buffer, "UPDATE %s set IA = '%f' , IB = '%f' ,IC = '%f',UA = '%f', UB = '%f', UC = %f ,UPDATE_DATE = to_date('%s','yyyy-mm-dd hh24:mi:ss'),TIMESTAMP = %lld where DEVICE_CODE = '%s'",
                        tabName.c_str(),
                        faultPtr->m_value[1],
                        faultPtr->m_value[2],
                        faultPtr->m_value[3],
                        faultPtr->m_value[4],
                        faultPtr->m_value[5],
                        faultPtr->m_value[6],
                        m_occurDevTime.c_str(),
                        faultPtr->m_occurDevTime,
                        faultPtr->m_devCode.c_str()
                        );
        pkgPtr->sqlVec.push_back(buffer);
        pkgPtr->sqlVec.push_back(buffer);
    }

    return hr;
};
