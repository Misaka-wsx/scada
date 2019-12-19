#include        "AppDScadaCallDataWorker.h"
#include        "SysServiceMgr.h"
#include        "AppDScadaError.h"

SYS_CLASS_LOGGER_DEF(AppDScadaFLCallDataWorker);

SYS_HRESULT AppDScadaFLCallDataWorker::loadDevAccessMap()
{
    SYS_HRESULT hr = SYS_OK;

    if (SYS_OK != m_modelDB.refreshMaster())
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "AppDScadaFLCallDataWorker::loadDevAccessMap() 连接数据库失败!");
        return SYS_FAIL;
    }

    std::vector<SysString> sqlVec;
    SysString queryDevAccessInfo = "SELECT DEVICE_CODE, FES_IP FROM DIM_DEVICE";
    sqlVec.push_back(queryDevAccessInfo);
    SysDBModelSrvRsp rsp;
    if (SYS_OK != m_modelDB.exeQuery(sqlVec, rsp))
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "AppDScadaFLCallDataWorker::loadDevAccessMap() 加载设备信息失败!");
        return SYS_FAIL;
    }

    if (SYS_OK != rsp.m_resultsVec[0].m_hr || !rsp.m_resultsVec[0].m_recordNum)
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "AppDScadaFLCallDataWorker::loadDevAccessMap() 加载错误设备信息!");
        return SYS_FAIL;
    }

    for (SysInt i = 0; i < rsp.m_resultsVec[0].m_recordNum; i++)
    {
        SysString devCode, accessIP;
        devCode = rsp.m_resultsVec[0].m_recordVec[i].m_record[0].m_dataValue.valueCharPtr;
        accessIP = rsp.m_resultsVec[0].m_recordVec[i].m_record[1].m_dataValue.valueCharPtr;
        m_devAccessMap.insert(make_pair(devCode, accessIP));
    }

    return hr;
}

SYS_HRESULT AppDScadaFLCallDataWorker::init()
{
    SYS_HRESULT hr = SYS_OK;

    if (SYS_OK != loadDevAccessMap())
    {
        return SYS_FAIL;
    }

    return hr;
}

SYS_HRESULT AppDScadaFLCallDataWorker::connectSrv(SysTcpChannel* linkChannel, SysString& ipInfo)
{
    SYS_HRESULT hr = SYS_OK;
    ACE_INET_Addr addr = ACE_INET_Addr(ipInfo.c_str());
    return linkChannel->channelConnect(addr);
}

SYS_HRESULT AppDScadaFLCallDataWorker::addNewConnection(SysString& ipInfo, SysTcpChannel* &channel)
{
    SYS_HRESULT hr = SYS_OK;

    channel = new SysTcpChannel;
    if (NULL == channel)
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "AppDScadaFLCallDataWorker::addNewConnection() 分配内存失败!");
        return SYS_FAIL;
    }

    if (SYS_OK != connectSrv(channel, ipInfo))
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "AppDScadaFLCallDataWorker::sendCmd() 连接前置服务失败!");
        delete channel;
        channel = NULL;
        return SYS_FAIL;
    }

    if (SYS_OK != shakeHand(channel))
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "AppDScadaFLCallDataWorker::sendCmd() 连接前置服务失败!");
        delete channel;
        channel = NULL;
        return SYS_FAIL;
    }

    m_fesLinkMap.insert(make_pair(ipInfo, channel));

    return hr;
}

SysBool AppDScadaFLCallDataWorker::checkMagic(SysChar* ptr)
{
    if (('x' == ptr[0])&&('h' == ptr[1])&&('d' == ptr[2])&&('l' == ptr[3]))
    {
        return true;
    }
    return false;
}

SYS_HRESULT AppDScadaFLCallDataWorker::shakeHand(SysTcpChannel* linkChannel)
{
    SYS_HRESULT hr = SYS_OK;
    if (linkChannel)
    {
        SysInt ret, len, tryCount;
        SysChar magic[4] = {'x','h','d','l'};
        SysChar recvBuf[256];
        ACE_Time_Value recvTimeOut = ACE_Time_Value(5, 0);

        tryCount = 0;
        while(tryCount < 3)
        {
            ret = linkChannel->channelSend(magic, sizeof(magic));
            if (ret <= 0)
            {
                SYS_LOG_MESSAGE(m_logger, ll_error, "AppDScadaFLCallDataWorker::shakeHand() 发送握手报文失败!");
                hr = SYS_FAIL;
            }

            memset(recvBuf, 0, sizeof(recvBuf));
            ret = linkChannel->channelRecvN(recvBuf, 5, 0, &recvTimeOut);
            if (ret <= 0 || !checkMagic(recvBuf))
            {
                SYS_LOG_MESSAGE(m_logger, ll_error, "AppDScadaFLCallDataWorker::shakeHand() 接收前置校验报文失败!");
                hr = SYS_FAIL;
            }

            len = SysInt(recvBuf[4] & 0xff);
            memset(recvBuf, 0, sizeof(recvBuf));
            ret = linkChannel->channelRecvN(recvBuf, len, 0, &recvTimeOut);
            SysString message = recvBuf;
            if (ret <= 0 || message.find("reg fail!") != SysString::npos)
            {
                SYS_LOG_MESSAGE(m_logger, ll_error, "AppDScadaFLCallDataWorker::shakeHand() 接收前置回复报文失败!");
                hr = SYS_FAIL;
            }

            if (SYS_OK == hr)
            {
                SYS_LOG_MESSAGE(m_logger, ll_debug, "与前置服务:" << message << "建立连接!");
                return hr;
            }

            tryCount++;
        }
    }

    return hr;
}

SysChar AppDScadaFLCallDataWorker::getChkSum(SysChar* buf, SysUInt size)
{
    SysChar chkSum = 0;

    for (SysInt i = 0; i < size; i++)
    {
        chkSum += buf[i];
    }

    return (chkSum % 256);
}

SYS_HRESULT AppDScadaFLCallDataWorker::transData(SysTcpChannel* linkChannel, webServiceReq* reqPtr)
{
    SYS_HRESULT hr = SYS_OK;

    if (linkChannel)
    {
        SysString msgStr;
        reqPtr->SerializeToString(&msgStr);
        SysUShort msgLen = reqPtr->ByteSize();
        SysUShort totalLen = msgLen + 5; // 1 for 0x68, 2 for len, 1 for checksum, 1 for 0x16
        SysChar* buf = new SysChar[totalLen];
        buf[0] = 0x68;
        *(SysUShort*)(buf + 1) = msgLen;
        memcpy(buf + 3, msgStr.c_str(), msgLen);
        buf[3 + msgLen] = getChkSum(buf, 3+ msgLen);
        buf[4 + msgLen] = 0x16;

        SysInt ret = linkChannel->channelSend(buf, totalLen);
        delete buf;
        if (ret <= 0)
        {
            SYS_LOG_MESSAGE(m_logger, ll_error, "AppDScadaFLCallDataWorker::transData() 命令数据传输失败!");
            return SYS_FAIL;
        }
    }

    return hr;
}

SYS_HRESULT AppDScadaFLCallDataWorker::sendCmd(webServiceReq* reqPtr)
{
    SYS_HRESULT hr = SYS_OK;

    if (reqPtr)
    {
        SysString devCode = reqPtr->devcode();
        DEV_ACCESS_MAP::iterator devAccessIter = m_devAccessMap.find(devCode);
        if (devAccessIter != m_devAccessMap.end())
        {
            FES_LINK_MAP::iterator fesLinkIter = m_fesLinkMap.find(devAccessIter->second);
            SysTcpChannel* channel = NULL;
            if (fesLinkIter != m_fesLinkMap.end())
            {
                channel = fesLinkIter->second;
                if (!channel->getConnectionStatus())
                {
                    if (SYS_OK != shakeHand(channel))
                    {
                        SYS_LOG_MESSAGE(m_logger, ll_error, "AppDScadaFLCallDataWorker::sendCmd() 连接前置服务失败!");
                        return SYS_FAIL;
                    }
                }
            }
            else
            {
                if (SYS_OK != addNewConnection(devAccessIter->second, channel))
                {
                    SYS_LOG_MESSAGE(m_logger, ll_error, "AppDScadaFLCallDataWorker::sendCmd() 连接前置服务失败!");
                    return SYS_FAIL;
                }
            }

            if (SYS_OK != transData(channel, reqPtr))
            {
                SYS_LOG_MESSAGE(m_logger, ll_error, "AppDScadaFLCallDataWorker::sendCmd() 下发召测命令失败!");
                return SYS_FAIL;
            }
        }
        else
        {
            SYS_LOG_MESSAGE(m_logger, ll_error, "AppDScadaFLCallDataWorker::sendCmd() 无法找到设备对应的前置召测接口IP信息!");
            return SYS_FAIL;
        }
    }

    return hr;
}

SYS_HRESULT AppDScadaFLCallDataWorker::run()
{
    SYS_HRESULT hr = SYS_OK;
    ACE_Time_Value interval(5, 50 * 1000);

    ACE_Time_Value reConnectionInterval(3, 0);
    SysUInt counter = 1;

    while(true)
    {
        webServiceReq * callDataPtr = NULL;
        AppFLWebCallDataQueue::getInstance()->getQ(callDataPtr);
        if (NULL == callDataPtr)
        {
            SYS_LOG_MESSAGE(m_logger, ll_error, "Failed get output stream ptr. Get NULL data pointer.");
            ACE_OS::sleep(interval);
            continue;
        }

        hr = sendCmd(callDataPtr);
        if (SYS_OK != hr)
        {
            SYS_LOG_MESSAGE(m_logger, ll_error, "召测数据失败...");
        }

        delete callDataPtr;
        ACE_OS::sleep(reConnectionInterval);
    }

    return hr;
}
