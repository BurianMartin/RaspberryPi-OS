#pragma once

#include <utils.hpp>

class InterruptController
{
private:
    /* data */
public:
    InterruptController() = default;
    ~InterruptController() = default;

    static void EnableIRQs();
    static void DisableIRQs();
};