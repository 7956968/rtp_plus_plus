#pragma once
#include <vector>
#include <rtp++/util/TracesUtil.h>

namespace rtp_plus_plus
{
namespace rto
{

/**
 * @brief The ChannelDataSource class is used to share a data set between multiple Channels.
 * This is important in the case of say an RTP and an RTCP channel, or a forward and reverse channel.
 * In cases where the dataset changes to simulate network conditions, it is important that this change
 * is synchronised across RTP and RTCP, or forward and reverse channels.
 */
class ChannelDataSource
{
public:
  ChannelDataSource()
    :m_uiIndex(0)
  {
    VLOG(5) << "[" << this << "] Channel Data Source Constructor: Index: " << m_uiIndex;
  }

  ~ChannelDataSource()
  {
    VLOG(5) << "[" << this << "] Channel Data Source Destructor: Index: " << m_uiIndex;
  }

  bool loadDataSource(const std::string& sDataSource)
  {
    return TracesUtil::loadOneWayDelayDataFile(sDataSource, m_vDelays);
  }

  /**
   * @brief getDelayUs retrieves the next delay value. This method is not thread-safe in cases where
   * the data source is shared amonst multiple channels
   * @return
   */
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

private:

  std::vector<double> m_vDelays;
  uint32_t m_uiIndex;
};

} // rto
} // rtp_plus_plus
