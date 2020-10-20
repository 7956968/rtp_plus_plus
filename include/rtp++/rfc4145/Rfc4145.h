#pragma once
#include <rtp++/rfc4566/SessionDescription.h>

namespace rtp_plus_plus
{
namespace rfc4145
{

extern const std::string SETUP;
extern const std::string CONNECTION;

extern const std::string ACTIVE;
extern const std::string PASSIVE;
extern const std::string ACTPASS;
extern const std::string HOLDCONN;

extern const std::string NEW;
extern const std::string EXISTING;

bool isActive(const rfc4566::MediaDescription& media);
bool isPassive(const rfc4566::MediaDescription& media);
bool isActPass(const rfc4566::MediaDescription& media);
bool isHoldConn(const rfc4566::MediaDescription& media);

bool useNew(const rfc4566::MediaDescription& media);
bool useExisting(const rfc4566::MediaDescription& media);

}
}
