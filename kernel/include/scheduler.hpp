#pragma once

#include <cstdint>
#include <uart.hpp>
#include <utils.hpp>
#include <context.hpp>
#include <syscalls.hpp>
#include <page_manager.hpp>
#include <task_mem_manager.hpp>

enum class SchedulerPolicy : uint8_t
{
    ROUND_ROBIN,
    PRIORITY
};

struct Task
{
    VFP vpf;
    context ctx;
    bool done = false;

    TaskMemManager mem_mgr;
    const char *name;

    Task() : mem_mgr(0, 0, nullptr) {};
    Task(const char *title, uint32_t stack_addr, uint32_t heap_addr, PageManager *page_manager)
        : mem_mgr(stack_addr, heap_addr, page_manager), name(title) { ctx.sp = stack_addr; }
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

    PageManager *mm_;

    context *kernel_end_context = nullptr;

    bool kernel_end = false;

    void Execute(Task &task);

public:
    Scheduler(PageManager &mm, context &kernel_end_context);

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

extern Scheduler scheduler;