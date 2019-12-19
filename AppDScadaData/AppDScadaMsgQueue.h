
#ifndef					TAS_APP_DSCADA_QUEUE_H
#define					TAS_APP_DSCADA_QUEUE_H

#include	"SysUtils.h"
#include	"ace/synch.h"
#include    "AppDScadaComData.h"
#include    "SysWebServiceDataAnalysis.h"

class AppDScadaMsg
{
public:
	AppDScadaMsg(): m_msgLen(0), m_ptr(NULL),m_type(0)
	{

	}
	~AppDScadaMsg()
	{
		if (m_ptr)
		{
			delete m_ptr;
		}
		m_msgLen = 0;
	}
	SysUInt m_msgLen;
	SysUInt	m_type;
	void *  m_ptr;
};


template <class T>
class AppDScadaMsgQueue
{

public:
	enum
	{
		BLOCK_MODE = 1,

		NONE_BLOCK_MODE
	};

	AppDScadaMsgQueue() :m_queueSize(0),m_cond(m_mutex),m_mode(BLOCK_MODE)
	{

	}
	~AppDScadaMsgQueue()
	{

	}


	SYS_HRESULT putQ(T* msg)
	{
		SYS_HRESULT hr = SYS_OK;
		ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_mutex);

		m_msgQueue.push_back(msg);
		m_queueSize++;
		m_cond.signal();
		return hr;
	}

	SYS_HRESULT getQ(T* & msg)
	{
		SYS_HRESULT hr = SYS_OK;
		msg = NULL;
		ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_mutex);

		if (BLOCK_MODE == m_mode)
		{
			while( m_msgQueue.empty())
			{
				m_cond.wait();
			}
			msg = m_msgQueue.front();
			m_msgQueue.pop_front();
			m_queueSize --;
		}
		else
		{
			if (m_msgQueue.empty())
			{
				return NULL;
			}
			else
			{
				msg = m_msgQueue.front();
				m_msgQueue.pop_front();
				m_queueSize --;
			}
		}

		return hr;
	}

	SYS_HRESULT getQSize()
	{
		ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_mutex);
		return m_queueSize;
	}

	void setMode(SysUInt mode)
	{
		m_mode = mode;
	}

private:

	std::list<T*> m_msgQueue;

	SysUInt	m_queueSize;

	SysUInt m_mode;

	ACE_Recursive_Thread_Mutex			 m_mutex;
	ACE_Condition<ACE_Recursive_Thread_Mutex>	 m_cond;
};

typedef		SysSingleton< AppDScadaMsgQueue<AppDScadaSignal> >    AppSignalQueue;
//typedef     SysSingleton< AppDScadaMsgQueue<AppDScadaSignal> >    AppWaringSignalQueue;
typedef		SysSingleton< AppDScadaMsgQueue<AppDataPersisitPkg> > AppTriggerSampleQueue;
typedef		SysSingleton< AppDScadaMsgQueue<DScadaCmdPkg> >  AppFLCallDataQueue;
typedef     SysSingleton< AppDScadaMsgQueue<webServiceReq> > AppFLWebCallDataQueue;
typedef     SysSingleton< AppDScadaMsgQueue<DScadaDataPkg> > AppDScadaPkgQueue;


#endif