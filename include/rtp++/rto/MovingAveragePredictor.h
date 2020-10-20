#pragma once
#include <rtp++/rto/PredictorBase.h>
#include <rtp++/rto/NormalDistribution.h>
#include <cpputil/RunningAverageQueue.h>

namespace rtp_plus_plus
{
namespace rto
{


template <typename T, typename R>
class MovingAveragePredictor : public PredictorBase<T, R>
{
public:
    MovingAveragePredictor(uint32_t uiQueueSize, uint32_t uiMinRequired, double dPrematureTimeoutProbability = 0.05)
      :m_queue(uiQueueSize),
      m_uiMinQueueSize(uiMinRequired),
      m_dPrematureTimeoutProbability(dPrematureTimeoutProbability)
    {
        VLOG(2) << "Constructor: Queue Size: " << uiQueueSize << " Premature TO: " << dPrematureTimeoutProbability;
    }

    void reset()
    {
      m_queue.clear();
    }

protected:
    virtual bool isReady() const
    {
      return m_queue.size() >= m_uiMinQueueSize;
    }
    virtual void doInsert(const T& val)
    {
      m_queue.insert(val);
    }
    virtual T predict()
    {
      return static_cast<T>(m_queue.getAverage());
    }
    virtual T calculateDelta()
    {
      // debug
      // return 0;
#if 1
      // use property of normal distribution that mean is 0
      double dStdDev = m_queue.getStandardDeviation();
      double dZ = NormalDistribution::getZScore(1 - m_dPrematureTimeoutProbability);
      double dOffset = dStdDev * dZ;
      T final = static_cast<T>(dOffset + 0.5);
      VLOG(15) << " Delta: " << final
              << " stddev: " << dStdDev
              << " dZ: " << dZ
              << " dOffset: " << dOffset;
      return final;
#else
      return static_cast<T>(3 * m_queue.getStandardDeviation() + 0.5);
#endif
    }
private:
    RunningAverageQueue<T, double> m_queue;
    uint32_t m_uiMinQueueSize;
    double m_dPrematureTimeoutProbability;
};

} // rto
} // rtp_plus_plus
