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
 * drivers/analog/adc.c
 *
 *   Copyright (C) 2008-2009, 2016 Gregory Nutt. All rights reserved.
 *   Copyright (C) 2011 Li Zhuoyi. All rights reserved.
 *   Author: Li Zhuoyi <lzyy.cn@gmail.com>
 *           Gregory Nutt <gnutt@nuttx.org>
 *
 * Derived from drivers/can.c
 *
 *   Copyright (C) 2008-2009 Gregory Nutt. All rights reserved.
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

#include <tinyara/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#include <tinyara/fs/fs.h>
#include <tinyara/arch.h>
#include <tinyara/semaphore.h>
#include <tinyara/analog/adc.h>

#include <tinyara/irq.h>

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
static int     adc_open(FAR struct file *filep);
static int     adc_close(FAR struct file *filep);
static ssize_t adc_read(FAR struct file *fielp, FAR char *buffer,
			size_t buflen);
static int     adc_ioctl(FAR struct file *filep, int cmd, unsigned long arg);
static int     adc_receive(FAR struct adc_dev_s *dev, uint8_t ch,
			   int32_t data);
static void    adc_notify(FAR struct adc_dev_s *dev);
#ifndef CONFIG_DISABLE_POLL
static int     adc_poll(FAR struct file *filep, struct pollfd *fds, bool setup);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/
static const struct file_operations g_adc_fops = {
	adc_open,	/* open */
	adc_close,	/* close */
	adc_read,	/* read */
	0,		/* write */
	0,		/* seek */
	adc_ioctl,	/* ioctl */
#ifndef CONFIG_DISABLE_POLL
	adc_poll,		/* poll */
#endif
};

static const struct adc_callback_s g_adc_callback = {
	adc_receive	/* au_receive */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/****************************************************************************
 * Name: adc_open
 *
 * Description:
 *   This function is called whenever the ADC device is opened.
 *
 ****************************************************************************/
static int adc_open(FAR struct file *filep)
{
	FAR struct inode     *inode = filep->f_inode;
	FAR struct adc_dev_s *dev   = inode->i_private;
	uint8_t               tmp;
	int                   ret   = OK;

	/*
	 * If the port is the middle of closing, wait until the close is
	 * finished
	 */
	if (sem_wait(&dev->ad_closesem) != OK) {
		ret = -errno;
	} else {
		/*
		 * Increment the count of references to the device.  If this
		 * the first time that the driver has been opened for this
		 * device, then initialize the device.
		 */
		tmp = dev->ad_ocount + 1;
		if (tmp == 0) {
			/* More than 255 opens; uint8_t overflows to zero */
			ret = -EMFILE;
		} else {
			/*
			 * Check if this is the first time that the driver
			 * has been opened.
			 */
			if (tmp == 1) {
				/*
				 * Yes.. perform one time hardware
				 * initialization.
				 */
				irqstate_t flags = enter_critical_section();
				ret = dev->ad_ops->ao_setup(dev);
				if (ret == OK) {
					/* Mark the FIFOs empty */
					dev->ad_recv.af_head = 0;
					dev->ad_recv.af_tail = 0;

					/*
					 * Finally, Enable the ADC RX
					 * interrupt
					 */
					dev->ad_ops->ao_rxint(dev, true);

					/* Save the new open count on success */
					dev->ad_ocount = tmp;
				}

				leave_critical_section(flags);
			}
		}

		sem_post(&dev->ad_closesem);
	}

	return ret;
}

/****************************************************************************
 * Name: adc_close
 *
 * Description:
 *   This routine is called when the ADC device is closed.
 *   It waits for the last remaining data to be sent.
 *
 ****************************************************************************/
static int adc_close(FAR struct file *filep)
{
	FAR struct inode     *inode = filep->f_inode;
	FAR struct adc_dev_s *dev   = inode->i_private;
	irqstate_t            flags;
	int                   ret = OK;
#ifndef CONFIG_DISABLE_POLL
	int                   i;
#endif

	if (sem_wait(&dev->ad_closesem) != OK) {
		ret = -errno;
	} else {
#ifndef CONFIG_DISABLE_POLL
		/* Check if this file is registered in a list of waiters for polling.
		 * For example, when task A is blocked by calling poll and task B try to terminate task A,
		 * a pollfd of A remains in this list. If it is, it should be cleared.
		 */
		for (i = 0; i < CONFIG_ADC_NPOLLWAITERS; i++) {
			struct pollfd *fds = dev->fds[i];
			if (fds && (FAR struct file *)fds->filep == filep) {
				dev->fds[i] = NULL;
			}
		}
#endif

		/*
		 * Decrement the references to the driver.  If the reference
		 * count will decrement to 0, then uninitialize the driver.
		 */
		if (dev->ad_ocount > 1) {
			dev->ad_ocount--;
			sem_post(&dev->ad_closesem);
		} else {
			/* There are no more references to the port */
			dev->ad_ocount = 0;

			/* Free the IRQ and disable the ADC device */
			flags = enter_critical_section(); /* Disable interrupts */
			dev->ad_ops->ao_shutdown(dev); /* Disable the ADC */
			leave_critical_section(flags);

			sem_post(&dev->ad_closesem);
		}
	}

	return ret;
}

/****************************************************************************
 * Name: adc_read
 ****************************************************************************/
static ssize_t adc_read(FAR struct file *filep, FAR char *buffer,
			size_t buflen)
{
	FAR struct inode     *inode = filep->f_inode;
	FAR struct adc_dev_s *dev   = inode->i_private;
	size_t                nread;
	irqstate_t            flags;
	int                   ret   = 0;
	int                   msglen;

	avdbg("buflen: %d\n", (int)buflen);

	if (buflen % 5 == 0)
		msglen = 5;
	else if (buflen % 4 == 0)
		msglen = 4;
	else if (buflen % 3 == 0)
		msglen = 3;
	else if (buflen % 2 == 0)
		msglen = 2;
	else if (buflen == 1)
		msglen = 1;
	else
		msglen = 5;

	if (buflen >= msglen) {
		/*
		 * Interrupts must be disabled while accessing the ad_recv
		 * FIFO
		 */
		flags = enter_critical_section();
		while (dev->ad_recv.af_head == dev->ad_recv.af_tail) {
			/*
			 * The receive FIFO is empty
			 * -- was non-blocking mode selected?
			 */
			if (filep->f_oflags & O_NONBLOCK) {
				ret = -EAGAIN;
				goto return_with_irqdisabled;
			}

			/* Wait for a message to be received */
			dev->ad_nrxwaiters++;
			ret = sem_wait(&dev->ad_recv.af_sem);
			dev->ad_nrxwaiters--;
			if (ret < 0) {
				ret = -errno;
				goto return_with_irqdisabled;
			}
		}

		/*
		 * The ad_recv FIFO is not empty.
		 * Copy all buffered data that will fit in the user buffer.
		 */
		nread = 0;
		do {
			FAR struct adc_msg_s *msg =
				&dev->ad_recv.af_buffer[dev->ad_recv.af_head];

			/*
			 * Will the next message in the FIFO fit into the
			 * user buffer?
			 */
			if (nread + msglen > buflen) {
				/*
				 * No.. break out of the loop now with nread
				 * equal to the actual number of bytes
				 * transferred.
				 */
				break;
			}

			/* Copy the message to the user buffer */
			if (msglen == 1) {
				/* Only one channel,read highest 8-bits */
				buffer[nread] = msg->am_data >> 24;
			} else if (msglen == 2) {
				/* Only one channel, read highest 16-bits */
				*(int16_t *)&buffer[nread] =
							msg->am_data >> 16;
			} else if (msglen == 3) {
				/* Read channel highest 16-bits */
				buffer[nread] = msg->am_channel;
				*(int16_t *)&buffer[nread + 1] =
							msg->am_data >> 16;
			} else if (msglen == 4) {
				/* read channel highest 24-bits */
				*(int32_t *)&buffer[nread] = msg->am_data;
				buffer[nread] = msg->am_channel;
			} else {
				/* Read all */
				*(int32_t *)&buffer[nread + 1] = msg->am_data;
				buffer[nread] = msg->am_channel;
			}
			nread += msglen;

			/* Increment the head of the circular message buffer */
			if (++dev->ad_recv.af_head >= CONFIG_ADC_FIFOSIZE) {
				dev->ad_recv.af_head = 0;
			}
		} while (dev->ad_recv.af_head != dev->ad_recv.af_tail);

		/*
		 * All on the messages have bee transferred.
		 * Return the number of bytes that were read.
		 */
		ret = nread;

return_with_irqdisabled:
		leave_critical_section(flags);
	}

	avdbg("Returning: %d\n", ret);
	return ret;
}

/****************************************************************************
 * Name: adc_ioctl
 ****************************************************************************/
static int adc_ioctl(FAR struct file *filep, int cmd, unsigned long arg)
{
	FAR struct inode *inode = filep->f_inode;
	FAR struct adc_dev_s *dev = inode->i_private;
	int ret;

	ret = dev->ad_ops->ao_ioctl(dev, cmd, arg);
	return ret;
}

/****************************************************************************
 * Name: adc_receive
 ****************************************************************************/
static int adc_receive(FAR struct adc_dev_s *dev, uint8_t ch, int32_t data)
{
	FAR struct adc_fifo_s *fifo = &dev->ad_recv;
	int                    nexttail;
	int                    errcode = -ENOMEM;

	/*
	 * Check if adding this new message would over-run the drivers
	 * ability to enqueue read data.
	 */
	nexttail = fifo->af_tail + 1;
	if (nexttail >= CONFIG_ADC_FIFOSIZE) {
		nexttail = 0;
	}

	/* Refuse the new data if the FIFO is full */
	if (nexttail != fifo->af_head) {
		/* Add the new, decoded ADC sample at the tail of the FIFO */
		fifo->af_buffer[fifo->af_tail].am_channel = ch;
		fifo->af_buffer[fifo->af_tail].am_data    = data;

		/* Increment the tail of the circular buffer */
		fifo->af_tail = nexttail;

		adc_notify(dev);

		errcode = OK;
	}

	return errcode;
}

/****************************************************************************
 * Name: adc_pollnotify
 ****************************************************************************/

#ifndef CONFIG_DISABLE_POLL
static void adc_pollnotify(FAR struct adc_dev_s *dev, uint32_t type)
{
	int i;

	for (i = 0; i < CONFIG_ADC_NPOLLWAITERS; i++) {
		struct pollfd *fds = dev->fds[i];
		if (fds) {
			fds->revents |= type;
			sem_post(fds->sem);
		}
	}
}
#endif

/****************************************************************************
 * Name: adc_notify
 ****************************************************************************/

static void adc_notify(FAR struct adc_dev_s *dev)
{
	FAR struct adc_fifo_s *fifo = &dev->ad_recv;

	/* If there are threads waiting for read data, then signal one of them
	* that the read data is available.
	*/

	if (dev->ad_nrxwaiters > 0) {
		sem_post(&fifo->af_sem);
	}

	/* If there are threads waiting on poll() for data to become available,
	* then wake them up now.
	*/

#ifndef CONFIG_DISABLE_POLL
	adc_pollnotify(dev, POLLIN);
#endif
}

/************************************************************************************
 * Name: adc_poll
 ************************************************************************************/

#ifndef CONFIG_DISABLE_POLL
static int adc_poll(FAR struct file *filep, struct pollfd *fds, bool setup)
{
	FAR struct inode     *inode = filep->f_inode;
	FAR struct adc_dev_s *dev   = inode->i_private;
	irqstate_t flags;
	int ret = 0;
	int i;

	/* Interrupts must be disabled while accessing the list of poll structures
	* and ad_recv FIFO.
	*/

	flags = enter_critical_section();

	if (setup) {
		/* Ignore waits that do not include POLLIN */

		if ((fds->events & POLLIN) == 0) {
			ret = -EDEADLK;
			goto return_with_irqdisabled;
		}

		/* This is a request to set up the poll.  Find an available
		* slot for the poll structure reference
		*/

		for (i = 0; i < CONFIG_ADC_NPOLLWAITERS; i++) {
			/* Find an available slot */

			if (!dev->fds[i]) {
				/* Bind the poll structure and this slot */

				dev->fds[i] = fds;
				fds->priv   = &dev->fds[i];
				fds->filep = (void *)filep;
				break;
			}
		}

		if (i >= CONFIG_ADC_NPOLLWAITERS) {
			fds->priv    = NULL;
			fds->filep   = NULL;
			ret          = -EBUSY;
			goto return_with_irqdisabled;
		}

		/* Should we immediately notify on any of the requested events? */

		if (dev->ad_recv.af_head != dev->ad_recv.af_tail) {
			adc_pollnotify(dev, POLLIN);
		}
	} else if (fds->priv) {
		/* This is a request to tear down the poll. */

		struct pollfd **slot = (struct pollfd **)fds->priv;

		/* Remove all memory of the poll setup */

		*slot                = NULL;
		fds->priv            = NULL;
		fds->filep           = NULL;
	}

return_with_irqdisabled:
	leave_critical_section(flags);
	return ret;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: adc_register
 ****************************************************************************/
int adc_register(FAR const char *path, FAR struct adc_dev_s *dev)
{
	int ret;

	DEBUGASSERT(path != NULL && dev != NULL);

	/* Bind the upper-half callbacks to the lower half ADC driver */
	DEBUGASSERT(dev->ad_ops != NULL && dev->ad_ops->ao_bind != NULL);
	ret = dev->ad_ops->ao_bind(dev, &g_adc_callback);
	if (ret < 0) {
		avdbg("ERROR: Failed to bind callbacks: %d\n", ret);
		return ret;
	}

	/* Initialize the ADC device structure */
	dev->ad_ocount = 0;

	/* Initialize semaphores */
	sem_init(&dev->ad_recv.af_sem, 0, 0);
	sem_init(&dev->ad_closesem, 0, 1);

	/*
	 * The receive semaphore is used for signaling and, hence,
	 * should not have priority inheritance enabled.
	 */
	sem_setprotocol(&dev->ad_recv.af_sem, SEM_PRIO_NONE);

	/* Reset the ADC hardware */
	DEBUGASSERT(dev->ad_ops->ao_reset != NULL);
	dev->ad_ops->ao_reset(dev);

	/* Register the ADC character driver */
	ret = register_driver(path, &g_adc_fops, 0444, dev);
	if (ret < 0) {
		sem_destroy(&dev->ad_recv.af_sem);
		sem_destroy(&dev->ad_closesem);
	}

	return ret;
}
