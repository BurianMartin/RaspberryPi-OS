#include <context.hpp>

void irq_handle()
{
}

void context_swap(context &ctx_save, context &ctx_load)
{
    context_get(ctx_save);
    context_set(ctx_load);
}

extern "C" void fill_new_context(context *ctx, void (*entry)())
{
    ctx->r0 = 0;
    ctx->r1 = 0;
    ctx->r2 = 0;
    ctx->r3 = 0;
    ctx->r4 = 0;
    ctx->r5 = 0;
    ctx->r6 = 0;
    ctx->r7 = 0;
    ctx->r8 = 0;
    ctx->r9 = 0;
    ctx->r10 = 0;
    ctx->r11 = 0;
    ctx->r12 = 0;
    ctx->lr = 0;
    ctx->pc = reinterpret_cast<uint32_t>(entry);
    ctx->CPSR = 0x1F; // System mode, IRQ unmasked, ARM state
}
