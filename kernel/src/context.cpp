#include <context.hpp>

void irq_handle()
{
}

void context_swap(context &ctx_save, context &ctx_load)
{
    context_get(ctx_save);
    context_set(ctx_load);
}
