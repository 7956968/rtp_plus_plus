#pragma once
#include <rtp++/rto/PredictorBase.h>
#include <boost/circular_buffer.hpp>

#define AR2_HISTORY_SIZE 100

namespace rtp_plus_plus
{
namespace rto
{ 

template <typename T, typename R>
class AR2Predictor : public PredictorBase<T, R>
{
public:
    AR2Predictor()
        :m_queue(AR2_HISTORY_SIZE),
        m_uiN(0)
    {

    }

    ~AR2Predictor()
    {
    }

    void reset()
    {
      m_uiN = 0;
      m_queue.clear();
    }

protected:
    virtual bool isReady() const
    {
      return m_uiN >= AR2_HISTORY_SIZE;
    }

    virtual void doInsert(const T& val)
    {
      ++m_uiN;
      m_queue.push_back(val);

      if (m_uiN > 2)
      {
      if (m_uiN == 3)
      {
        r_0 = ( m_queue[2] * m_queue[2] + m_queue[1] * m_queue[1] + m_queue[0] * m_queue[0] ) / 3.0;
        r_1 = ( m_queue[2] * m_queue[1] + m_queue[1] * m_queue[0] ) / 2.0;
        r_2 = ( m_queue[2] * m_queue[0] );
      }
      else
      {
        r_0 = ( previous_r_0 * (m_uiN-1) + (m_queue[2] * m_queue[2]) ) / ( m_uiN );
        r_1 = ( previous_r_1 * (m_uiN-2) + (m_queue[2] * m_queue[1]) ) / ( m_uiN - 1);
        r_2 = ( previous_r_2 * (m_uiN-3) + (m_queue[2] * m_queue[0]) ) / ( m_uiN - 2);
      }
      previous_r_0 = r_0;
      previous_r_1 = r_1;
      previous_r_2 = r_2;

      m_alpha_2_2 = ( r_1 * r_1 - r_0 * r_2 ) / (r_1 * r_1 - r_0 * r_0);
      m_alpha_1_2 = ( r_1 * r_0 - r_2 * r_1 ) / (r_0 * r_0 - r_1 * r_1 );

      }
    }

    virtual T predict()
    {
//      double r_0;
//      double r_1;
//      double r_2;
//      if (m_uiN == AR2_HISTORY_SIZE)
//      {
//        r_0 = ( m_queue[2] * m_queue[2] + m_queue[1] * m_queue[1] + m_queue[0] * m_queue[0] ) / 3.0;
//        r_1 = ( m_queue[2] * m_queue[1] + m_queue[1] * m_queue[0] ) / 2.0;
//        r_2 = ( m_queue[2] * m_queue[0] );
//      }
//      else
//      {
//        r_0 = ( previous_r_0 * (m_uiN-1) + (m_queue[2] * m_queue[2]) ) / ( m_uiN );
//        r_1 = ( previous_r_1 * (m_uiN-2) + (m_queue[2] * m_queue[1]) ) / ( m_uiN - 1);
//        r_2 = ( previous_r_2 * (m_uiN-3) + (m_queue[2] * m_queue[0]) ) / ( m_uiN - 2);
//      }
//      previous_r_0 = r_0;
//      previous_r_1 = r_1;
//      previous_r_2 = r_2;

#if 0
      double alpha_1_1 = r_1 / r_0;
      double p_1       = ( 1 - ( alpha_1_1 * alpha_1_1) ) * r_0;
      double alpha_2_2 = ( r_2 - ( alpha_1_1 * r_1 ))/p_1;
      double alpha_1_2 = alpha_1_1 * (1 - alpha_2_2);
      double pred = (alpha_1_2 * m_queue[2]) + (alpha_2_2 * m_queue[1]);
      VLOG(15) << "Pred: " << pred << " " << r_0 << " " << r_1 << " " << r_2 << " " << p_1;
#else
//      double alpha_2_2 = ( r_1 * r_1 - r_0 * r_2 ) / (r_1 * r_1 - r_0 * r_0);
//      double alpha_1_2 = ( r_1 * r_0 - r_2 * r_1 ) / (r_0 * r_0 - r_1 * r_1 );

      double pred = (m_alpha_1_2 * m_queue[2]) + (m_alpha_2_2 * m_queue[1]);
      VLOG(15) << "Pred: " << pred << " " << r_0 << " " << r_1 << " " << r_2 << " " << m_alpha_1_2 << " " << m_alpha_2_2;
#endif
      T ret = static_cast<T>(pred);
      // HACK: need to determine this dynamically somehow
//      if (ret < 35000 || ret > 45000)
//      {
//          VLOG(2) << "***: Pred: " << pred << " " << r_0 << " " << r_1 << " " << r_2/* << " " << p_1*/;
//          // HACK FOR NOW: sometimes the prediction values get screwed
//          return PredictorBase<T, R>::m_pred;
//      }

      return ret;
    }

private:

    boost::circular_buffer<T> m_queue;
    double previous_r_0;
    double previous_r_1;
    double previous_r_2;
    uint64_t m_uiN;

    double r_0;
    double r_1;
    double r_2;
    double m_alpha_1_2;
    double m_alpha_2_2;

};

} // rto
} // rtp_plus_plus
