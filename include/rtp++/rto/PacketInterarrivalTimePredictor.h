#pragma once
#include <rtp++/rto/AR2Predictor.h>
#include <rtp++/rto/NormalDistribution.h>

namespace rtp_plus_plus
{
namespace rto
{

/**
  * This class builds on the AR2 predictor by specifying the premature
  * timeout probability as specified in Begen's RTO estimation paper
  * To take this factor into account it adds tau to the prediction
  */
template <typename T, typename R>
class PacketInterarrivalTimePredictor : public AR2Predictor<T, R>
{
public:
  PacketInterarrivalTimePredictor(double dPrematureTimeoutProbability = 0.05)
    :m_dPrematureTimeoutProbability(dPrematureTimeoutProbability)
  {
    // TODO: how does getZScore work at the boundaries
    if (m_dPrematureTimeoutProbability >= 1.0 || m_dPrematureTimeoutProbability < 0.001)
    {
      LOG(WARNING) << "Invalid premature timeout probability: " << m_dPrematureTimeoutProbability
                   << " resetting to 0.05";
      m_dPrematureTimeoutProbability = 0.05;
    }
  }

  virtual T calculateDelta()
  {
//#define IGNORE_DELTA
#ifdef IGNORE_DELTA
    // TEST
    return 0;
#endif
    double dStdDevError = PredictorBase<T,R>::getErrorStandardDeviation();
    // use property of normal distribution that mean is 0
    double dZ = NormalDistribution::getZScore(1 - m_dPrematureTimeoutProbability);
    double dOffset = dStdDevError * dZ;
    T final = static_cast<T>(dOffset + 0.5);
    VLOG(2) << " Delta: " << final
            << " stddev: " << dStdDevError
            << " dZ: " << dZ
            << " dOffset: " << dOffset;
    return final;
  }

private:
  double m_dPrematureTimeoutProbability;
};

} // rto
} // rtp_plus_plus
