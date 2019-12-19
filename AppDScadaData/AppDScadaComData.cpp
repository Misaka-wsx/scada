#include		"AppDScadaComData.h"

void AppDScadaSignal::signalToBuffer(SysChar* bufferPtr, SysUInt bufferSize)
{
	sprintf(bufferPtr, "%lld,%lld,%lld,%lld,%d,%d,%d,",
		m_signalID,m_faultID,m_occurDevTime,m_occurTasTime,m_phase,m_faultType,m_value.size());
	SysChar* tmpPtr = bufferPtr + strlen(bufferPtr);
	for(int i = 0; i < m_value.size(); i++)
	{
		sprintf(tmpPtr,"%f,", m_value[i]);
		tmpPtr = bufferPtr + strlen(bufferPtr);
	}
	sprintf(tmpPtr, "%d,%d,%d,%d,%d,%d,%s,%s", 
		m_status,m_QuickBreakLimit,m_OverFlowLimit,m_OverFlowDurance, m_transRatioFirst,m_transRatioSecond,m_devCode.c_str(),m_pole.c_str());
}

void AppDScadaSignal::bufferToSignal(SysChar* bufferPtr, SysUInt bufferSize, AppDScadaSignal & signalobj)
{

}

SysBool  AppDScadaSignal::isSCFault()
{
	if ((TAS_SHORT_WARNING_1 == m_faultType) ||
		(TAS_SHORT_WARNING_2 == m_faultType) ||
		(TAS_SHORT_WARNING_3 == m_faultType))
	{
		return true;
	}
	return false;
}

SysBool  AppDScadaSignal::isGroundingFault()
{
	if ((TAS_GROUNDING_ALARM == m_faultType) ||
		(TAS_GROUNDING_ALARM_PDI == m_faultType) ||
		(TAS_GROUNDING_ALARM_SFS == m_faultType) ||
		(TAS_TINY_GROUNDING_LOST_VOLTAGE == m_faultType))
	{
		return true;
	}
	return false;
}