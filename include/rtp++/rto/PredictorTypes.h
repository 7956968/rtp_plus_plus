#pragma once
#include <boost/optional.hpp>
#include <cpputil/GenericParameters.h>
#include <rtp++/application/ApplicationParameters.h>

namespace rtp_plus_plus
{
namespace rto
{

enum PredictorType
{
    PT_NONE,
    PT_SIMPLE,
    PT_MOVING_AVERAGE,
    PT_AR2
};

enum MpPredictorType
{
    PT_MP_NONE,
    PT_MP_SINGLE,
    PT_MP_CROSSPATH,
    PT_MP_COMPARE
};

static PredictorType getPredictorType(const GenericParameters &applicationParameters)
{
  boost::optional<std::string> sPredictor = applicationParameters.getStringParameter(app::ApplicationParameters::pred);
  if (sPredictor)
  {
      std::string sType = *sPredictor;
      if (sType == app::ApplicationParameters::mavg)
          return PT_MOVING_AVERAGE;
      else if (sType == app::ApplicationParameters::ar2)
          return PT_AR2;
      else if (sType == app::ApplicationParameters::simple)
          return PT_SIMPLE;
  }
  return PT_SIMPLE;
}

static MpPredictorType getMpPredictorType(const GenericParameters &applicationParameters)
{
  boost::optional<std::string> sPredictor = applicationParameters.getStringParameter(app::ApplicationParameters::mp_pred);
  if (sPredictor)
  {
      std::string sType = *sPredictor;
      if (sType == app::ApplicationParameters::mp_single)
          return PT_MP_SINGLE;
      else if (sType == app::ApplicationParameters::mp_cross)
          return PT_MP_CROSSPATH;
      else if (sType == app::ApplicationParameters::mp_comp)
          return PT_MP_COMPARE;
  }
  return PT_MP_NONE;
}

} // rto
} // rtp_plus_plus
