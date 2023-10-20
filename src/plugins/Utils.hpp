#pragma once

#include <Device.hpp>

bool hasReadableValue(TuxClocker::Device::ReadResult res);

std::optional<std::string> fileContents(const std::string &path);
