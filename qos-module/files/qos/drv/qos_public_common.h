/*************************************************************************
* qos_public_common.h
*
* Copyright (C) 2015-2017 Renesas Electronics Corporation
*
* License        Dual MIT/GPLv2
*
* The contents of this file are subject to the MIT license as set out below.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Alternatively, the contents of this file may be used under the terms of
* the GNU General Public License Version 2 ("GPL") in which case the provisions
* of GPL are applicable instead of those above.
*
* If you wish to allow use of your version of this file only under the terms of
* GPL, and not to allow others to use your version of this file under the terms
* of the MIT license, indicate your decision by deleting the provisions above
* and replace them with the notice and other provisions required by GPL as set
* out in the file called "GPL-COPYING" included in this distribution. If you do
* not delete the provisions above, a recipient may use your version of this file
* under the terms of either the MIT license or GPL.
*
* This License is also included in this distribution in the file called
* "MIT-COPYING".
*
* EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
* PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
* BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
* PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*
* GPLv2:
* If you wish to use this file under the terms of GPL, following terms are
* effective.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; version 2 of the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*************************************************************************/
#ifndef __QOSPUBLIC_COMMON_H__
#define __QOSPUBLIC_COMMON_H__

#include <linux/types.h>
#include <asm/ioctl.h>

#define QOS_DEVICE_NAME		"qos"
#define QOS_VERSION		"2.07"

enum {
	QOS_TYPE_FIX = 0,
	QOS_TYPE_BE = 1,
	QOS_TYPE_TRAFFIC_MONITOR = 2,
	QOS_TYPE_MAX
};

struct qos_ioc_get_status_param {
	__u8 statqen;
	__u8 exe_membank;
};

struct qos_ioc_set_all_qos_param {
	__u8 *fix_qos;
	__u8 *be_qos;
};


struct qos_ioc_set_ip_qos_param {
	__u8 qos_type;
	__u16 master_id;
	__u64 qos;
};

struct qos_ioc_get_ip_qos_param {
	__u8 qos_type;
	__u16 master_id;
	__u8 membank;
	__u64 qos;
};

#define QOS_IOCTL_BASE			'q'
#define QOS_IO(nr)			_IO(QOS_IOCTL_BASE, nr)
#define QOS_IOR(nr, type)		_IOR(QOS_IOCTL_BASE, nr, type)
#define QOS_IOW(nr, type)		_IOW(QOS_IOCTL_BASE, nr, type)
#define QOS_IOWR(nr, type)		_IOWR(QOS_IOCTL_BASE, nr, type)

#define QOS_IOCTL_SET_ALL_QOS	\
		QOS_IOW(0x01, struct qos_ioc_set_all_qos_param)
#define QOS_IOCTL_SWITCH_MEMBANK	\
		QOS_IO(0x03)

#define QOS_IOCTL_MAX_NR		0x04

#endif /* __QOSPUBLIC_COMMON_H__ */
