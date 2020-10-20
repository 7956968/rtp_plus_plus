#pragma once
#include <algorithm>
#include <string>
#include <vector>

namespace rtp_plus_plus
{ 
namespace rfc5888
{

extern const std::string GROUP;
extern const std::string MID;

class Group
{
public:
  Group()
  {

  }

  Group(const std::string& sSemantic, std::vector<std::string>& vMids )
    :m_sSemantic(sSemantic),
      m_vMids(vMids)
  {

  }

  std::string getSemantic() const { return m_sSemantic; }
  const std::vector<std::string>& getMids() const { return m_vMids; }

  bool contains(const std::string& sMid) const
  {
    return (std::find(m_vMids.begin(), m_vMids.end(), sMid) != m_vMids.end());
  }

private:

  std::string m_sSemantic;
  std::vector<std::string> m_vMids;
};

} // rfc5888
} // rtp_plus_plus
