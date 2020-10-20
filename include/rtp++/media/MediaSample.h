#pragma once

#include <deque>
#include <boost/make_shared.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/shared_ptr.hpp>

#include <cpputil/Buffer.h>


namespace rtp_plus_plus
{
namespace media
{

/**
* @brief This class represents a media sample.
* The source and channel id can be used to differentiate between different sources and channels
* where samples from two different sources might share the same channel id e.g. audio and video.
*/
class MediaSample
{
public:
  
  typedef boost::shared_ptr<MediaSample> ptr;
  
  /**
  * @brief Named constructor for shared_ptr creation
  *
  * @return 
  */
  static MediaSample::ptr create()
  {
    return boost::make_shared<MediaSample>();
  }

 MediaSample()
   :m_dStartTime(0.0),
     m_dDecodingTime(0.0),
     m_bMarker(false),
     m_uiSourceId(0),
     m_uiChannelId(0),
     m_iDecodingOrderNumber(-1),
     m_iFlowIdHint(-1),
     m_iStartCodeLengthHint(-1),
     m_bNaluContainsStartCode(false)
 {

 }

  double getStartTime() const { return m_dStartTime; }
  void setStartTime( double dStartTime ) { m_dStartTime = dStartTime; }

  double getDecodingTime() const { return m_dDecodingTime; }
  void setDecodingTime( double dDecodingTime ) { m_dDecodingTime = dDecodingTime; }

  // utility method
  uint32_t getPayloadSize() const { return m_data.getSize(); }

  const Buffer& getDataBuffer() const { return m_data;}
  // this class takes ownership of the data
  void setData(uint8_t* pData, uint32_t uiSize)
  {
    m_data.setData(pData, uiSize);
  }
  void setData(Buffer data)
  {
    m_data = data;
  }

  bool isMarkerSet() const { return m_bMarker; }
  void setMarker(bool bValue) { m_bMarker = bValue; }

  uint32_t getSourceId() const { return m_uiSourceId; }
  void setSourceId(uint32_t uiSourceId) { m_uiSourceId = uiSourceId; }

  uint32_t getChannelId() const { return m_uiChannelId;}
  void setChannelId(uint32_t uiChannelId) { m_uiChannelId = uiChannelId; }

  uint32_t getRtpTime() const { return m_uiRtpTime; }
  void setRtpTime(uint32_t uiRtpTime) { m_uiRtpTime = uiRtpTime; }

  int16_t getDecodingOrderNumber() const { return m_iDecodingOrderNumber;}
  void setDecodingOrderNumber(int16_t iDecodingOrderNumber) { m_iDecodingOrderNumber = iDecodingOrderNumber; }

  int32_t getFlowIdHint() const { return m_iFlowIdHint;}
  void setFlowIdHint(int32_t iFlowIdHint) { m_iFlowIdHint = iFlowIdHint; }

  int32_t getStartCodeLengthHint() const { return m_iStartCodeLengthHint;}
  void setStartCodeLengthHint(int32_t iHint) { m_iStartCodeLengthHint = iHint; }

  bool doesNaluContainsStartCode() const { return m_bNaluContainsStartCode; }
  void setNaluContainsStartCode(bool bNaluContainsStartCode) { m_bNaluContainsStartCode = bNaluContainsStartCode; }
  
  boost::posix_time::ptime getPresentationTime() const { return m_tPresentation; }
  void setPresentationTime(const boost::posix_time::ptime& tPresentation) { m_tPresentation = tPresentation; }

private:  
  /// start time of media sample
  double m_dStartTime;
  /// Optional decoding time
  double m_dDecodingTime;
  /// Buffer to store media data
  Buffer m_data;
  /// marker bit that can be used to show special events in the stream such
  /// as I-Frames, RTCP synchronisation, etc
  bool m_bMarker;
  /// optional RtpTime which can be used to determine new AUs and determine the length of start codes
  uint32_t m_uiRtpTime;
  /// optional source ID which can be used to differentiate between different sources
  uint32_t m_uiSourceId;
  /// optional channel ID which can be used to differentiate between different channels
  uint32_t m_uiChannelId;
  /// Optional decoding order number, -1 if not set
  int16_t m_iDecodingOrderNumber;
  /// Optional flow ID hint for MPRTP, -1 if not set
  int32_t m_iFlowIdHint;
  /// Optional start code hint for H.264 to write correct start codes
  int32_t m_iStartCodeLengthHint;
  /// to handle encoders that create NAL units with start code
  bool m_bNaluContainsStartCode;
  /// Presentation time
  boost::posix_time::ptime m_tPresentation;
};

typedef std::deque<MediaSample> MediaSampleQueue_t;

}
}
