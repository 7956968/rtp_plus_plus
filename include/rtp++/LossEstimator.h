#pragma once
#include <algorithm>
#include <deque>
#include <set>
#include <rtp++/rfc3550/Rfc3550.h>

namespace rtp_plus_plus
{

/**
 * This class was written for the purpose of logging lost packet to file for post-inspection.
 * This class keeps track of which packets have been received and lost using the RTP sequence numbers.
 * The calling class is expected to periodically call getEstimatedLostSequenceNumbers which returns
 * packets that have not been received by the time the method is called. These packets will not be returned
 * in subsequent calls of the method.
 *
 * This method partially reuses the algorithm from RFC3550 to detect lost and reordered packets.
 *
 * DEPRECATED: This class is not currently used
 */
class LossEstimator
{
public:
    LossEstimator()
        :m_uiMax(rfc3550::MAX_DROPOUT)
    {
    }

    /*
     * This method resets the internal state of the loss estimator
     */
    void clear()
    {
       m_vSequenceNumbers.clear();
       m_vExpected.clear();
    }

    /*
     * This method should be called as each RTP packet is received
     */
    void addSequenceNumber(uint16_t uiSN)
    {
        if (m_vSequenceNumbers.empty())
        {
            init(uiSN);
        }
        else
        {
            // limit queue size
            if (m_vSequenceNumbers.size() == m_uiMax)
                m_vSequenceNumbers.pop_front();
            m_vSequenceNumbers.push_back(uiSN);

            uint16_t uiDelta = uiSN - m_uiMaxSeq;
            if (uiDelta < rfc3550::MAX_DROPOUT)
            {
                // insert missing sequence numbers into expected list
                for (size_t i = 1; i < uiDelta; ++i)
                {
                    uint16_t uiMissing = m_uiMaxSeq + i;
                    // insert missing sequence numbers into expected packet list
                    auto itExisting = std::find_if(m_vSequenceNumbers.rbegin(), m_vSequenceNumbers.rend(), [uiMissing](uint16_t uiExistingSN)
                    {
                      return uiMissing == uiExistingSN;
                    });
                    if (itExisting == m_vSequenceNumbers.rend())
                      m_vExpected.insert(uiMissing);
                }

                // remove current SN from expected list
                auto it = m_vExpected.find(uiSN);
                if (it != m_vExpected.end())
                    m_vExpected.erase(it);

                // insert next expected SN into expected if it hasn't been received already
                uint16_t uiNext = uiSN + 1;
                auto it2 = std::find_if(m_vSequenceNumbers.rbegin(), m_vSequenceNumbers.rend(), [uiNext](uint16_t uiExistingSN)
                {
                  return uiExistingSN == uiNext;
                });
                if (it2 == m_vSequenceNumbers.rend())
                {
                  m_vExpected.insert(uiNext);
                }

                m_uiMaxSeq = uiSN;
            }
            else if (uiDelta <= rfc3550::RTP_SEQ_MOD - rfc3550::MAX_MISORDER)
            {
                  // either huge jump or big misorder
                  if (uiSN == m_uiBadSN)
                  {
                  /*
                  * Two sequential packets -- assume that the other side
                  * restarted without telling us so just re-sync
                  * (i.e., pretend this was the first packet).
                  */
                      init(uiSN);
                  }
                  else
                  {
                      m_uiBadSN = (uiSN + 1) & (rfc3550::RTP_SEQ_MOD-1);
                  }
            }
            else
            {
                  // misordered packet
                  // remove current SN from expected list
                  auto it = m_vExpected.find(uiSN);
                  if (it != m_vExpected.end())
                    m_vExpected.erase(it);
            }
            return;
        }
    }

    /**
     * This method retrieves a vector of sequence numbers that have not been
     * received until the point the method was called and are assumed to be lost.
     * Calling this method removes the sequence numbers from the set of sequence
     * numbers assumed to be lost
     */
    std::vector<uint16_t> getEstimatedLostSequenceNumbers()
    {
        std::vector<uint16_t> vLost = copyLostSequenceNumbers();
        // remove them from the expected set
        std::for_each(vLost.begin(), vLost.end(), [this](uint16_t uiSN)
        {
            auto it = m_vExpected.find(uiSN);
            if (it != m_vExpected.end())
                m_vExpected.erase(it);
        });
        return vLost;
    }

private:

    std::vector<uint16_t> copyLostSequenceNumbers()
    {
      // get last sequence number
      auto it = m_vSequenceNumbers.rbegin();
      if (it != m_vSequenceNumbers.rend())
      {
          std::vector<uint16_t> vPossiblyLost;
          // need to isolate the case where wrap around has occurred
          uint16_t uiMin = m_vSequenceNumbers[0];
          uint16_t uiMax = *it;
          uiMin = *std::min_element(m_vSequenceNumbers.begin(), m_vSequenceNumbers.end());
          uiMax = *std::max_element(m_vSequenceNumbers.begin(), m_vSequenceNumbers.end());

          if (uiMax - uiMin > (USHRT_MAX >> 1))
          {
              // the values lie at the beginning and end of the unsigned range
              // copy values into signed containers so that we can sort them
              std::deque<int16_t> vSignedSequenceNumbers(m_vSequenceNumbers.size());
              std::copy(m_vSequenceNumbers.begin(), m_vSequenceNumbers.end(), vSignedSequenceNumbers.begin());
              std::sort(vSignedSequenceNumbers.begin(), vSignedSequenceNumbers.end());

              std::set<int16_t> vSignedExpected;
              std::copy(m_vExpected.begin(), m_vExpected.end(), std::inserter( vSignedExpected, vSignedExpected.begin() ) );

              auto itSigned = vSignedSequenceNumbers.rbegin();
              // removing RFC3550_MAX_MISORDER: seems to cause unit tests to fail
              int16_t iMax = *itSigned /*- RFC3550_MAX_MISORDER*/; // try take reordering into account (esp. for mprtp)
              // treat numbers as ints for sorting purposes
              std::for_each(vSignedExpected.begin(), vSignedExpected.end(),[&vPossiblyLost, iMax](int16_t iSN)
              {
                if (iSN < (int16_t)iMax )
                {
                   vPossiblyLost.push_back(iSN);
                }
              });

          }
          else
          {
            // Here the values are only in one section of the unsigned range
            std::for_each(m_vExpected.begin(), m_vExpected.end(),[&vPossiblyLost, uiMax](uint16_t uiSN)
            {
              // Commenting out RFC3550_MAX_MISORDER: this causes unit tests to fail 
              // in that still expected SNs are reported as lost
              if (uiSN < uiMax /*- RFC3550_MAX_MISORDER*/) // try take reordering into account (esp. for mprtp)
              {
                 vPossiblyLost.push_back(uiSN);
              }
              else
              {
                // check for wrap around: TODO: can this ever happen?
                if (uiSN - uiMax > (USHRT_MAX >> 1))
                {
                  assert(false);
                  vPossiblyLost.push_back(uiSN);
                }
              }
            });
          }
          return vPossiblyLost;
      }
      else
          return std::vector<uint16_t>();
    }

    void init(uint16_t uiSN)
    {
      // first SN
      m_vSequenceNumbers.push_back(uiSN);
      m_vExpected.insert(uiSN + 1);
      m_uiMaxSeq = uiSN;
      m_uiBadSN = UINT_MAX;
    }

    /// maximum number of sequence number used in detection
    uint32_t m_uiMax;
    /// maximum sequence number
    uint16_t m_uiMaxSeq;
    /// bad SN for SN restarting
    uint32_t m_uiBadSN;

    /// previously received sequence numbers
    std::deque<uint16_t> m_vSequenceNumbers;
    /// currently expected sequence numbers
    std::set<uint16_t> m_vExpected;

};

}
