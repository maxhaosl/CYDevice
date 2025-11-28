#ifndef __CY_STRING_HELPER_HPP__
#define __CY_STRING_HELPER_HPP__

#include "CYDevice/CYDeviceDefine.hpp"

#include <vector>
#include <string>
#include <xstring>

CYDEVICE_NAMESPACE_BEGIN

std::wstring CharToWchar(const std::string& str);
void GetTokenList(std::vector<std::wstring>& strList, const wchar_t* lpString, char token, bool bIncludeEmpty);

CYDEVICE_NAMESPACE_END

#endif // __CY_STRING_HELPER_HPP__