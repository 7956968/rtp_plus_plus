#pragma once
#include <cmath>
#include "z.h"

namespace rtp_plus_plus
{
namespace rto
{

class NormalDistribution
{
public:
  static double getZScore(double dExpectation)
  {
    return std::abs(critz(dExpectation));
  }
};

} // rto
} // rtp_plus_plus
