#pragma once
#include <cstdint>
#include <vector>
#include <cpputil/Buffer.h>
#include <cpputil/StringTokenizer.h>
#include <rtp++/media/MediaSample.h>

namespace rtp_plus_plus
{

class IDataModel
{
public:
  ~IDataModel()
  {

  }

  /**
   * @brief fillBuffer fills buffer with random data. Subclass this to implement
   * other types of data.
   * @param buffer
   */
  virtual void fillBuffer(Buffer& buffer)
  {
    char* pBuffer = reinterpret_cast<char*>(const_cast<uint8_t*>(buffer.data()));
    for (size_t j = 4; j < buffer.getSize(); ++j)
    {
      pBuffer[j] = rand() % 256;
    }
  }
};

class H264DataModel : public IDataModel
{
public:
  H264DataModel()
    :m_bFirst(true)
  {

  }

  virtual void fillBuffer(Buffer& buffer)
  {
    // fill with random junk
    IDataModel::fillBuffer(buffer);
    // now write NAL unit types
    char* pBuffer = reinterpret_cast<char*>(const_cast<uint8_t*>(buffer.data()));
    if (m_bFirst)
    {
      m_bFirst = false;
      // IDR frame
      pBuffer[0] = 0x65;
    }
    else
    {
      // p-frame
      pBuffer[0] = 0x41;
    }
  }

private:
  bool m_bFirst;
};

/**
 * @brief The VideoModel class allows the modelling of a video source based on frame sizes
 */
class IVideoModel
{
public:
  virtual ~IVideoModel(){}

  /**
   * @brief getNextFrame returns the size of the next frame according to the model.
   * @param[out] uiFramesize
   * @return true if there was more data to be collected, and false otherwise
   */
  virtual bool getNextFrame(uint32_t& uiFramesize, media::MediaSample& mediaSample) = 0;
};

class BitrateVideoModel : public IVideoModel
{
public:
  explicit BitrateVideoModel(uint32_t uiFps, uint32_t uiTargetBitrateKbps)
    :m_uiFrameSize(uiTargetBitrateKbps / uiFps * 1000 / 8)
  {
    assert(uiFps > 0);
    m_buffer = Buffer(new uint8_t[m_uiFrameSize], m_uiFrameSize);
  }

  virtual bool getNextFrame(uint32_t& uiFramesize, media::MediaSample& mediaSample)
  {
    uiFramesize = m_uiFrameSize;
    mediaSample.setData(m_buffer);
    return true;
  }

private:
  Buffer m_buffer;
  uint32_t m_uiFrameSize;
};

class DynamicBitrateVideoModel : public IVideoModel
{
  struct Segment
  {
    Segment()
      :BitrateKbps(0),
        DurationSeconds(0),
        FrameSize(0),
        FrameCount(0)
    {

    }

    Segment(uint32_t uiBitrateKbps, uint32_t uiDurationSeconds, uint32_t uiFrameSize, uint32_t uiFrameCount)
      :BitrateKbps(uiBitrateKbps),
        DurationSeconds(uiDurationSeconds),
        FrameSize(uiFrameSize),
        FrameCount(uiFrameCount)
    {

    }

    uint32_t BitrateKbps;
    int32_t DurationSeconds;
    uint32_t FrameSize;
    uint32_t FrameCount;
  };

public:
  explicit DynamicBitrateVideoModel(uint32_t uiFps, const std::string& sVideoBitrateModel)
    :m_uiCurrentSegment(0),
      m_uiCurrentSegmentCount(0)
  {
    assert(uiFps > 0);
    // split bitrate string
    std::vector<std::string> vSegments = StringTokenizer::tokenize(sVideoBitrateModel, "|", true, true);
    // only the last segment can not have a duration
    for (const std::string& sSegment : vSegments)
    {
      std::vector<std::string> vSegmentInfo = StringTokenizer::tokenize(sSegment, ":", true, true);
      bool bDummy;
      if (vSegmentInfo.size() == 2)
      {
        uint32_t uiBitrate = convert<uint32_t>(vSegmentInfo[0], bDummy);
        assert(bDummy);
        uint32_t uiDurationS = convert<uint32_t>(vSegmentInfo[1], bDummy);
        assert(bDummy);
        m_vSegments.push_back(Segment(uiBitrate, uiDurationS, uiBitrate/uiFps * 1000/8, uiDurationS * uiFps));
      }
      else if (vSegmentInfo.size() == 1)
      {
        uint32_t uiBitrate = convert<uint32_t>(vSegmentInfo[0], bDummy);
        assert(bDummy);
        m_vSegments.push_back(Segment(uiBitrate, -1, uiBitrate/uiFps * 1000/8, 0));
      }
      else
      {
        LOG(WARNING) << "Invalid segment";
      }
    }
  }

  virtual bool getNextFrame(uint32_t& uiFramesize, media::MediaSample& mediaSample)
  {
    uiFramesize = m_vSegments[m_uiCurrentSegment].FrameSize;

    uint32_t uiKbps = m_vSegments[m_uiCurrentSegment].BitrateKbps;
    uint32_t uiDurationS = m_vSegments[m_uiCurrentSegment].DurationSeconds;
    uint32_t uiFrameCount = m_vSegments[m_uiCurrentSegment].FrameCount;

    if (m_uiCurrentSegmentCount == 0)
    {
      VLOG(2) << "Initialising model kbps: " << uiKbps
              << " duration: " << uiDurationS
              << "s Frame size: " << uiFramesize
              << " count: " << uiFrameCount;
      // init media sample
      m_buffer = Buffer(new uint8_t[uiFramesize], uiFramesize);
      mediaSample.setData(m_buffer);
    }

    if (m_vSegments[m_uiCurrentSegment].FrameCount != 0 && // this is for the case where the last segment is open-ended
        m_uiCurrentSegmentCount == m_vSegments[m_uiCurrentSegment].FrameCount)
    {
      m_uiCurrentSegment = (m_uiCurrentSegment + 1) % m_vSegments.size();
      m_uiCurrentSegmentCount = 0;
      VLOG(2) << "Next model kbps: " << uiKbps
              << " duration: " << uiDurationS
              << "s Frame size: " << uiFramesize
              << " count: " << uiFrameCount;
    }
    else
    {
      ++m_uiCurrentSegmentCount;
    }
    return true;
  }

private:

  std::vector<Segment> m_vSegments;
  uint32_t m_uiCurrentSegment;
  uint32_t m_uiCurrentSegmentCount;
  Buffer m_buffer;
};

} // namespace rtp_plus_plus
