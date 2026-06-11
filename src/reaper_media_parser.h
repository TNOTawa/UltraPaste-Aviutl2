#pragma once
#include "plugin.h"
#include <vector>
#include <cstdint>

bool parse_reaper_media(const std::vector<uint8_t>& data, ReaperProject& proj);
