#pragma once

#include <cstdint>
#include <uart.hpp>
#include <utils.hpp>
#include <context.hpp>
#include <syscalls.hpp>

enum class SchedulerPolicy : uint8_t
{
    ROUND_ROBIN,
    PRIORITY
};

struct Task
{
    VFP vpf;
    context ctx;
    const char *name;
    bool done = false;
};

extern Task *current_task;
extern context *current_context;

class Scheduler
{
private:
    SchedulerPolicy policy_ = SchedulerPolicy::ROUND_ROBIN;
    Task tasks_[SCHEDULER_MAX_TASKS];
    uint32_t task_count_ = 0;

    uint32_t head_ = 0;
    uint32_t tail_ = 0;

    void Execute(Task &task);

    int GetFreeStack();

public:
    Scheduler();
    ~Scheduler() = default;

    void SetPolicy(SchedulerPolicy policy);

    bool AddTask(Task p);

    bool CreateAndAddTask(void (*entry)());

    bool SetCurrentTask(Task *task);

    void Run();

    void DeleteCurrentTask();
};