#pragma once
#include <deque>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/bind.hpp>
#include <rtp++/scheduling/RtpScheduler.h>

namespace rtp_plus_plus
{

/**
 * @brief The PacedRtpScheduler class
 */
class PacedRtpScheduler : public RtpScheduler
{
public:
  /**
   * @brief PacedRtpScheduler
   * @param pRtpSession
   */
  PacedRtpScheduler(RtpSession::ptr pRtpSession, boost::asio::io_service& ioService)
    :RtpScheduler(pRtpSession),
      m_ioService(ioService),
      m_timer(m_ioService)
  {

  }
  /**
   * @brief ~PacedRtpScheduler destructor
   */
  ~PacedRtpScheduler()
  {
    // could add stop method?
    m_timer.cancel();
  }
  /**
   * @brief scheduleRtpPackets paces the sending of packets if there is more than one RTP packet.
   * @param vRtpPackets
   */
  virtual void scheduleRtpPackets(const std::vector<RtpPacket>& vRtpPackets)
  {
    assert(!vRtpPackets.empty());
    // for now we assume a framerate of 25fps.
    // later we can measure it, or set it via parameter
    // if no send is outstanding, send it immediately
    doScheduleRtpPackets(vRtpPackets);
  }

protected:

  /**
   * @brief doScheduleRtpPackets
   * @param vRtpPackets
   *
   * This code assumes that the calling code and this code are using the same IO service
   * Hence no locking of deque
   */
  virtual void doScheduleRtpPackets(const std::vector<RtpPacket>& vRtpPackets)
  {
    auto it = vRtpPackets.begin();
    // check if something is currently being scheduled
    bool bBusyScheduling = true;
    if (m_qRtpPackets.empty())
    {
      bBusyScheduling = false;
      m_pRtpSession->sendRtpPacket(*it);
      ++it;
    }
    else
    {
      // schedule this packet to be sent at a later stage
      LOG(WARNING) << "There are still " << m_qRtpPackets.size() << " outstanding packets that haven't been sent yet";
    }

    if (it != vRtpPackets.end())
    {
      //store packets for later transmission
      m_qRtpPackets.insert(m_qRtpPackets.end(), it, vRtpPackets.end());
    }

    if (!bBusyScheduling && !m_qRtpPackets.empty())
    {
      // no pacing in progress, start the timer
      m_timer.expires_from_now(boost::posix_time::milliseconds(10)); // use fixed timer for now: should actually be adaptive
      m_timer.async_wait(boost::bind(&PacedRtpScheduler::onScheduleTimeout, this, boost::asio::placeholders::error));
    }
    else
    {
      // the timeout will schedule the next sample anyway
    }
  }
  /**
   * @brief onScheduleTimeout handler for timer timeout
   * @param ec
   */
  void onScheduleTimeout(const boost::system::error_code& ec )
  {
    if (!ec)
    {
      auto it = m_qRtpPackets.begin();
      m_pRtpSession->sendRtpPacket(*it);
      m_qRtpPackets.pop_front();
      if (!m_qRtpPackets.empty())
      {
        // no pacing in progress, start the timer
        m_timer.expires_from_now(boost::posix_time::milliseconds(10)); // use fixed timer for now: should actually be adaptive
        m_timer.async_wait(boost::bind(&PacedRtpScheduler::onScheduleTimeout, this, boost::asio::placeholders::error));
      }
    }
  }

private:
  boost::asio::io_service& m_ioService;
  boost::asio::deadline_timer m_timer;
  std::deque<RtpPacket> m_qRtpPackets;
};

} // rtp_plus_plus
