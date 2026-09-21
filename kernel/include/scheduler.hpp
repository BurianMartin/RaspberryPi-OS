#pragma once

#include <cstdint>
#include <uart.hpp>
#include <utils.hpp>
#include <context.hpp>
#include <syscalls.hpp>
#include <memorymanager.hpp>

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

void task_finished();

class Scheduler
{
private:
    SchedulerPolicy policy_ = SchedulerPolicy::ROUND_ROBIN;
    Task tasks_[SCHEDULER_MAX_TASKS];
    uint32_t task_count_ = 0;

    uint32_t head_ = 0;
    uint32_t tail_ = 0;

    MemoryManager *mm_;

    context *kernel_end_context = nullptr;

    bool kernel_end = false;

    void Execute(Task &task);

public:
    Scheduler(MemoryManager &mm, context &kernel_end_context);

    Scheduler() = default;
    ~Scheduler() = default;

    void SetPolicy(SchedulerPolicy policy);

    bool AddTask(Task p);

    bool CreateAndAddTask(void (*entry)());

    bool SetCurrentTask(Task *task);

    void Run();

    void DeleteCurrentTask();

    void FreeTask(Task &task);

    void FreeCurrentTask();
};