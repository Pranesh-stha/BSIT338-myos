#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "arch/i686/isr.h"
#include <stdbool.h>

typedef void (*TaskFunction)(void);

typedef struct Task
{
    Registers*   regs;        // saved register state (points into task's stack)
    void*        stackBase;   // PMM-allocated stack page (NULL for kernel-main)
    struct Task* next;        // circular linked list for round-robin
} Task;

void  Scheduler_Initialize(void);
Task* Scheduler_CreateKernelTask(void);             // makes current code into task 0
Task* Scheduler_CreateTask(TaskFunction entry);
void  Scheduler_Start(void);
void  Scheduler_StopAndReset(void);                 // disable + free non-kernel tasks
void  Scheduler_OnTimer(Registers* regs);           // called from PIT handler

// Set by Scheduler_OnTimer, read by i686_ISR_Handler. Defined in isr.c.
extern Registers* g_SchedulerNextTask;

#endif
