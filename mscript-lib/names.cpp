#include "pch.h"
#include "names.h"
#include "utils.h"

void mscript::validateName(const std::wstring& name)
{
    if (isReserved(name))
        raiseWError(L"Name is reserved: " + name);

    if (!isName(name))
        raiseWError(L"Names must start with a letter and contain only letters, digits, or underscores: " + name);
}

bool mscript::isName(const std::wstring& name)
{
    if (name.empty())
        return false;

    if (!iswalpha(name[0]))
        return false;

    for (size_t c = 1; c < name.size(); ++c)
    {
        if (!iswalnum(name[c]) && name[c] != '_')
            return false;
    }

    return true;
}

bool mscript::isReserved(const std::wstring& name)
{
    static std::unordered_set<std::wstring> ReservedWords
    {
        L"null",
        L"true",
        L"false",
        L"pi",
        L"e",
        L"dquote",
        L"squote",
        L"tab",
        L"crlf",
        L"lf",
    };
    return ReservedWords.find(name) != ReservedWords.end();
}
