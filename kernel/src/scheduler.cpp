#include <scheduler.hpp>

Task *current_task = nullptr;
context *current_context = nullptr;

void Scheduler::Execute(Task &task)
{
    context_set(task.ctx); // For now just set the normal context, no VPF (Need to add floating point suopport later)
}

Scheduler::Scheduler(MemoryManager &mm) : mm_(&mm)
{
}

void Scheduler::SetPolicy(SchedulerPolicy policy)
{
    policy_ = policy;
}

bool Scheduler::AddTask(Task p)
{
    if (task_count_ >= SCHEDULER_MAX_TASKS)
    {
        return false;
    }

    tasks_[tail_] = p;
    tail_ = (tail_ + 1) % SCHEDULER_MAX_TASKS;
    task_count_++;

    return true;
}

bool Scheduler::CreateAndAddTask(void (*entry)())
{
    uint32_t stack_address = 0;
    if (mm_)
    {
        stack_address = mm_->GetPageBaseAddress();
        if (stack_address == 0)
        {
            uart_puts("Failed to allocate stack memory\n\0");
            return false;
        }
    }

    Task task;

    task.ctx.sp = stack_address;
    fill_new_context(&task.ctx, entry);
    task.name = "Test Task";

    return AddTask(task);
}

void Scheduler::Run()
{
    if (!SetCurrentTask(&tasks_[head_]))
    {
        uart_puts("Failed to set current task\n\0");
        return;
    }

    head_ = (head_ + 1) % SCHEDULER_MAX_TASKS;
    task_count_--;

    Execute(*current_task);
}

void Scheduler::FreeTask(Task &task)
{
    if (mm_)
    {
        uint32_t stack_address = task.ctx.sp - task.ctx.sp % 4096;
        mm_->FreePage(stack_address);
    }
}

void Scheduler::FreeCurrentTask()
{
    if (current_task)
    {
        FreeTask(*current_task);
    }
}

bool Scheduler::SetCurrentTask(Task *task)
{
    if (task_count_ > 0)
    {
        current_task = task;
        current_context = &current_task->ctx;
        return true;
    }
    return false;
}
