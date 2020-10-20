#pragma once

#include <string>

std::string base64_encode(const std::string& s);
std::string base64_encode(unsigned char const* , unsigned int len);
std::string base64_decode(std::string const& s);
