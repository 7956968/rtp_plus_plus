#pragma once
#include <unordered_map>
#include <vector>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <rtp++/network/EndPoint.h>
#include <rtp++/network/NetworkPacket.h>

namespace rtp_plus_plus
{
namespace rto
{

/// @def DEBUG_CHANNEL Debugging information for channel
// #define DEBUG_CHANNEL

typedef boost::function<void (const NetworkPacket&, const EndPoint& , const EndPoint&)> ReceiveCb_t;

/**
  * Virtual channel: the class simulates the sending of packets over a network
  * This channel can simulate packet losses and channel delays
  */
class Channel
{
public:

  /**
    * Constructor
    * @param[in] uiPacketLossProbability Loss probability of packets sent over this channel
    * @param[in] fnOnRtp Callback required to deliver RTP packets sent over the channel to the receiver
    * @param[in] fnOnRtcp Callback required to deliver RTCP packets sent over the channel to the receiver
    * @param[in] rIoService io_service used for simulating delays
    */
  Channel(uint32_t uiPacketLossProbability, ReceiveCb_t fnOnRtp, ReceiveCb_t fnOnRtcp, boost::asio::io_service& rIoService)
    :m_uiPacketLossProbability(uiPacketLossProbability),
    m_fnOnRtp(fnOnRtp),
    m_fnOnRtcp(fnOnRtcp),
    m_uiIndex(0),
    m_rIoService(rIoService)
  {

  }

  /**
    * Sets the vector used to simulate delays
    * @param[in] vDelays The vector containing the delays
    */
  void setChannelDelays(std::vector<double>& vDelays)
  {
    m_vDelays = vDelays;
  }

  /**
    * Must be called to shutdown component cleanly
    */
  void stop()
  {
    // RTX
    std::for_each(m_mTimers.begin(), m_mTimers.end(), [this](const std::pair<boost::asio::deadline_timer*, boost::asio::deadline_timer*>& pair)
    {
      pair.second->cancel();
    });
    m_mTimers.clear();
  }

  /**
    * Simulates sending of RTP packet over channel
    * @param[in] rtcpPacket The RTCP packet to be sent over the channel
    * @param[in] from The endpoint that sent the packet
    * @param[in] to The endpoint that the packet is being sent to
    */
  void sendRtpPacketOverChannel(NetworkPacket rtpPacket, const EndPoint& from, const EndPoint& to)
  {
    sendPacketOverChannel(rtpPacket, from, to, true);
  }

  /**
    * Simulates sending of RTCP packet over channel
    * @param[in] rtcpPacket The RTCP packet to be sent over the channel
    * @param[in] from The endpoint that sent the packet
    * @param[in] to The endpoint that the packet is being sent to
    */
  void sendRtcpPacketOverChannel(NetworkPacket rtcpPacket, const EndPoint& from, const EndPoint& to)
  {
    sendPacketOverChannel(rtcpPacket, from, to, false);
  }

private:
  uint32_t getDelayUs()
  {
    uint32_t uiDelayUs = 0;
    if (!m_vDelays.empty())
    {
      // schedule packet according to one way delay model
      assert(m_uiIndex < m_vDelays.size());
      // schedule packet according to one way delay model
      uiDelayUs = static_cast<uint32_t>(m_vDelays[m_uiIndex] * 1000000 + 0.5);
      // make sure input from delay model is valid
      assert (uiDelayUs != 0);
      m_uiIndex = (m_uiIndex + 1) % m_vDelays.size();
    }
    return uiDelayUs;
  }

  void sendPacketOverChannel(NetworkPacket packet, const EndPoint& from, const EndPoint& to, bool bRtp)
  {
    uint32_t uiRand = rand()%100;
    if (uiRand >= m_uiPacketLossProbability)
    {
      uint32_t uiDelayUs = getDelayUs();
      boost::asio::deadline_timer* pTimer = new boost::asio::deadline_timer(m_rIoService);
      pTimer->expires_from_now(boost::posix_time::microseconds(uiDelayUs));
      pTimer->async_wait(boost::bind(&Channel::schedulePacket, this, _1, pTimer, packet, from, to, bRtp) );
  #ifdef DEBUG_CHANNEL
      if (bRtp)
          VLOG(10) << "RTP Packet scheduled from " << from << " to " << to << " in " << uiDelayUs << "us";
      else
        VLOG(10) << "RTCP packet scheduled from " << from << " to " << to << " in " << uiDelayUs << "us";
  #endif
      m_mTimers.insert(std::make_pair(pTimer, pTimer));
    }
  }

  void schedulePacket(const boost::system::error_code& ec, boost::asio::deadline_timer* pTimer, NetworkPacket packet, const EndPoint& from, const EndPoint& to, bool bRtp)
  {
    if (!ec)
    {
      if (bRtp)
        m_fnOnRtp(packet, from, to);
      else
        m_fnOnRtcp(packet, from, to);
    }

    m_mTimers.erase(pTimer);
    delete pTimer;
  }

private:
  /// Packet loss probability
  uint32_t m_uiPacketLossProbability;
  /// Callback for receiving RTP that has been sent over this channel
  ReceiveCb_t m_fnOnRtp;
  /// Callback for receiving RTCP that has been sent over this channel
  ReceiveCb_t m_fnOnRtcp;
  /// delay vector
  std::vector<double> m_vDelays;
  /// index of next delay reading
  std::size_t m_uiIndex;
  /// io_service for delay timer
  boost::asio::io_service& m_rIoService;
  /// Timers for delay simulation
  std::unordered_map<boost::asio::deadline_timer*, boost::asio::deadline_timer*> m_mTimers;
};

}
}

