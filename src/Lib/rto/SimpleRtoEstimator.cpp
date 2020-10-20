#include "CorePch.h"
#include <rtp++/rfc3550/Rfc3550.h>
#include <rtp++/rto/SimpleRtoEstimator.h>

#define COMPONENT_LOG_LEVEL 10

namespace rtp_plus_plus
{
namespace rto
{

SimplePacketLossDetection::SimplePacketLossDetection()
  :m_bShuttingdown(false),
    m_bFirst(true),
    m_uiPreviouslyReceivedMaxSN(0)
{

}

SimplePacketLossDetection::~SimplePacketLossDetection()
{

}

void SimplePacketLossDetection::reset()
{
  m_bShuttingdown = false;
  m_bFirst = true;
  m_uiPreviouslyReceivedMaxSN = 0;
}

void SimplePacketLossDetection::doOnPacketArrival(const boost::posix_time::ptime &tArrival, const uint32_t uiSN)
{
  VLOG(COMPONENT_LOG_LEVEL) << "SimplePacketLossDetection::doOnPacketArrival: " << uiSN;
  if (m_bShuttingdown) return;

  if (m_bFirst)
  {
    m_bFirst = false;
    m_uiPreviouslyReceivedMaxSN = uiSN;
  }
  else
  {
    if (uiSN == m_uiPreviouslyReceivedMaxSN + 1)
    {
      // normal operation
      m_uiPreviouslyReceivedMaxSN = uiSN;
    }
    else
    {
      uint32_t uiDelta = uiSN - m_uiPreviouslyReceivedMaxSN;
      VLOG(COMPONENT_LOG_LEVEL) << "SimplePacketLossDetection::doOnPacketArrival uiDelta: " << uiDelta;
      // all packets in between have been lost
      if (uiDelta < rfc3550::MAX_DROPOUT)
      {
        // double check boundary conditions
        for (int i = 1; i < uiSN - m_uiPreviouslyReceivedMaxSN; ++i)
        {
          VLOG(COMPONENT_LOG_LEVEL) << "SimplePacketLossDetection::doOnPacketArrival list: " << m_uiPreviouslyReceivedMaxSN + i;
          m_onLost(m_uiPreviouslyReceivedMaxSN + i);
          // update stats
          m_statisticsManager.assumePacketLost(tArrival, m_uiPreviouslyReceivedMaxSN + i);
        }
        m_uiPreviouslyReceivedMaxSN = uiSN;
      }
      else if (uiDelta <= rfc3550::RTP_SEQ_MOD - rfc3550::MAX_MISORDER)
      {
        reset();
      }
      else
      {
        /* duplicate or reordered packet */
        // if duplicate ignore
        // if previously estimated lost - false positive e.g. due to misordering
        if (m_statisticsManager.isFalsePositive(uiSN))
        {
          VLOG(2) << "Detected false positive: " << uiSN;
          if (m_onFalsePositive)
            if (m_onFalsePositive(uiSN))
            {
              VLOG(2) << "Cancelled NACK for: " << uiSN;
              m_statisticsManager.setRtxCancelled(uiSN);
            }
        }
      }
    }
  }
}

void SimplePacketLossDetection::doStop()
{
  m_bShuttingdown = true;
}

} // rto
} // rtp_plus_plus
