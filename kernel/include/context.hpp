#pragma once

#include <cstdint>

constexpr uint32_t USER_REG_COUNT = 16;

struct context
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r12;
    uint32_t sp;
    uint32_t lr;
    uint32_t pc;
    uint32_t CPSR;
};

struct VFP
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r12;
    uint32_t FPSCR;
};

void irq_handle();

void context_swap(context &ctx_save, context &ctx_load);

extern "C" void context_get(context &ctx_reg);

extern "C" void context_set(context &ctx_reg);

extern "C" void fill_new_context(context *ctx, void (*entry)());
