#pragma once
#include <vector>
#include <boost/system/error_code.hpp>
#include <rtp++/experimental/ICooperativeCodec.h>
#include <rtp++/media/ITransform.h>

namespace rtp_plus_plus {
namespace media {

class IVideoCodecTransform : public ITransform, public ICooperativeCodec
{

};

} // media
} // rtp_plus_plus

