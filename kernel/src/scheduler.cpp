#include <scheduler.hpp>

Task *current_task = nullptr;
context *current_context = nullptr;

uint8_t stacks[SCHEDULER_MAX_TASKS][TASK_STACK_SIZE]; // 8 * 4Kb

bool stack_free_[SCHEDULER_MAX_TASKS];

void Scheduler::Execute(Task &task)
{
    context_set(task.ctx); // For now just set the normal context, no VPF (Need to add floating point suopport later)
}

int Scheduler::GetFreeStack()
{
    for (size_t i = 0; i < SCHEDULER_MAX_TASKS; i++)
    {
        if (stack_free_[i])
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}

Scheduler::Scheduler()
{
    for (size_t i = 0; i < SCHEDULER_MAX_TASKS; i++)
    {
        stack_free_[i] = true;
        memset(stacks[i], 0, TASK_STACK_SIZE);
    }
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
    int st = GetFreeStack();
    if (st == -1)
    {
        return false;
    }

    Task task;
    task.ctx.sp = reinterpret_cast<uint32_t>(stacks[st]) + TASK_STACK_SIZE;
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
