#pragma once

#include <Device.hpp>

bool hasEnum(uint enum_, const TuxClocker::Device::EnumerationVec &enumVec);

bool hasReadableValue(TuxClocker::Device::ReadResult res);

std::optional<std::string> fileContents(const std::string &path);
