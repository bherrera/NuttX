/****************************************************************************
 * arch/arm/src/xmc4/xmc4_timerisr.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <time.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <arch/board/board.h>

#include "nvic.h"
#include "clock/clock.h"
#include "up_internal.h"
#include "up_arch.h"

#include "chip.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The SysTick counter runs on the clock selected by SYST_CSR.CLKSOURCE.
 * That selection may be either:
 *
 *    CLKSOURCE=0: fSTDBY / 2
 *    CLKSOURCE=1: fCPU
 *
 * In the first case, the SysTick counter would run at 16.384Khz.  The most
 * common system clock of 10 msec/tick cannot be exactly represented with
 * that value.
 *
 * In the second case, the SysTick counter may run too rapidly to support
 * longer timer tick intervals.  For example, if the CPU clock is 144Mhz,
 * then that 10 msec interval would correspond to a reload value of 1,440,000
 * or 0x0015f900.
 */

/* The desired timer interrupt frequency is provided by the definition
 * CLK_TCK (see include/time.h).  CLK_TCK defines the desired number of
 * system clock ticks per second.  That value is a user configurable setting
 * that defaults to 100 (100 ticks per second = 10 MS interval).
 *
 * Lets try fCPU first:
 */

#define SYSTICK_RELOAD ((BOARD_CPU_FREQUENCY / CLK_TCK) - 1)
#undef  USE_STDBY_CLOCK

/* Verify that the reload value will fit in the reload register. */

#if SYSTICK_RELOAD > 0x00ffffff
  /* No, then revert to fSTDBY */

#  undef SYSTICK_RELOAD
#  define SYSTICK_RELOAD ((BOARD_STDBY_FREQUENCY / CLK_TCK) - 1)
#  define USE_STDBY_CLOCK 1
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Function:  xmc4_timerisr
 *
 * Description:
 *   The timer ISR will perform a variety of services for various portions
 *   of the systems.
 *
 ****************************************************************************/

static int xmc4_timerisr(int irq, uint32_t *regs, FAR void *arg)
{
  /* Process timer interrupt */

  sched_process_timer();
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Function:  arm_timer_initialize
 *
 * Description:
 *   This function is called during start-up to initialize
 *   the timer interrupt.
 *
 ****************************************************************************/

void arm_timer_initialize(void)
{
  uint32_t regval;

  /* Set the SysTick interrupt to the default priority */

  regval  = getreg32(NVIC_SYSH12_15_PRIORITY);
  regval &= ~NVIC_SYSH_PRIORITY_PR15_MASK;
  regval |= (NVIC_SYSH_PRIORITY_DEFAULT << NVIC_SYSH_PRIORITY_PR15_SHIFT);
  putreg32(regval, NVIC_SYSH12_15_PRIORITY);

#ifndef USE_STDBY_CLOCK
  /* Note that is should not be neccesary to set the SYSTICK clock source:
   * "The CLKSOURCE bit in SysTick Control and Status register is always set
   *  to select the core clock."
   *
   * For the XMC4xx, fhat selection may be either:
   *
   *   CLKSOURCE=0: fSTDBY / 2
   *   CLKSOURCE=1: fCPU
   */

  regval  = getreg32(NVIC_SYSTICK_CTRL);
  regval |= NVIC_SYSTICK_CTRL_CLKSOURCE;
  putreg32(regval, NVIC_SYSTICK_CTRL);
#endif

  /* Configure SysTick to interrupt at the requested rate */

  putreg32(SYSTICK_RELOAD, NVIC_SYSTICK_RELOAD);

  /* Attach the timer interrupt vector */

  (void)irq_attach(XMC4_IRQ_SYSTICK, (xcpt_t)xmc4_timerisr, NULL);

  /* Enable SysTick interrupts */

  putreg32((NVIC_SYSTICK_CTRL_CLKSOURCE | NVIC_SYSTICK_CTRL_TICKINT |
            NVIC_SYSTICK_CTRL_ENABLE),
           NVIC_SYSTICK_CTRL);

  /* And enable the timer interrupt */

  up_enable_irq(XMC4_IRQ_SYSTICK);
}
