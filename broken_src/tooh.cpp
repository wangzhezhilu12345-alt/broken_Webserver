#include <iostream>
#include <string>
#include <algorithm>
#include "../broken_include/tool.h"
std::string tool::trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" ");
    size_t end = s.find_last_not_of(" ");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

void tool::to_lower(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
}