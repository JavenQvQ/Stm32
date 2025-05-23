/*
*********************************************************************************************************
*                                                uC/OS-III
*                                          The Real-Time Kernel
*
*
*                         (c) Copyright 2009-2013; Micrium, Inc.; Weston, FL
*                    All rights reserved.  Protected by international copyright laws.
*
*                                           ARM Cortex-M4 Port
*
* File      : OS_CPU_C.C
* Version   : V3.03.02
* By        : JJL
*             BAN
*             JBL
*
* LICENSING TERMS:
* ---------------
*           uC/OS-III is provided in source form for FREE short-term evaluation, for educational use or 
*           for peaceful research.  If you plan or intend to use uC/OS-III in a commercial application/
*           product then, you need to contact Micrium to properly license uC/OS-III for its use in your 
*           application/product.   We provide ALL the source code for your convenience and to help you 
*           experience uC/OS-III.  The fact that the source is provided does NOT mean that you can use 
*           it commercially without paying a licensing fee.
*
*           Knowledge of the source code may NOT be used to develop a similar product.
*
*           Please help us continue to provide the embedded community with the finest software available.
*           Your honesty is greatly appreciated.
*
*           You can contact us at www.micrium.com, or by phone at +1 (954) 217-2036.
*
* For       : ARMv7 Cortex-M4
* Mode      : Thumb-2 ISA
* Toolchain : RealView
*********************************************************************************************************
*/

#define   OS_CPU_GLOBALS
#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const  CPU_CHAR  *os_cpu_c__c = "$Id: $";
#endif


/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  <os.h>
#include "includes.h"   	//����ͷ�ļ�   

#ifdef __cplusplus
extern  "C" {
#endif


/*
*********************************************************************************************************
*                                           IDLE TASK HOOK
*
* Description: This function is called by the idle task.  This hook has been added to allow you to do
*              such things as STOP the CPU to conserve power.
*
* Arguments  : None.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSIdleTaskHook (void)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppIdleTaskHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppIdleTaskHookPtr)();
    }
#endif
}


/*
*********************************************************************************************************
*                                       OS INITIALIZATION HOOK
*
* Description: This function is called by OSInit() at the beginning of OSInit().
*
* Arguments  : None.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSInitHook (void)
{
                                                                    /* 8-byte align the ISR stack.                            */    
    OS_CPU_ExceptStkBase = (CPU_STK *)(OSCfg_ISRStkBasePtr + OSCfg_ISRStkSize);
    OS_CPU_ExceptStkBase = (CPU_STK *)((CPU_STK)(OS_CPU_ExceptStkBase) & 0xFFFFFFF8);
}


/*
*********************************************************************************************************
*                                         STATISTIC TASK HOOK
*
* Description: This function is called every second by uC/OS-III's statistics task.  This allows your
*              application to add functionality to the statistics task.
*
* Arguments  : None.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSStatTaskHook (void)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppStatTaskHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppStatTaskHookPtr)();
    }
#endif
}


/*
*********************************************************************************************************
*                                          TASK CREATION HOOK
*
* Description: This function is called when a task is created.
*
* Arguments  : p_tcb        Pointer to the task control block of the task being created.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSTaskCreateHook (OS_TCB  *p_tcb)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskCreateHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskCreateHookPtr)(p_tcb);
    }
#else
    (void)p_tcb;                                            /* Prevent compiler warning                               */
#endif
}


/*
*********************************************************************************************************
*                                           TASK DELETION HOOK
*
* Description: This function is called when a task is deleted.
*
* Arguments  : p_tcb        Pointer to the task control block of the task being deleted.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSTaskDelHook (OS_TCB  *p_tcb)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskDelHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskDelHookPtr)(p_tcb);
    }
#else
    (void)p_tcb;                                            /* Prevent compiler warning                               */
#endif
}


/*
*********************************************************************************************************
*                                            TASK RETURN HOOK
*
* Description: This function is called if a task accidentally returns.  In other words, a task should
*              either be an infinite loop or delete itself when done.
*
* Arguments  : p_tcb        Pointer to the task control block of the task that is returning.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSTaskReturnHook (OS_TCB  *p_tcb)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskReturnHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskReturnHookPtr)(p_tcb);
    }
#else
    (void)p_tcb;                                            /* Prevent compiler warning                               */
#endif
}


/*
**********************************************************************************************************
*                                       INITIALIZE A TASK'S STACK
*
* Description: This function is called by OS_Task_Create() or OSTaskCreateExt() to initialize the stack
*              frame of the task being created. This function is highly processor specific.
*
* Arguments  : p_task       Pointer to the task entry point address.
*
*              p_arg        Pointer to a user supplied data area that will be passed to the task
*                               when the task first executes.
*
*              p_stk_base   Pointer to the base address of the stack.
*
*              stk_size     Size of the stack, in number of CPU_STK elements.
*
*              opt          Options used to alter the behavior of OS_Task_StkInit().
*                            (see OS.H for OS_TASK_OPT_xxx).
*
* Returns    : Always returns the location of the new top-of-stack' once the processor registers have
*              been placed on the stack in the proper order.
*
* Note(s)    : 1) Interrupts are enabled when task starts executing.
*
*              2) All tasks run in Thread mode, using process stack.
**********************************************************************************************************
*/

// CPU_STK  *OSTaskStkInit (OS_TASK_PTR    p_task,
//                          void          *p_arg,
//                          CPU_STK       *p_stk_base,
//                          CPU_STK       *p_stk_limit,
//                          CPU_STK_SIZE   stk_size,
//                          OS_OPT         opt)
// {
//     CPU_STK    *p_stk;


//     (void)opt;                                                  /* Prevent compiler warning                               */

//     p_stk = &p_stk_base[stk_size];                              /* Load stack pointer                                     */
//                                                                 /* Align the stack to 8-bytes.                            */
//     p_stk = (CPU_STK *)((CPU_STK)(p_stk) & 0xFFFFFFF8);

// #if (__FPU_PRESENT==1)&&(__FPU_USED==1)     	/* Registers stacked as if auto-saved on exception        */
// 	*(--p_stk) = (CPU_STK)0x00000000u; //No Name Register  
// 	*(--p_stk) = (CPU_STK)0x00001000u; //FPSCR
// 	*(--p_stk) = (CPU_STK)0x00000015u; //s15
// 	*(--p_stk) = (CPU_STK)0x00000014u; //s14
// 	*(--p_stk) = (CPU_STK)0x00000013u; //s13
// 	*(--p_stk) = (CPU_STK)0x00000012u; //s12
// 	*(--p_stk) = (CPU_STK)0x00000011u; //s11
// 	*(--p_stk) = (CPU_STK)0x00000010u; //s10
// 	*(--p_stk) = (CPU_STK)0x00000009u; //s9
// 	*(--p_stk) = (CPU_STK)0x00000008u; //s8
// 	*(--p_stk) = (CPU_STK)0x00000007u; //s7
// 	*(--p_stk) = (CPU_STK)0x00000006u; //s6
// 	*(--p_stk) = (CPU_STK)0x00000005u; //s5
// 	*(--p_stk) = (CPU_STK)0x00000004u; //s4
// 	*(--p_stk) = (CPU_STK)0x00000003u; //s3
// 	*(--p_stk) = (CPU_STK)0x00000002u; //s2
// 	*(--p_stk) = (CPU_STK)0x00000001u; //s1
// 	*(--p_stk) = (CPU_STK)0x00000000u; //s0
// #endif
	
// 	*(--p_stk) = (CPU_STK)0x01000000u;                            /* xPSR                                                   */
//     *(--p_stk) = (CPU_STK)p_task;                                 /* Entry Point                                            */
//     *(--p_stk) = (CPU_STK)OS_TaskReturn;                          /* R14 (LR)                                               */
//     *(--p_stk) = (CPU_STK)0x12121212u;                            /* R12                                                    */
//     *(--p_stk) = (CPU_STK)0x03030303u;                            /* R3                                                     */
//     *(--p_stk) = (CPU_STK)0x02020202u;                            /* R2                                                     */
//     *(--p_stk) = (CPU_STK)p_stk_limit;                            /* R1                                                     */
//     *(--p_stk) = (CPU_STK)p_arg;                                  /* R0 : argument                                          */

// #if (__FPU_PRESENT==1)&&(__FPU_USED==1)
// 	*(--p_stk) = (CPU_STK)0x00000031u; //s31
// 	*(--p_stk) = (CPU_STK)0x00000030u; //s30
// 	*(--p_stk) = (CPU_STK)0x00000029u; //s29
// 	*(--p_stk) = (CPU_STK)0x00000028u; //s28
// 	*(--p_stk) = (CPU_STK)0x00000027u; //s27
// 	*(--p_stk) = (CPU_STK)0x00000026u; //s26	
// 	*(--p_stk) = (CPU_STK)0x00000025u; //s25
// 	*(--p_stk) = (CPU_STK)0x00000024u; //s24
// 	*(--p_stk) = (CPU_STK)0x00000023u; //s23
// 	*(--p_stk) = (CPU_STK)0x00000022u; //s22
// 	*(--p_stk) = (CPU_STK)0x00000021u; //s21
// 	*(--p_stk) = (CPU_STK)0x00000020u; //s20
// 	*(--p_stk) = (CPU_STK)0x00000019u; //s19
// 	*(--p_stk) = (CPU_STK)0x00000018u; //s18
// 	*(--p_stk) = (CPU_STK)0x00000017u; //s17
// 	*(--p_stk) = (CPU_STK)0x00000016u; //s16
// #endif
//                                                                 /* Remaining registers saved on process stack             */
//     *(--p_stk) = (CPU_STK)0x11111111u;                            /* R11                                                    */
//     *(--p_stk) = (CPU_STK)0x10101010u;                            /* R10                                                    */
//     *(--p_stk) = (CPU_STK)0x09090909u;                            /* R9                                                     */
//     *(--p_stk) = (CPU_STK)0x08080808u;                            /* R8                                                     */
//     *(--p_stk) = (CPU_STK)0x07070707u;                            /* R7                                                     */
//     *(--p_stk) = (CPU_STK)0x06060606u;                            /* R6                                                     */
//     *(--p_stk) = (CPU_STK)0x05050505u;                            /* R5                                                     */
//     *(--p_stk) = (CPU_STK)0x04040404u;                            /* R4                                                     */

//     return (p_stk);
// }

CPU_STK  *OSTaskStkInit (OS_TASK_PTR    p_task,
                         void          *p_arg,
                         CPU_STK       *p_stk_base,
                         CPU_STK       *p_stk_limit,
                         CPU_STK_SIZE   stk_size,
                         OS_OPT         opt)
{
    CPU_STK    *p_stk;

    (void)opt;                                                  /* Prevent compiler warning                               */

    p_stk = &p_stk_base[stk_size];                              /* Load stack pointer                                     */
                                                                /* Align the stack to 8-bytes.                            */
    p_stk = (CPU_STK *)((CPU_STK)(p_stk) & 0xFFFFFFF8);
                                                                    /* Registers stacked as if auto-saved on exception        */
    *--p_stk = (CPU_STK)0x01000000u;                            /* xPSR                                                   */
    *--p_stk = (CPU_STK)p_task;                                 /* Entry Point                                            */
    *--p_stk = (CPU_STK)OS_TaskReturn;                          /* R14 (LR)                                               */
    *--p_stk = (CPU_STK)0x12121212u;                            /* R12                                                    */
    *--p_stk = (CPU_STK)0x03030303u;                            /* R3                                                     */
    *--p_stk = (CPU_STK)0x02020202u;                            /* R2                                                     */
    *--p_stk = (CPU_STK)p_stk_limit;                            /* R1                                                     */
    *--p_stk = (CPU_STK)p_arg;                                  /* R0 : argument                                          */
                                                                                                                                                                                                /* Remaining registers saved on process stack             */
    *--p_stk = (CPU_STK)0x11111111u;                            /* R11                                                    */
    *--p_stk = (CPU_STK)0x10101010u;                            /* R10                                                    */
    *--p_stk = (CPU_STK)0x09090909u;                            /* R9                                                     */
    *--p_stk = (CPU_STK)0x08080808u;                            /* R8                                                     */
    *--p_stk = (CPU_STK)0x07070707u;                            /* R7                                                     */
    *--p_stk = (CPU_STK)0x06060606u;                            /* R6                                                     */
    *--p_stk = (CPU_STK)0x05050505u;                            /* R5                                                     */
    *--p_stk = (CPU_STK)0x04040404u;                            /* R4                                                     */

        *--p_stk = (CPU_STK)0xFFFFFFFDUL;

    return (p_stk);
}
/*
*********************************************************************************************************
*                                           TASK SWITCH HOOK
*
* Description: This function is called when a task switch is performed.  This allows you to perform other
*              operations during a context switch.
*
* Arguments  : None.
*
* Note(s)    : 1) Interrupts are disabled during this call.
*              2) It is assumed that the global pointer 'OSTCBHighRdyPtr' points to the TCB of the task
*                 that will be 'switched in' (i.e. the highest priority task) and, 'OSTCBCurPtr' points
*                 to the task being switched out (i.e. the preempted task).
*********************************************************************************************************
*/

void  OSTaskSwHook (void)
{
#if OS_CFG_TASK_PROFILE_EN > 0u
    CPU_TS  ts;
#endif
#ifdef  CPU_CFG_INT_DIS_MEAS_EN
    CPU_TS  int_dis_time;
#endif


#if (OS_CPU_ARM_FP_EN == DEF_ENABLED)
//    if ((OSTCBCurPtr->Opt & OS_OPT_TASK_SAVE_FP) != (OS_OPT)0) {
//        OS_CPU_FP_Reg_Push(OSTCBCurPtr->StkPtr);
//    }

//    if ((OSTCBHighRdyPtr->Opt & OS_OPT_TASK_SAVE_FP) != (OS_OPT)0) {
//        OS_CPU_FP_Reg_Pop(OSTCBHighRdyPtr->StkPtr);
//    }
#endif

#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskSwHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppTaskSwHookPtr)();
    }
#endif

#if (defined(TRACE_CFG_EN) && (TRACE_CFG_EN > 0u))
    TRACE_OS_TASK_SWITCHED_IN(OSTCBHighRdyPtr);             /* Record the event.                                      */
#endif

#if OS_CFG_TASK_PROFILE_EN > 0u
    ts = OS_TS_GET();
    if (OSTCBCurPtr != OSTCBHighRdyPtr) {
        OSTCBCurPtr->CyclesDelta  = ts - OSTCBCurPtr->CyclesStart;
        OSTCBCurPtr->CyclesTotal += (OS_CYCLES)OSTCBCurPtr->CyclesDelta;
    }

    OSTCBHighRdyPtr->CyclesStart = ts;
#endif

#ifdef  CPU_CFG_INT_DIS_MEAS_EN
    int_dis_time = CPU_IntDisMeasMaxCurReset();                 /* Keep track of per-task interrupt disable time          */
    if (OSTCBCurPtr->IntDisTimeMax < int_dis_time) {
        OSTCBCurPtr->IntDisTimeMax = int_dis_time;
    }
#endif

#if OS_CFG_SCHED_LOCK_TIME_MEAS_EN > 0u
                                                                /* Keep track of per-task scheduler lock time             */
    if (OSTCBCurPtr->SchedLockTimeMax < OSSchedLockTimeMaxCur) {
        OSTCBCurPtr->SchedLockTimeMax = OSSchedLockTimeMaxCur;
    }
    OSSchedLockTimeMaxCur = (CPU_TS)0;                          /* Reset the per-task value                               */
#endif
}


/*
*********************************************************************************************************
*                                              TICK HOOK
*
* Description: This function is called every tick.
*
* Arguments  : None.
*
* Note(s)    : 1) This function is assumed to be called from the Tick ISR.
*********************************************************************************************************
*/

void  OSTimeTickHook (void)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTimeTickHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppTimeTickHookPtr)();
    }
#endif
}


/*
*********************************************************************************************************
*                                          SYS TICK HANDLER
*
* Description: Handle the system tick (SysTick) interrupt, which is used to generate the uC/OS-II tick
*              interrupt.
*
* Arguments  : None.
*
* Note(s)    : 1) This function MUST be placed on entry 15 of the Cortex-M3 vector table.
*********************************************************************************************************
*/

void  OS_CPU_SysTickHandler (void)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    OSIntNestingCtr++;                                      /* Tell uC/OS-III that we are starting an ISR             */
    CPU_CRITICAL_EXIT();

    OSTimeTick();                                           /* Call uC/OS-III's OSTimeTick()                          */

    OSIntExit();                                            /* Tell uC/OS-III that we are leaving the ISR             */
}


/*
*********************************************************************************************************
*                                         INITIALIZE SYS TICK
*
* Description: Initialize the SysTick.
*
* Arguments  : cnts         Number of SysTick counts between two OS tick interrupts.
*
* Note(s)    : 1) This function MUST be called after OSStart() & after processor initialization.
*********************************************************************************************************
*/

void  OS_CPU_SysTickInit (CPU_INT32U  cnts)
{
    CPU_INT32U  prio;


    CPU_REG_NVIC_ST_RELOAD = cnts - 1u;

                                                            /* Set SysTick handler prio.                              */
    prio                   = CPU_REG_NVIC_SHPRI3;
    prio                  &= DEF_BIT_FIELD(24, 0);
    prio                  |= DEF_BIT_MASK(OS_CPU_CFG_SYSTICK_PRIO, 24);

    CPU_REG_NVIC_SHPRI3    = prio;

                                                            /* Enable timer.                                          */
    CPU_REG_NVIC_ST_CTRL  |= CPU_REG_NVIC_ST_CTRL_CLKSOURCE |
                             CPU_REG_NVIC_ST_CTRL_ENABLE;
                                                            /* Enable timer interrupt.                                */
    CPU_REG_NVIC_ST_CTRL  |= CPU_REG_NVIC_ST_CTRL_TICKINT;
}

#ifdef __cplusplus
}
#endif
