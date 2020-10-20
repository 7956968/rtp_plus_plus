#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <cpputil/GenericParameters.h>
#include <cpputil/RunningAverageQueue.h>
//#include <rtp++/rto/RetransmissionInfo.h>
#include <rtp++/rto/MultipathRtoManagerInterface.h>
#include <rtp++/rto/BasicRtoEstimator.h>
#include <rtp++/rto/PredictorTypes.h>
#include <rtp++/rto/RtoManagerInterface.h>

namespace rtp_plus_plus
{
namespace rto
{ 

class MultipathRtoEstimator : public MultipathPacketLossDetectionBase
{
public:
  MultipathRtoEstimator(boost::asio::io_service& rIoService, const GenericParameters &applicationParameters);

protected:

  void doOnPacketArrival( const boost::posix_time::ptime& tArrival, const uint16_t uiSN, const uint16_t uiFlowId, const uint16_t uiFSSN );
  void doOnRtxRequested(const boost::posix_time::ptime& tRequested, const int uiSN, const int uiFlowId, const int uiFSSN)
  {
    if (uiFlowId != SN_NOT_SET && uiFSSN != SN_NOT_SET)
    {
      m_mEstimators[uiFlowId]->onRtxRequested(tRequested, uiFSSN);
    }
  }
  void doOnRtxPacketArrival(const boost::posix_time::ptime& tArrival,
                                  const int uiSN, const int uiFlowId, const int uiFSSN,
                                  bool bLate, bool bDuplicate)
  {
    if (uiFlowId != SN_NOT_SET && uiFSSN != SN_NOT_SET)
    {
      m_mEstimators[uiFlowId]->onRtxPacketArrival(tArrival, uiFSSN, bLate, bDuplicate);
    }
  }
  virtual void doStop();

private:
  void onRtpPacketAssumedLostInFlow(uint16_t uiFSSN, uint16_t uiFlowId);

private:
  /// asio service
  boost::asio::io_service& m_rIoService;
  const GenericParameters& m_applicationParameters;
  /// shutdown flag
  bool m_bShuttingdown;

  PredictorType m_eType;

  std::map<uint16_t, std::unique_ptr<PacketLossDetectionBase> > m_mEstimators;
};

} // rto
} // rtp_plus_plus
