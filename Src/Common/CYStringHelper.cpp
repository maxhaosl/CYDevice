#include "Common/CYStringHelper.hpp"
#include "Capture/Win/ReSampleRateDefine.hpp"

CYDEVICE_NAMESPACE_BEGIN

void GetTokenList(std::vector<std::wstring>& strList, const wchar_t* lpString, char token, bool bIncludeEmpty)
{
    const wchar_t* lpTemp = lpString;

    do
    {
        wchar_t* lpNextSeperator = wstrstr(lpTemp, token);

        if (lpNextSeperator)
            *lpNextSeperator = 0;

        if (*lpTemp || bIncludeEmpty)
        {
            strList.push_back(lpTemp);
        }

        if (lpNextSeperator)
            *lpNextSeperator = token;
    } while ((lpTemp = wstrstr(lpTemp, token) + 1) != (wchar_t*)sizeof(wchar_t));
}

std::wstring CharToWchar(const std::string& str)
{
    if (str.empty()) return L"";

    size_t len = std::mbstowcs(nullptr, str.c_str(), 0);
    if (len == static_cast<size_t>(-1)) return L"";

    std::wstring wstr(len, L'\0');
    std::mbstowcs(&wstr[0], str.c_str(), len);
    return wstr;
}

CYDEVICE_NAMESPACE_END