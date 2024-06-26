// Typical PCH stuff here, and pch.h includes this
// The reason this isn't in pch.h directly is for other projects 
// to include this in their PCH's
#pragma once

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _CRT_SECURE_NO_WARNINGS

#define _USE_MATH_DEFINES

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h> // HMODULE
#endif

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <codecvt>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <locale>
#include <memory>
#include <optional>
#include <regex>
#include <stdexcept>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
