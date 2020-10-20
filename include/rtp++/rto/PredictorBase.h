#pragma once
#include <numeric>
#include <stdexcept>
#include <vector>
#include <cpputil/Conversion.h>

namespace rtp_plus_plus
{
namespace rto
{

template <typename T, typename R>
class PredictorBase
{
public:
  virtual ~PredictorBase()
  {
    VLOG(15) << "History: " << ::toString(m_vHistory);
    VLOG(15) << "Predictions: " << ::toString(m_vPredictions);
    VLOG(15) << "Errors: " << ::toString(m_vErrors);
  }

  T getLastPrediction() const
  {
    if (!isReady()) throw std::runtime_error("Predictor not ready");
    return m_pred + m_delta;
  }

  double getErrorStandardDeviation() const
  {
    if (m_vErrors.empty()) return 0.0;
    double average = std::accumulate(m_vErrors.begin(), m_vErrors.end(), 0) / static_cast<R>(m_vErrors.size());
    std::vector<R> vVariance(m_vErrors.size());
    std::transform(m_vErrors.begin(), m_vErrors.end(), vVariance.begin(), [average](R val)
    {
      return static_cast<R>(val - average + 0.5);
    });
#if 0
    double dSumErrorSquared = std::inner_product(m_vErrors.begin(), m_vErrors.end(), m_vErrors.begin(), 0.0);
#else
    double dSumErrorSquared = std::inner_product(vVariance.begin(), vVariance.end(), vVariance.begin(), 0.0);
#endif
    return std::sqrt(dSumErrorSquared / m_vErrors.size());
    }

  virtual void reset() = 0;

  virtual bool isReady() const = 0;

  virtual void insert(const T& val)
  {
    m_vHistory.push_back(val);
    doInsert(val);
    if (isReady())
    {
      if (m_bFirst)
      {
        m_vErrors.push_back(m_pred - val);
      }
      else
      {
        m_bFirst = true;
      }
      // keep prediction and delta separately since we don't want to the delta to
      // factor into the error
      m_pred = predict();
      m_delta = calculateDelta();
      VLOG(15) << " Prediction: " << m_pred
        << " delta: " << m_delta
        << " Total: " << m_pred + m_delta;
      m_vPredictions.push_back(m_pred);

    }
  }


protected:
  PredictorBase()
    :m_bFirst(false){}
  virtual void doInsert(const T& val) = 0;
  virtual T predict() = 0;
  virtual T calculateDelta() { return 0; }

  T m_pred;
  T m_delta;
  bool m_bFirst;
  std::vector<R> m_vErrors;
  // Debugging
  std::vector<T> m_vHistory;
  std::vector<T> m_vPredictions;
  };

} // rto
} // rtp_plus_plus
