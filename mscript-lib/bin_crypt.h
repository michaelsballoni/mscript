#pragma once

#include <string>

namespace mscript
{
    struct bin_crypt_info
    {
        std::wstring publisher;
        std::wstring subject;
    };
    bin_crypt_info getBinCryptInfo(const std::wstring& filePath);
}
