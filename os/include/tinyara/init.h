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
/****************************************************************************
 *
 *   Copyright (C) 2007, 2008, 2011 Gregory Nutt. All rights reserved.
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

#ifndef __INCLUDE_INIT_H
#define __INCLUDE_INIT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#include <tinyara/compiler.h>
#include <sys/types.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Macros for testing which OS services are available at this phase of
 * initialization.
 */

#define OSINIT_MM_READY()        (g_nx_initstate >= OSINIT_MEMORY)
#define OSINIT_HW_READY()        (g_nx_initstate >= OSINIT_HARDWARE)
#define OSINIT_OS_READY()        (g_nx_initstate >= OSINIT_OSREADY)
#define OSINIT_IDLELOOP()        (g_nx_initstate >= OSINIT_IDLELOOP)
#define OSINIT_OS_INITIALIZING() (g_nx_initstate  < OSINIT_OSREADY)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Initialization state.  OS bring-up occurs in several phases: */

enum os_initstate_e
{
	OSINIT_POWERUP   = 0,  /* Power-up.  No initialization yet performed.
				* Depends on .bss initialization logic for this
				* * value. */
	OSINIT_BOOT      = 1,  /* Basic boot up initialization is complete.  OS
				* services and hardware resources are not yet
				* * available. */
	OSINIT_TASKLISTS = 2,  /* Head of ready-to-run/assigned task lists valid */
	OSINIT_MEMORY    = 3,  /* The memory manager has been initialized */
	OSINIT_HARDWARE  = 4,  /* MCU-specific hardware is initialized.  Hardware
				* resources such as timers and device drivers are
				* now available.  Low-level OS services sufficient
				* to support the hardware are also available but
				* the OS has not yet completed its full
				* initialization. */
	OSINIT_OSREADY   = 5,  /* The OS is fully initialized and multi-tasking is
				* active. */
	OSINIT_IDLELOOP  = 6   /* The OS enter idle loop */
};

/****************************************************************************
 * Global Data
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/* This is the current initialization state.  The level of initialization
 * is only important early in the start-up sequence when certain OS or
 * hardware resources may not yet be available to the OS-internal logic.
 */

EXTERN uint8_t g_os_initstate; /*See enum os_initstae_e */

/****************************************************************************
 * Global Function Prototypes
 ****************************************************************************/

/* This entry point must be supplied by the application */

int CONFIG_USER_ENTRYPOINT(int argc, char *argv[]);
int preapp_start(int argc, char *argv[]);

/* Functions contained in os_task.c *****************************************/
/* OS entry point called by boot logic */

void os_start(void) noreturn_function;

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif							/* __INCLUDE_INIT_H */
