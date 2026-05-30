#include "stdint.h"
#include "stdio.h"
#include "memory.h"
#include "hal/hal.h"
#include "arch/i686/irq.h"
#include "arch/i686/keyboard.h"
#include "arch/i686/mouse.h"
#include "arch/i686/pit.h"
#include "boot/bootparams.h"
#include "pmm.h"
#include "shell.h"
#include "status_bar.h"
#include "scheduler.h"
#include "boot_logo.h"

extern uint8_t __bss_start;
extern uint8_t __end;

void __attribute__((section(".entry"))) start(BootParams* bootParams)
{
    memset(&__bss_start, 0, (&__end) - (&__bss_start));

    // Register IRQ handlers (and program the PIT) before HAL_Initialize STIs.
    i686_PIT_Initialize();
    i686_Keyboard_Initialize();
    i686_Mouse_Initialize();

    HAL_Initialize();

    clrscr();
    StatusBar_Initialize();
    PMM_Initialize(&bootParams->memory);

    // Scheduler ready but not enabled: this code (shell) becomes task 0,
    // no other tasks yet, so PIT_Handler -> Scheduler_OnTimer is a no-op
    // until 'multi' command creates more tasks and calls Scheduler_Start.
    Scheduler_Initialize();
    Scheduler_CreateKernelTask();

    BootLogo_Show();
    Shell_Run();
}
