/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/************************************************************************
 * kernel/sched/sched_lock.c
 *
 *   Copyright (C) 2007, 2009 Gregory Nutt. All rights reserved.
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
 ************************************************************************/

/************************************************************************
 * Included Files
 ************************************************************************/

#include <tinyara/config.h>

#include <sched.h>
#include <assert.h>

#include <tinyara/arch.h>
#include "sched/sched.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Pre-emption is disabled via the interface sched_lock(). sched_lock()
 * works by preventing context switches from the currently executing tasks.
 * This prevents other tasks from running (without disabling interrupts) and
 * gives the currently executing task exclusive access to the (single) CPU
 * resources. Thus, sched_lock() and its companion, sched_unlock(), are
 * used to implement some critical sections.
 *
 * In the single CPU case, pre-emption is disabled using a simple lockcount
 * in the TCB. When the scheduling is locked, the lockcount is incremented;
 * when the scheduler is unlocked, the lockcount is decremented. If the
 * lockcount for the task at the head of the g_readytorun list has a
 * lockcount > 0, then pre-emption is disabled.
 *
 * No special protection is required since only the executing task can
 * modify its lockcount.
 */

#ifdef CONFIG_SMP
/* In the multiple CPU, SMP case, disabling context switches will not give a
 * task exclusive access to the (multiple) CPU resources (at least without
 * stopping the other CPUs): Even though pre-emption is disabled, other
 * threads will still be executing on the other CPUS.
 *
 * There are additional rules for this multi-CPU case:
 *
 * 1. There is a global lock count 'g_cpu_lockset' that includes a bit for
 *    each CPU: If the bit is '1', then the corresponding CPU has the
 *    scheduler locked; if '0', then the CPU does not have the scheduler
 *    locked.
 * 2. Scheduling logic would set the bit associated with the cpu in
 *    'g_cpu_lockset' when the TCB at the head of the g_assignedtasks[cpu]
 *    list transitions has 'lockcount' > 0. This might happen when
 *    sched_lock() is called, or after a context switch that changes the
 *    TCB at the head of the g_assignedtasks[cpu] list.
 * 3. Similarly, the cpu bit in the global 'g_cpu_lockset' would be cleared
 *    when the TCB at the head of the g_assignedtasks[cpu] list has
 *    'lockcount' == 0. This might happen when sched_unlock() is called, or
 *    after a context switch that changes the TCB at the head of the
 *    g_assignedtasks[cpu] list.
 * 4. Modification of the global 'g_cpu_lockset' must be protected by a
 *    spinlock, 'g_cpu_schedlock'. That spinlock would be taken when
 *    sched_lock() is called, and released when sched_unlock() is called.
 *    This assures that the scheduler does enforce the critical section.
 *    NOTE: Because of this spinlock, there should never be more than one
 *    bit set in 'g_cpu_lockset'; attempts to set additional bits should
 *    cause the CPU to block on the spinlock.  However, additional bits
 *    could get set in 'g_cpu_lockset' due to the context switches on the
 *    various CPUs.
 * 5. Each time the head of a g_assignedtasks[] list changes and the
 *    scheduler modifies 'g_cpu_lockset', it must also set 'g_cpu_schedlock'
 *    depending on the new state of 'g_cpu_lockset'.
 * 6. Logic that currently uses the currently running tasks lockcount
 *    instead uses the global 'g_cpu_schedlock'. A value of SP_UNLOCKED
 *    means that no CPU has pre-emption disabled; SP_LOCKED means that at
 *    least one CPU has pre-emption disabled.
 */



volatile spinlock_t g_cpu_schedlock = SP_UNLOCKED;

/* Used to keep track of which CPU(s) hold the IRQ lock. */

volatile spinlock_t g_cpu_locksetlock;
volatile cpu_set_t g_cpu_lockset;

#endif /* CONFIG_SMP */

/************************************************************************
 * Definitions
 ************************************************************************/

/************************************************************************
 * Private Type Declarations
 ************************************************************************/

/************************************************************************
 * Global Variables
 ************************************************************************/

/************************************************************************
 * Private Variables
 ************************************************************************/

/************************************************************************
 * Private Function Prototypes
 ************************************************************************/

/************************************************************************
 * Private Functionss
 ************************************************************************/

/************************************************************************
 * Public Functions
 ************************************************************************/

/************************************************************************
 * Name:  sched_lock
 *
 * Description:
 *   This function disables context switching by disabling addition of
 *   new tasks to the g_readytorun task list.  The task that calls this
 *   function will be the only task that is allowed to run until it
 *   either calls  sched_unlock() (the appropriate number of times) or
 *   until it blocks itself.
 *
 * Inputs
 *   None
 *
 * Return Value:
 *   OK on success; ERROR on failure
 *
 ************************************************************************/

#ifdef CONFIG_SMP
int sched_lock(void)
{
	FAR struct tcb_s *rtcb;

	/* If the CPU supports suppression of interprocessor interrupts, then
	 * simple disabling interrupts will provide sufficient protection for
	 * the following operation.
	 */

	rtcb = this_task();

	/* Check for some special cases:  (1) rtcb may be NULL only during early
	 * boot-up phases, and (2) sched_lock() should have no effect if called
	 * from the interrupt level.
	 */

	if (rtcb != NULL && !up_interrupt_context()) {
		/* Catch attempts to increment the lockcount beyond the range of the
		 * integer type.
		 */

		DEBUGASSERT(rtcb->lockcount < MAX_LOCK_COUNT);

		irqstate_t flags = enter_critical_section();

		/* We must hold the lock on this CPU before we increment the lockcount
		 * for the first time. Holding the lock is sufficient to lockout
		 * context switching.
		 */

		if (rtcb->lockcount == 0) {
			/* We don't have the scheduler locked.  But logic running on a
			 * different CPU may have the scheduler locked.  It is not
			 * possible for some other task on this CPU to have the scheduler
			 * locked (or we would not be executing!).
			 */

			spin_setbit(&g_cpu_lockset, this_cpu(), &g_cpu_locksetlock,
			&g_cpu_schedlock);
		} else {
			/* If this thread already has the scheduler locked, then
			 * g_cpu_schedlock() should indicate that the scheduler is locked
			 * and g_cpu_lockset should include the bit setting for this CPU.
			 */

			DEBUGASSERT(g_cpu_schedlock == SP_LOCKED && \
					(g_cpu_lockset & (1 << this_cpu())) != 0);
		}

		/* A counter is used to support locking.  This allows nested lock
		 * operations on this thread (on any CPU)
		 */

		rtcb->lockcount++;

		/* Move any tasks in the ready-to-run list to the pending task list
		 * where they will not be available to run until the scheduler is
		 * unlocked and nxsched_merge_pending() is called.
		 */

		sched_merge_prioritized((FAR dq_queue_t *)&g_readytorun, \
				(FAR dq_queue_t *)&g_pendingtasks, \
				TSTATE_TASK_PENDING);

		leave_critical_section(flags);
	}

	return OK;
}

#else /* CONFIG_SMP */
int sched_lock(void)
{
	struct tcb_s *rtcb = this_task();

	/* Check for some special cases:  (1) rtcb may be NULL only during
	 * early boot-up phases, and (2) sched_lock() should have no
	 * effect if called from the interrupt level.
	 */

	if (rtcb && !up_interrupt_context()) {
		ASSERT(rtcb->lockcount < MAX_LOCK_COUNT);
		rtcb->lockcount++;
	}

	return OK;
}
#endif /* CONFIG_SMP */
