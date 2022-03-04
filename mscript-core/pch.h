#pragma once

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _CRT_SECURE_NO_WARNINGS

#pragma warning(disable: 26451)
#include "json.hpp"
#pragma warning(default: 26451)

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
