#pragma once
#include <cstdint>
#include <sstream>
#include <vector>
#include <boost/random.hpp>

namespace rtp_plus_plus
{
namespace rfc2326 {

/**
 * @brief The SessionGenerator class: generates unique session
 */
class SessionGenerator
{
public:
  SessionGenerator()
  {
    m_rng.seed((uint32_t)std::time(0));
  }

  std::string generateSession()
  {
    boost::uint64_t uiSession = generateNumber();

    while (binary_search (m_vSessions.begin(), m_vSessions.end(), uiSession))
    {
      uiSession = generateNumber();
    }

    m_vSessions.push_back(uiSession);
    std::sort(m_vSessions.begin(), m_vSessions.end());

    std::ostringstream ostr;
    ostr << generateNumber();
    return ostr.str();
  }

private:
  uint64_t generateNumber()
  {
    uint64_t uiSession;
    unsigned uiRandom = m_rng();
    uiSession = ((uint64_t)uiRandom) << 32;
    uiRandom = m_rng();
    uiSession |=  uiRandom;
    return uiSession;
  }

  boost::mt19937 m_rng;
  std::vector<uint64_t> m_vSessions;
};

}
}
