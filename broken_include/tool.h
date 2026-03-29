#pragma once
#include <string>
class tool
{
public:
    tool ()
    {}
    tool(std::string s1)
    :tmp(s1)
    {}

    std::string trim(const std::string& s);


    void to_lower(std::string& s);



public:
std::string tmp;
};