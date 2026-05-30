#include "scheduler.h"
#include "pmm.h"
#include "memory.h"
#include "stdio.h"
#include "arch/i686/gdt.h"

#define MAX_TASKS   16

static Task  g_Tasks[MAX_TASKS];
static int   g_TaskCount   = 0;
static Task* g_CurrentTask = NULL;
static bool  g_Enabled     = false;

void Scheduler_Initialize(void)
{
    memset(g_Tasks, 0, sizeof(g_Tasks));
    g_TaskCount   = 0;
    g_CurrentTask = NULL;
    g_Enabled     = false;
}

// =====================================================
// Make the currently running code (kernel main) into a task.
// Call this FIRST, before creating other tasks. regs is NULL until the
// first timer preemption, which fills it in with the actual stack frame.
// =====================================================
Task* Scheduler_CreateKernelTask(void)
{
    Task* task      = &g_Tasks[g_TaskCount++];
    task->stackBase = NULL;     // kernel-main already has a stack
    task->regs      = NULL;     // filled on first timer switch
    task->next      = task;     // self-loop; OnTimer treats "next==self" as "only one task"
    g_CurrentTask   = task;
    return task;
}

// =====================================================
// Create a new task with a freshly-allocated 4KB stack.
// Builds a fake interrupt frame so the scheduler's iret will jump into
// entry() with kernel segments and interrupts enabled.
// =====================================================
Task* Scheduler_CreateTask(TaskFunction entry)
{
    if (g_TaskCount >= MAX_TASKS) return NULL;

    Task* task = &g_Tasks[g_TaskCount++];

    task->stackBase = PMM_AllocateBlock();
    if (task->stackBase == NULL)
    {
        printf("Scheduler: failed to allocate stack for task\r\n");
        g_TaskCount--;
        return NULL;
    }

    uint32_t stackTop = (uint32_t)task->stackBase + PMM_BLOCK_SIZE;

    // Place the fake Registers frame at the top of the stack. When
    // isr_common does mov esp = this, then pop ds + popa + iret, the
    // iret pops eip/cs/eflags and jumps into entry() with the kernel
    // data selector in ds/ss and IF=1.
    Registers* regs = (Registers*)(stackTop - sizeof(Registers));
    memset(regs, 0, sizeof(Registers));

    regs->eip    = (uint32_t)entry;
    regs->cs     = GDT_CODE_SEGMENT;
    regs->eflags = 0x202;              // IF=1 (interrupts enabled) + reserved bit 1
    regs->ds     = GDT_DATA_SEGMENT;
    regs->ss     = GDT_DATA_SEGMENT;
    regs->esp    = stackTop;           // informational; same-priv iret ignores it

    task->regs = regs;
    task->next = NULL;

    return task;
}

// =====================================================
// Link all created tasks into a circular list and enable scheduling.
// =====================================================
void Scheduler_Start(void)
{
    if (g_TaskCount <= 1) return;       // need at least 2 tasks to switch

    for (int i = 0; i < g_TaskCount; i++)
        g_Tasks[i].next = &g_Tasks[(i + 1) % g_TaskCount];

    g_Enabled = true;
    printf("Scheduler: started with %d tasks\r\n", g_TaskCount);
}

// =====================================================
// Called from the PIT handler on every tick. Saves the interrupted
// task's stack, advances to the next task, and tells i686_ISR_Handler
// to resume that one instead.
// =====================================================
void Scheduler_OnTimer(Registers* regs)
{
    if (!g_Enabled || g_CurrentTask == NULL) return;
    if (g_CurrentTask->next == g_CurrentTask) return;   // only one task

    g_CurrentTask->regs = regs;
    g_CurrentTask       = g_CurrentTask->next;
    g_SchedulerNextTask = g_CurrentTask->regs;
}

// =====================================================
// Stop scheduling and free every task except the kernel task (index 0).
// Called from the kernel task itself (i.e., shell command context), so
// once g_Enabled flips false there are no more switches: the kernel
// task simply continues executing without preemption.
// =====================================================
void Scheduler_StopAndReset(void)
{
    g_Enabled = false;     // first: no more switches even if PIT fires mid-reset

    for (int i = 1; i < g_TaskCount; i++)
    {
        if (g_Tasks[i].stackBase != NULL)
            PMM_FreeBlock(g_Tasks[i].stackBase);
        g_Tasks[i].stackBase = NULL;
        g_Tasks[i].regs      = NULL;
        g_Tasks[i].next      = NULL;
    }

    g_TaskCount        = 1;
    g_Tasks[0].next    = &g_Tasks[0];   // back to self-loop
    g_CurrentTask      = &g_Tasks[0];
    g_SchedulerNextTask = NULL;
}
