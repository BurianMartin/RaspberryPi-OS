#pragma once

#include <utils.hpp>

class InterruptController
{
private:
    /* data */
public:
    InterruptController() = default;
    ~InterruptController() = default;

    void EnableIRQs();
};