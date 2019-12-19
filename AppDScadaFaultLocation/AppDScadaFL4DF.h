#ifndef             APP_DSCADA_DROP_FUSE_H
#define             APP_DSCADA_DROP_FUSE_H

#include            "SysUtils.h"
#include            "SysBaseTypes.h"
#include            "AppDScadaComData.h"
#include            "AppDScadaDataDef.h"
#include            "AppDScadaMsgQueue.h"
#include            "AppDScadaFLConfig.h"
#include            "SysDBSrvModelClient.h"
#include            "AppDScadaDataToDBWorker.h"

#define             DEF_MIN_FAULT_INTERVAL        60*60

#define             OVERLOAD_IS_ZERO(value)     (value < AppScadaFLConfigurationMgr::getInstance()->Df_OverloadAsZeroValue)

typedef enum
{
    FAULT_INFO_SECTION = 0,
    FAULT_INFO_GPS = 1,
    FAULT_INFO_NAME = 2,
    FAULT_INFO_SIGNAL = 3,
    FAULT_INFO_TYPE = 4,
    LONGITUDE_GPS = 5,
    LATITUDE_GPS= 6
}FAULT_INFO;

class AppDScadaFTreeNode;

class AppDScadaDropFuseFault
{
public:

    enum
    {
        DF_INIT = 0,

        DF_OFF = 1,

        DF_SC_OFF,

        DF_SC_ONLY,

        INVALID_STATUS = 0x111111
    };

    AppDScadaDropFuseFault():m_lastFaultStamp(0),m_status(DF_INIT), m_sig(NULL),m_counter(0)
    {

    }

    ~AppDScadaDropFuseFault()
    {
        if (m_sig)
        {
            delete m_sig;
            m_sig = NULL;
        }
    }

    SysULong   m_lastFaultStamp; // counter
    
    SysString  m_devCode;

    SysUInt    m_status;

    AppDScadaSignal* m_sig;

    SysUInt    m_counter;
};

class AppDScadaFl4DFWorker : public SysThread
{
public:

    AppDScadaFl4DFWorker():m_minFaultInterval(DEF_MIN_FAULT_INTERVAL)
    {

    }

    SYS_HRESULT init();

    SYS_HRESULT run();

    SysBool isDropFuse(AppDScadaSignal* sig);
	//������ϵ����ź�
    SYS_HRESULT processDropFuseOff(AppDScadaSignal* sig);
	//*@brief ��ȡ�豸״̬
	//* ״̬000000 0-�� 1-�� 
	//*������ �Ƿ���/�Ƿ��˳�����/�Ƿ�����/����/��ѹ/��λ �Ƿ��쳣
    SYS_HRESULT getDeviceStatus(SysString & deviceCode,SysString & statusValue);
	//��ȡ�豸������·״̬
	SYS_HRESULT getDeviceLineStatus(SysString & deviceCode, SysString & statusValue);
	//����δ���� ��·�ź�
    SYS_HRESULT processDropFuseSC(AppDScadaSignal* sig);
	//�����һ�ζ�·�ź�
    SYS_HRESULT processDropFuseSCFirst(AppDScadaSignal* sig);
	//ѭ����������������ݣ��ȴ��ٲ���
    SYS_HRESULT updateStatus();
	//�����ٲ����������
    SYS_HRESULT analysisCallData(AppDScadaDropFuseFault* faultPtr);
	//�������
    SYS_HRESULT dispatchSignalAsWarnMsg(AppDScadaSignal* sig);
	//ͬһ�豸��·�ź�������źźϲ�
    SysBool combineAndSplitSignal(AppDScadaDropFuseFault* preSig, AppDScadaSignal* curSig);
	//��ǰ�÷��͹����Ĺ�����Ϣ
    AppDScadaMsgQueue<AppDScadaSignal> m_queue;

private:
	//��������ź�
    SYS_HRESULT processDropFuseSignal(AppDScadaSignal* sig);
	//�����ٲ����ж���·�غ��Ƿ�仯
    SysBool overloadIsChanged(SysFloat ia, SysFloat ib, SysFloat ic);
	//��������
    SYS_HRESULT deliverCallData(SysString& devCode);
	//������������
    SYS_HRESULT getDropFuseDescription(SysUInt faultType, SysString& faultName ,SysString& faultID);
	//�����豸��γ����Ϣ
    SYS_HRESULT getDropFuseFaultInfo(AppDScadaSignal* sig, std::vector<SysString>& faultInfoVec);
	//���䱣��ʵʱ���� �������
    SYS_HRESULT getDropFuseRtData(AppDScadaDropFuseFault* faultPtr, SysDBModelSrvRsp& rsp);

	//��ȡ�豸�ϴι���ʱ���������������
	SYS_HRESULT getDropFuseLastFaultData(AppDScadaSignal* faultPtr, SysDBModelSrvRsp& rsp);
	
    ACE_Recursive_Thread_Mutex     m_guard; 
	//���ϵ�����ϱ�
    std::map<SysString, AppDScadaDropFuseFault*> m_DropFaultMap;
	//��·���ϣ�����δ���䣩���ϱ�
    std::map<SysString, AppDScadaDropFuseFault*> m_ScFaultMap;
	//�����������ʼ����һСʱ������û�б��ù���������Ҳ��֪�����Ǹ���ġ�
    SysULong  m_minFaultInterval;   // seconds;
	//��ɾ���ź�
    std::vector<SysString>          m_removeSigVec;

    SysDBSrvModelClient             m_modelDB;

    SysDBSrvModelClient *m_pModelDB;

    SYS_CLASS_LOGGER;
};

typedef SysSingleton<AppDScadaFl4DFWorker>  AppDScadaFl4DFWorkerMgr;

class AppDScadaDropFusePolicy
{

public:

    AppDScadaDropFusePolicy():m_minFaultInterval(DEF_MIN_FAULT_INTERVAL)
    {

    }

    void init();

public:

    SysUInt   m_minFaultInterval;

private:

};

typedef SysSingleton<AppDScadaDropFusePolicy> AppDScadaDropFusePolicyMgr;
#endif