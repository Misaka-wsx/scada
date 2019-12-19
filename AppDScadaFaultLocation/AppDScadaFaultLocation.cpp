// AppDScadaFaultLocation.cpp : Defines the entry point for the console application.
//  Including NotifyClient, working thread.

#include "SysServiceMonitorClient.h"
#include "AppDScadaFL4Tiny.h"
#include "AppDScadaFL4DF.h"
#include "AppDScadaFLConfig.h"
#include "AppRPC_FaultLocationSrv_Server.h"
#include "AppDScadaCallDataWorker.h"
#include "SysXmlConfig.h"
#include "SysUtils.h"

SysString LOCAL_SYS_PROCESS_NAME_T = "AppDScadaFaultLocation";
log4cxx::LoggerPtr g_Logger = NULL;

//SysFloat ftuZeroCurrentMinLimit = 0.5;

int main(int argc, char* argv[])
{
    SYS_HRESULT hr = SYS_OK;
    // init log4cxx
    SysString tasHomePtr = "..";
    SysString logPropFile = (SysString)tasHomePtr + SLASHCHAR + "config" + SLASHCHAR + "AppDScadaFaultLocation.properties";
    log4cxx::PropertyConfigurator::configure(logPropFile.c_str());
    g_Logger = log4cxx::Logger::getLogger(LOCAL_SYS_PROCESS_NAME_T);

    // init not pop up on windows;
    NOTPOPUP_ON_WINDOWS;

    // ACE init;
    ACE::init();

    // init monitor and monitor probe;
    SYS_MONITOR_PROCESS(LOCAL_SYS_PROCESS_NAME_T);
    SYS_MONITOR_PROBE_ACTIVE(1, SysMonitorProbe::PROBE_KILL, SysString("APP_FL_JOB_WORKER"), SysSMClientConfigMgr::getInstance()->m_maxLostWorkerInterval);
    
    AppScadaFLConfigurationMgr::getInstance()->initConfiguration();
    //  register this process to process monitor.

    AppSignalQueue::getInstance()->setMode(AppDScadaMsgQueue<AppDScadaSignal> ::NONE_BLOCK_MODE);

    AppDScadaDataToDBWorker * warnMsgDsPtr = new AppDScadaDataToDBWorker;
    hr = warnMsgDsPtr->init();
    if(SYS_OK == hr)
    {
        hr = warnMsgDsPtr->start();
    }
    else
    {
        SYS_LOG_MESSAGE(g_Logger, ll_waring, "Init AppDScadaFLCallDataWorker failed.");
        return SYS_FAIL;
    }
    // 启动召测命令下发工作线程；
    AppDScadaFLCallDataWorkerMgr::getInstance()->init();
    AppDScadaFLCallDataWorkerMgr::getInstance()->start();

    // 启动400V系统工作线程；
    AppDScadaTinyWorkerMgr::getInstance()->init();
    AppDScadaTinyWorkerMgr::getInstance()->start();

    // 启动跌落保险工作线程；
    AppDScadaFl4DFWorkerMgr::getInstance()->init();
    AppDScadaFl4DFWorkerMgr::getInstance()->start();

    SysString configFilePath = TAS_BACKEND_CONFIG;
    SysXmlConfig config;
    hr = config.openConfigFile(configFilePath);
    if ( SYS_OK != hr)
    {
        SYS_LOG_MESSAGE(g_Logger, ll_error, "Open Config file error. hr : " << hr);
        SERVER_RETURN;
    }
    XML_NODE_PATH nodePath;

    nodePath.push_back(SysString("AppDScadaFaultLocation"));
    nodePath.push_back(SysString("SERVER_INFO"));
    SysString Ip, port;
    hr = config.locatToNode(nodePath); 
    if ( SYS_OK != hr)
    {
        SYS_LOG_MESSAGE(g_Logger, ll_error, "Parse configuration file error when get AppDScadaWarnServer::SERVER_INFO");
        SERVER_RETURN;
    }

    nodePath.clear();
    Ip.clear();
    port.clear();
    SysRPCProxyAttr attr;
    config.getNodeValue(SysString("Listen_IP"), Ip);
    config.getNodeValue(SysString("Listen_Port"), port);
    AppRPC_FaultLocationSrv_Server faultLocationServer(SysString("AppRPC_FaultLocationSrv_Server"));
    ACE_INET_Addr faultLocationSrvAddr(port.c_str(), Ip.c_str());
    attr.m_addr = faultLocationSrvAddr;
    hr = faultLocationServer.init(attr);
    if (SYS_OK != hr)
    {
        SYS_LOG_MESSAGE(g_Logger, ll_error, "初始化RPC服务失败，端口" << port << "可能被占用. errno: " << hr);
        exit(0);
    }
    SYS_TRY
    {
        faultLocationServer.start();
    }
    SYS_CATCH
    {
        SYS_LOG_MESSAGE(g_Logger, ll_error, "Waring service service stop working. ");
    }

    return 0;
}

