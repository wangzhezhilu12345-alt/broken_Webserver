#pragma once
class nocopy
{
public:
    nocopy(const nocopy&) = delete;
    nocopy& operator=(const nocopy&) = delete;
protected:
    nocopy() = default;
    ~nocopy() = default;
};