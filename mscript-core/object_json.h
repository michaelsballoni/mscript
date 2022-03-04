#pragma once

#include "object.h"

namespace mscript
{
	object objectFromJson(const std::wstring& json);
	std::wstring objectToJson(const object& obj);
}
