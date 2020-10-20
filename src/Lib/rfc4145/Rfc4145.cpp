#include "CorePch.h"
#include <rtp++/rfc4145/Rfc4145.h>

namespace rtp_plus_plus
{
namespace rfc4145
{

const std::string SETUP       = "setup";
const std::string CONNECTION  = "connection";
const std::string ACTIVE      = "active";
const std::string PASSIVE     = "passive";
const std::string ACTPASS     = "actpass";
const std::string HOLDCONN    = "holdconn";
const std::string NEW         = "new";
const std::string EXISTING    = "existing";

bool isActive(const rfc4566::MediaDescription& media)
{
  if (media.hasAttribute(SETUP))
    return (*media.getAttributeValue(SETUP) == ACTIVE);
  return false;
}

bool isPassive(const rfc4566::MediaDescription& media)
{
  if (media.hasAttribute(SETUP))
    return (*media.getAttributeValue(SETUP) == PASSIVE);
  return false;
}

bool isActPass(const rfc4566::MediaDescription& media)
{
  if (media.hasAttribute(SETUP))
    return (*media.getAttributeValue(SETUP) == ACTPASS);
  return false;
}

bool isHoldConn(const rfc4566::MediaDescription& media)
{
  if (media.hasAttribute(SETUP))
    return (*media.getAttributeValue(SETUP) == HOLDCONN);
  return false;
}

bool useNew(const rfc4566::MediaDescription& media)
{
  if (media.hasAttribute(CONNECTION))
    return (*media.getAttributeValue(SETUP) == NEW);
  return false;
}

bool useExisting(const rfc4566::MediaDescription& media)
{
  if (media.hasAttribute(CONNECTION))
    return (*media.getAttributeValue(SETUP) == EXISTING);
  return false;
}

}
}
