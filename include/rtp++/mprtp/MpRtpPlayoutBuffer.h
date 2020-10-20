//#pragma once
//#include <rtp++/RtpPlayoutBuffer.h>

//// -DDEBUG_RTP_LOSSES: logs received sequence numbers
//#define DEBUG_RTP_LOSSES

//namespace mprtp
//{

//// TODO: allow retrieval of flow specific discard metrics
//class MpRtpPlayoutBuffer : public RtpPlayoutBuffer
//{
//public:
//  typedef std::unique_ptr<MpRtpPlayoutBuffer> ptr;

//  static ptr create( )
//  {
//    return std::unique_ptr<MpRtpPlayoutBuffer>( new MpRtpPlayoutBuffer() );
//  }

//  MpRtpPlayoutBuffer(uint32_t uiPlayoutBufferLatency = MAX_LATENCY_MS)
//      :RtpPlayoutBuffer(uiPlayoutBufferLatency)
//  {

//  }

//  virtual bool addRtpPacket(const RtpPacket& packet, boost::posix_time::ptime& tPlayout)
//  {
//    // TODO: need to make sure that packets arriving for alternate path are not considered late
//    // search for MPRTP header extension
//    uint16_t uiFlowId = USHRT_MAX;
//    uint16_t uiFSSN = USHRT_MAX;
//    rfc5285::RtpHeaderExtension::ptr pHeaderExtension = packet.getHeader().getHeaderExtension();
//    if (pHeaderExtension)
//    {
//      // Potentially expensive???: create a work around if this proves to be a problem
//      MpRtpSubflowRtpHeader* pSubflowHeader = dynamic_cast<MpRtpSubflowRtpHeader*>(pHeaderExtension.get());
//      assert (pSubflowHeader);
//      uiFlowId = pSubflowHeader->getFlowId();
//      uiFSSN = pSubflowHeader->getFlowSpecificSequenceNumber();
//    }
//#ifdef DEBUG_RTP_LOSSES
//    VLOG(10) << "Adding packet with SN " << packet.getSequenceNumber() << " Flow Id: " << uiFlowId << " FSSN: " << uiFSSN;
//#endif

//    boost::mutex::scoped_lock l(m_listLock);
//    if (!m_playoutBufferList.empty())
//    {
//      uint32_t uiTs = packet.getRtpTimestamp();
//      // check if node with TS exists in list: update node
//      auto it = std::find_if(m_playoutBufferList.begin(), m_playoutBufferList.end(), [&uiTs](PlayoutBufferNode& node)
//      {
//        return (node.getRtpTs() == uiTs);
//      });
//      if (it != m_playoutBufferList.end())
//      {
//        // update existing entry
//        if (!it->insertIfUnique(packet))
//          ++m_uiTotalDuplicates;
//        // we don't schedule another playout on an update!
//        // Only the first RTP packet related to an RTP timestamp
//        // results in a playout time being returned from this method
//        return false;
//      }
//      else
//      {
//        // compare timestamp against oldest packet in list to see if it is late
//        uint32_t uiOldestTs = m_playoutBufferList.front().getRtpTs();
//        uint32_t uiOldestSn = m_playoutBufferList.front().getOldestSN();
//        if (uiTs < uiOldestTs)
//        {
//          // must take wrap around into account
//          uint32_t uiDiff = uiOldestTs - uiTs;
//          if ( uiDiff < UINT_MAX>>1 )
//          {
//            if (pHeaderExtension)
//            {
//              LOG(WARNING) << "RTP discard: packet is late: Flow: " << uiFlowId
//                           << " SN: " << packet.getExtendedSequenceNumber() << " TS:" << packet.getRtpTimestamp()
//                           << " Oldest TS: " << uiOldestTs << " SN: " << uiOldestSn;
//            }
//            else
//            {
//              // packet is late
//              LOG(WARNING) << "RTP discard: packet is late: Flow: N/A SN: " << packet.getExtendedSequenceNumber() << " TS:" << packet.getRtpTimestamp()
//                           << " Oldest TS: " << uiOldestTs << " SN: " << uiOldestSn;
//            }
//            ++m_uiTotalLatePackets;
//            return false;
//          }
//          else
//          {
//            // Take RTP TS wrap around into account
//            insertAccordingToRtpTs(packet, tPlayout, true);
//            return true;
//          }
//        }
//        else
//        {
//          // insert packet before 1st packet with bigger timestamp or the end
//          insertAccordingToRtpTs(packet, tPlayout, false);
//          return true;
//        }
//      }
//    }
//    else
//    {
//      // just insert the first packet
//      insertAccordingToRtpTs(packet, tPlayout, false);
//      return true;
//    }
//  }

//protected:
//  /**
//    * basic implementation that calculates the playout time based on the arrival time of the first RTP packet
//    * This MPRTP implementation has to account for timestamps that are decreasing since one path may be slower
//    * than another
//    */
//  virtual boost::posix_time::ptime calculatePlayoutTime(const RtpPacket& packet)
//  {
//    // This is extremely inefficient since we're retrieving the extension header multiple times
//    // create a work around if this proves to be a problem or refactor at a later stage
//    rfc5285::RtpHeaderExtension::ptr pHeaderExtension = packet.getHeader().getHeaderExtension();
//    assert (pHeaderExtension);
//    MpRtpSubflowRtpHeader* pSubflowHeader = dynamic_cast<MpRtpSubflowRtpHeader*>(pHeaderExtension.get());
//    assert (pSubflowHeader);
//    uint16_t uiFlowId = pSubflowHeader->getFlowId();
//    uint16_t uiFSSN = pSubflowHeader->getFlowSpecificSequenceNumber();

//    if (m_tFirstPacket.is_not_a_date_time())
//    {
//      // store arrival time of first packet as a reference
//      m_tFirstPacket = packet.getArrivalTime() + boost::posix_time::milliseconds(m_uiPlayoutBufferLatencyMs);
//      m_uiFirstRtpTime = packet.getRtpTimestamp();
//      m_uiPreviousRtpTime = m_uiFirstRtpTime;
//      LOG(INFO) << "Using " << m_uiPlayoutBufferLatencyMs << "ms playout buffer. Clock frequency: " << m_uiClockFrequency << "Hz";
//      m_tPrevPlayoutTime = m_tFirstPacket;
//      return m_tPrevPlayoutTime;
//    }
//    else
//    {
//      // there are four cases here
//      uint32_t uiTsCurrentPacket = packet.getRtpTimestamp();
//      bool bWrapAroundOccurred = false;
//      bool bIsLateOrOld = isLateOrOld(uiTsCurrentPacket, bWrapAroundOccurred);
//      if (!bIsLateOrOld && bWrapAroundOccurred)
//         ++m_uiWrapAroundCounter;

//      // now handle offset in RTP time from first packet to calculate relative playout time
//      uint32_t uiTotalWraparounds = m_uiWrapAroundCounter;

//      if (packet.getRtpTimestamp() < m_uiFirstRtpTime)
//      {
//        // wrap around is not 'complete'
//        --uiTotalWraparounds;
//      }

//      uint32_t uiDiffFromFirstPacket = packet.getRtpTimestamp() - m_uiFirstRtpTime;
//      uint64_t uiTotalTimeElapsedSinceFirstPacket = (uiTotalWraparounds * UINT32_MAX) + uiDiffFromFirstPacket;
//      // convert total time diff to real-time
//      double dSeconds = uiTotalTimeElapsedSinceFirstPacket/static_cast<double>(m_uiClockFrequency);
//      uint32_t uiMilliseconds = static_cast<uint32_t>(dSeconds * 1000 + 0.5);
//#if 0
//      LOG(INFO) << "Playout time " << uiMilliseconds << "ms from start";
//#endif

//      boost::posix_time::ptime tPlayoutTime = m_tFirstPacket + boost::posix_time::milliseconds(uiMilliseconds);
//  #if 0
//      int32_t iDiffFromPreviousPacket = static_cast<int32_t>(uiTsCurrentPacket - m_uiPreviousRtpTime);
//      VLOG(5) << "Diff to max RTP: " << iDiffFromPreviousPacket
//              << " TS: " << (tPlayoutTime - m_tPrevPlayoutTime).total_milliseconds() << "ms"
//              << " Diff to start RTP: "  << uiTotalTimeElapsedSinceFirstPacket
//              << " Diff to Start TS: " << (tPlayoutTime - m_tFirstPacket).total_milliseconds() << "ms";

//  #endif

//      // update the previous time stamp references
//      if (!bIsLateOrOld)
//      {
//          m_uiPreviousRtpTime = uiTsCurrentPacket;
//          m_tPrevPlayoutTime = tPlayoutTime;
//      }

//      return tPlayoutTime;
//    }
//  }

//private:
//  bool isLateOrOld(uint32_t uiTsCurrentPacket, bool& bWrapAroundOccurred) const
//  {
//    if (uiTsCurrentPacket < m_uiPreviousRtpTime)
//    {
//      // either the packet is old or wrap around has occurred
//      if (m_uiPreviousRtpTime - uiTsCurrentPacket > UINT32_MAX/2 )
//      {
//        // wrap-around has occurred: normal RTP operation
//        bWrapAroundOccurred = true;
//        return false;
//      }
//      else
//      {
//        bWrapAroundOccurred = false;
//        // packet is old or late
//        return true;
//      }
//    }
//    else
//    {
//      // either the packet is old or wrap around has occurred
//      if (uiTsCurrentPacket - m_uiPreviousRtpTime > UINT32_MAX/2 )
//      {
//        // wrap-around has occurred: packet is old or late
//        bWrapAroundOccurred = true;
//        return true;
//      }
//      else
//      {
//        // normal RTP operation
//        bWrapAroundOccurred = false;
//        return false;
//      }
//    }
//  }
//};

//}
