#pragma once
#include <vector>
#include <boost/optional.hpp>
#include <rtp++/media/MediaSample.h>

namespace rtp_plus_plus
{
namespace media
{

/**
 * @brief Common base class for media sources.
 */
class MediaSource
{
public:
  /**
   * @brief Destructor
   */
  virtual ~MediaSource(){}
  /**
   * @brief isGood This method can be used to check if NAL units can be read. If the end of
   * stream is reached and the source is not configured to loop or if the maximum loop count
   * is reached, this will return false. 
   * @return if the MediaSource is in a state to be read from
   */
  virtual bool isGood() const = 0;
  /**
   * @brief MediaSource subclasses must implement this.
   * Each call to this method must return a single media sample.
   * @return A media sample if successful, a null pointer otherwise
   */
  virtual boost::optional<MediaSample> getNextMediaSample() = 0;
  /**
   * @brief MediaSource subclasses must implement this.
   * Each call to this method must return an entire access unit.
   */
  virtual std::vector<MediaSample> getNextAccessUnit() = 0;

private:

};

} // media
} // rtp_plus_plus
