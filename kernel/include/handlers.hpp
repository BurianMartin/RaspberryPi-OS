#pragma once

#include <uart.hpp>
#include <utils.hpp>
#include <scheduler.hpp>
#include <task_mem_manager.hpp>

extern "C" void svc_handler();

extern "C" void irq_handler();