/*************************************************************************/ /*
 qos_core.c

 Copyright (C) 2015-2021 Renesas Electronics Corporation

 License        Dual MIT/GPLv2

 The contents of this file are subject to the MIT license as set out below.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 Alternatively, the contents of this file may be used under the terms of
 the GNU General Public License Version 2 ("GPL") in which case the provisions
 of GPL are applicable instead of those above.

 If you wish to allow use of your version of this file only under the terms of
 GPL, and not to allow others to use your version of this file under the terms
 of the MIT license, indicate your decision by deleting the provisions above
 and replace them with the notice and other provisions required by GPL as set
 out in the file called "GPL-COPYING" included in this distribution. If you do
 not delete the provisions above, a recipient may use your version of this file
 under the terms of either the MIT license or GPL.

 This License is also included in this distribution in the file called
 "MIT-COPYING".

 EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
 COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


 GPLv2:
 If you wish to use this file under the terms of GPL, following terms are
 effective.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/ /*************************************************************************/

#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/of_address.h>

#include "qos_core.h"
#include "qos_reg.h"

/* #define DEBUG */

#ifdef DEBUG
#define QOS_DBG(fmt, args...) \
		printk("%s: " fmt "\n", __func__, ##args)
#else
#define QOS_DBG(fmt, args...) do { } while (0)
#endif

#ifndef readq
#define readq(addr) (readl(addr) | (((__u64) readl((addr) + 4)) << 32))
#endif

#ifndef writeq
#define writeq(val, addr) do { \
	writel((__u32) (val), (addr)); \
	writel((__u32) ((val) >> 32), (addr) + 4); \
} while (0)
#endif

#define READ_REG32(address)		readl(address)
#define READ_REG64(address)		readq(address)
#define WRITE_REG32(value, address)	writel(value, address)
#define WRITE_REG64(value, address)	writeq(value, address)

#define QOS_BANK_OFF(__index) (QOS_BANK_SIZE * (__index))

extern uint32_t qos_base;				// Physical address of QoS module
extern void __iomem *qos_reg_base;		// Vitural address of QoS module

static DEFINE_MUTEX(qos_mutex);

static __u32 device, device_version;
static int master_id_max;
static int init;
static __u8 exe_membank_bk;
static bool support_exe_membank = true;

static __u8 fix_qos_buf[QOS_FIX_BANK_SIZE] = {};
static __u8 be_qos_buf[QOS_BE_BANK_SIZE] = {};

static __u8 backup_bank[QOS_REG_SIZE];

#define WAIT_SWITCH_BANK_US_MIN	(100)
#define WAIT_SWITCH_BANK_US_MAX	(1000)
#define WAIT_SWITCH_BANK_US	(10)
#define WAIT_RETRY_COUNT		(5)

static inline void qos_reg_load(void *src, __u32 offset, int index);
static inline void qos_reg_store(void *dst, __u32 offset, int index);
static int rcar_qos_wait_switching(__u32 value);

int rcar_qos_init(void)
{
	int ret = 0;

	__u32 prr;
	struct device_node *np;
	void __iomem *prr_reg_base = NULL;

	QOS_DBG("begin");

	mutex_lock(&qos_mutex);

	if (!init) {
        /* Try PRR first, then hardcoded fallback */
        np = of_find_compatible_node(NULL, NULL, "renesas,prr");
        if (np) {
			prr_reg_base = of_iomap(np, 0);
			of_node_put(np);

			if (prr_reg_base) {
				prr = readl(prr_reg_base);
				iounmap(prr_reg_base);
				device = prr & PRODUCT_ID_NUMBER_MASK;
				device_version = prr & CUT_NUMBER_MASK;

				QOS_DBG(
				"Succeeded to get device model[0x%08x],device version[0x%08x]",
					device, device_version);
			}
        } else {
			pr_err("%s: of_find_compatible_node[renesas,prr] error\n", __func__);
			ret = -ENOMEM;
		}

		if (device == R_CAR_H3) {
			switch (device_version) {
			case ES10:
				pr_info("Device \"R-Car H3 Ver.1.0\"\r\n");
				fallthrough;
			case ES11:
				pr_info("Device \"R-Car H3 Ver1.1\"\r\n");
				master_id_max = MASTER_ID_MAX_H3_ES1;
				support_exe_membank = false;
				break;
			case ES20:
				pr_info("Device \"R-Car H3 Ver2.0\"\r\n");
				fallthrough;
			default:
				master_id_max = MASTER_ID_MAX_H3_ES2;
				break;
			}
		} else if (device == R_CAR_M3_W) {
			switch (device_version) {
			case ES10:
				pr_info("Device \"R-Car M3 Ver1.0\"\r\n");
				fallthrough;
			case ES20: /* Ver1.1 */
				pr_info("Device \"R-Car M3 Ver1.1\"\r\n");
				fallthrough;
			default:
				master_id_max = MASTER_ID_MAX_M3_W;
				break;
			}
		} else if (device == R_CAR_M3_N) {
			switch (device_version) {
			case ES10:
				pr_info("Device \"R-Car M3N Ver1.0\"\r\n");
				fallthrough;
			default:
				master_id_max = MASTER_ID_MAX_M3_N;
				break;
			}
		} else if (device == R_CAR_D3) {
			switch (device_version) {
			case ES10:
				pr_info("Device \"R-Car D3 Ver1.0\"\r\n");
				fallthrough;
			default:
				master_id_max = MASTER_ID_MAX_D3;
				break;
			}
		} else if (device == R_CAR_E3) {
			switch (device_version) {
			case ES10:
				pr_info("Device \"R-Car E3 Ver1.0\"\r\n");
				fallthrough;
			default:
				master_id_max = MASTER_ID_MAX_E3;
				break;
			}
		} else if (device == R_CAR_V3U) {
			switch (device_version) {
			case ES10:
				pr_info("Device \"R-Car V3U Ver1.0\"\r\n");
				fallthrough;
			default:
				master_id_max = MASTER_ID_MAX_V3U;
				break;
			}
		} else if (device == R_CAR_V3H) {
			switch (device_version) {
			case ES11:
				pr_info("Device \"R-Car V3H Ver1.1\"\r\n");
				fallthrough;
			case ES20:
				pr_info("Device \"R-Car V3H Ver2.0\"\r\n");
				fallthrough;
			default:
				master_id_max = MASTER_ID_MAX_V3H;
				break;
			}
		} else if (device == R_CAR_V3M) {
			switch (device_version) {
			case ES20:
				pr_info("Device \"R-Car V3M Ver2.0\"\r\n");
				fallthrough;
			default:
				master_id_max = MASTER_ID_MAX_V3M;
				break;
			}
		} else if (device == R_CAR_V4H) {
			switch (device_version) {
			case ES10:
				pr_info("Device \"R-Car V4H Ver1.0\"\r\n");
				fallthrough;
			case ES20:
				pr_info("Device \"R-Car V4H Ver2.0\"\r\n");
				fallthrough;
			default:
				master_id_max = MASTER_ID_MAX_V4H;
				break;
			}
		} else if (device == R_CAR_S4) {
			switch (device_version) {
			case ES10:
				pr_info("Device \"R-Car S4 Ver1.0\"\r\n");
				fallthrough;
			case ES11:
				pr_info("Device \"R-Car S4 Ver1.1\"\r\n");
				fallthrough;
			default:
				master_id_max = MASTER_ID_MAX_S4;
				break;
			}
		}

		if (master_id_max == 0) {
			device = 0;
			device_version = 0;
			pr_err("%s: not support chip\n", __func__);
			ret = -ENOMEM;
		}
		QOS_DBG("Number of master id[%u]", master_id_max);

		init = 1;
	}

	mutex_unlock(&qos_mutex);

	QOS_DBG("end");

	return ret;

}

void rcar_qos_exit(void)
{

	QOS_DBG("begin");

	mutex_lock(&qos_mutex);

	if (init) {

		device = 0;
		device_version = 0;
		master_id_max = 0;
		init = 0;
	}

	mutex_unlock(&qos_mutex);

	QOS_DBG("end");
}

int rcar_qos_set_all_qos(struct qos_ioc_set_all_qos_param *param)
{
	__u32 qos_fix_offset = 0x00000000;
	__u32 qos_be_offset = 0x00000000;
	__u32 exe_membank;
	int i;

	QOS_DBG("begin");

	mutex_lock(&qos_mutex);

	if (!support_exe_membank) {
		exe_membank = exe_membank_bk;
	} else {
		QOS_DBG("Read Reg[QOS_REG_TYPE_MEMORY_BANK][0x%08x], value[0x%08x]",
						(qos_base + QOSCTRL_MEMBANK),
						READ_REG32(qos_reg_base + QOSCTRL_MEMBANK));

		exe_membank = (READ_REG32(qos_reg_base + QOSCTRL_MEMBANK)
						& EXE_MEMBANK_MASK) >> 8;
	}

	qos_fix_offset |= (QOS_TYPE_FIX << 13) & 0x0000E000;
	qos_fix_offset |= ((exe_membank ^ 0x00000001) << 12) & 0x00001000;

	qos_be_offset |= (QOS_TYPE_BE << 13) & 0x0000E000;
	qos_be_offset |= ((exe_membank ^ 0x00000001) << 12) & 0x00001000;

	QOS_DBG("QoS Fix Offset[0x%08x]", qos_fix_offset);
	QOS_DBG("QoS BE  Offset[0x%08x]", qos_be_offset);

	for (i = 0; i < master_id_max + 1; i++)
		qos_reg_load(param->fix_qos, qos_fix_offset, i);

	for (i = 0; i < master_id_max + 1; i++)
		qos_reg_load(param->be_qos, qos_be_offset, i);

	mutex_unlock(&qos_mutex);

	QOS_DBG("end");

	return 0;
}

int rcar_qos_switch_membank(void)
{
	__u32 memory_bank;
	__u32 qos_fix_offset = 0x00000000;
	__u32 qos_be_offset = 0x00000000;
	__u32 exe_membank;
	__u32 value = 0x00000000;
	int i;
	int ret = 0;

	QOS_DBG("begin");

	mutex_lock(&qos_mutex);

	memory_bank = READ_REG32(qos_reg_base + QOSCTRL_MEMBANK);
	QOS_DBG("Read Reg[QOS_REG_TYPE_MEMORY_BANK][0x%08x], value[0x%08x]",
					(qos_base + QOSCTRL_MEMBANK), memory_bank);

	if (!support_exe_membank)
		exe_membank = exe_membank_bk;
	else
		exe_membank = (memory_bank & EXE_MEMBANK_MASK) >> 8;

	qos_fix_offset |= (QOS_TYPE_FIX << 13) & 0x0000E000;
	qos_fix_offset |= ((exe_membank ^ 0x00000001) << 12) & 0x00001000;

	qos_be_offset |= (QOS_TYPE_BE << 13) & 0x0000E000;
	qos_be_offset |= ((exe_membank ^ 0x00000001) << 12) & 0x00001000;

	for (i = 0; i < master_id_max + 1; i++)
		qos_reg_store(fix_qos_buf, qos_fix_offset, i);

	for (i = 0; i < master_id_max + 1; i++)
		qos_reg_store(be_qos_buf, qos_be_offset, i);

	value |= memory_bank & 0xFFFFFFFE;
	value |= (exe_membank ^ 0x00000001) & 0x00000001;

	if (rcar_qos_wait_switching(value)) {
		ret = -ETIMEDOUT;
		goto err_i1;
	}

	exe_membank_bk = (exe_membank ^ 0x00000001) & 0x00000001;

	qos_fix_offset = 0x00000000;
	qos_fix_offset |= (QOS_TYPE_FIX << 13) & 0x0000E000;
	qos_fix_offset |= (exe_membank << 12) & 0x00001000;

	qos_be_offset = 0x00000000;
	qos_be_offset |= (QOS_TYPE_BE << 13) & 0x0000E000;
	qos_be_offset |= (exe_membank << 12) & 0x00001000;

	for (i = 0; i < master_id_max + 1; i++)
		qos_reg_load(fix_qos_buf, qos_fix_offset, i);

	for (i = 0; i < master_id_max + 1; i++)
		qos_reg_load(be_qos_buf, qos_be_offset, i);

err_i1:
	mutex_unlock(&qos_mutex);

	QOS_DBG("end");

	return ret;
}

static void qos_sram_backup(__u32 qos_fix_offset, __u32 qos_be_offset)
{
	int i;

	for (i = 0; i < master_id_max + 1; i++)
		qos_reg_store(backup_bank + qos_fix_offset, qos_fix_offset, i);

	for (i = 0; i < master_id_max + 1; i++)
		qos_reg_store(backup_bank + qos_be_offset, qos_be_offset, i);
}

void rcar_qos_suspend(void)
{
	__u32 exe_membank;
	__u32 qos_fix_offset = 0x00000000;
	__u32 qos_be_offset = 0x00000000;

	exe_membank = 0;
	qos_fix_offset |= (QOS_TYPE_FIX << 13) & 0x0000E000;
	qos_fix_offset |= (exe_membank << 12) & 0x00001000;

	qos_be_offset = 0x00000000;
	qos_be_offset |= (QOS_TYPE_BE << 13) & 0x0000E000;
	qos_be_offset |= (exe_membank << 12) & 0x00001000;

	qos_sram_backup(qos_fix_offset, qos_be_offset);

	qos_fix_offset = 0x00000000;
	qos_fix_offset |= (QOS_TYPE_FIX << 13) & 0x0000E000;
	qos_fix_offset |= ((exe_membank ^ 0x00000001) << 12) & 0x00001000;

	qos_be_offset |= (QOS_TYPE_BE << 13) & 0x0000E000;
	qos_be_offset |= ((exe_membank ^ 0x00000001) << 12) & 0x00001000;

	qos_sram_backup(qos_fix_offset, qos_be_offset);
}

static void qos_sram_reload(__u32 qos_fix_offset, __u32 qos_be_offset)
{
	int i;

	for (i = 0; i < master_id_max + 1; i++)
		qos_reg_load(backup_bank + qos_fix_offset, qos_fix_offset, i);

	for (i = 0; i < master_id_max + 1; i++)
		qos_reg_load(backup_bank + qos_be_offset, qos_be_offset, i);
}

void rcar_qos_resume(void)
{
	__u32 exe_membank;
	__u32 qos_fix_offset = 0x00000000;
	__u32 qos_be_offset = 0x00000000;
	__u32 value = 0x00000000;

	exe_membank = 0;
	qos_fix_offset |= (QOS_TYPE_FIX << 13) & 0x0000E000;
	qos_fix_offset |= ((exe_membank ^ 0x00000001) << 12) & 0x00001000;

	qos_be_offset |= (QOS_TYPE_BE << 13) & 0x0000E000;
	qos_be_offset |= ((exe_membank ^ 0x00000001) << 12) & 0x00001000;

	QOS_DBG("QoS Fix Offset[0x%08x]\n", qos_fix_offset);
	QOS_DBG("QoS BE  Offset[0x%08x]\n", qos_be_offset);

	qos_sram_reload(qos_fix_offset, qos_be_offset);

	value = exe_membank & 0xFFFFFFFE;
	value |= (exe_membank ^ 0x00000001) & 0x00000001;

	rcar_qos_wait_switching(value);

	qos_fix_offset = 0x00000000;
	qos_fix_offset |= (QOS_TYPE_FIX << 13) & 0x0000E000;
	qos_fix_offset |= (exe_membank << 12) & 0x00001000;

	qos_be_offset = 0x00000000;
	qos_be_offset |= (QOS_TYPE_BE << 13) & 0x0000E000;
	qos_be_offset |= (exe_membank << 12) & 0x00001000;

	QOS_DBG("QoS Fix Offset[0x%08x]\n", qos_fix_offset);
	QOS_DBG("QoS BE  Offset[0x%08x]\n", qos_be_offset);

	qos_sram_reload(qos_fix_offset, qos_be_offset);

	if (exe_membank_bk == 0) {
		value = exe_membank_bk;
		rcar_qos_wait_switching(value);
	}
}

static inline void qos_reg_load(void *src, __u32 offset, int index)
{
	/* QOS_DBG("[%d] Write value 0x%016llx to IP address 0x%08x\n", index, \
		*((__u64 *)(src + QOS_BANK_OFF(index))), \
		(qos_base + offset + QOS_BANK_OFF(index))); */
	WRITE_REG64(*((__u64 *)(src + QOS_BANK_OFF(index))),
		qos_reg_base + offset + QOS_BANK_OFF(index));
}

static inline void qos_reg_store(void *dst, __u32 offset, int index)
{
	*((__u64 *)(dst + QOS_BANK_OFF(index))) =
		READ_REG64(qos_reg_base + offset + QOS_BANK_OFF(index));
	/* QOS_DBG("[%d] Store value 0x%016llx of IP address 0x%08x\n", index, \
		*((__u64 *)(dst + QOS_BANK_OFF(index))), \
		(qos_base + offset + QOS_BANK_OFF(index))); */
}

static int rcar_qos_wait_switching(__u32 value)
{
	int ret = 0;

	QOS_DBG("Write Reg[QOS_REG_TYPE_MEMORY_BANK][0x%08x], value[0x%08x]\n",
						(qos_base + QOSCTRL_MEMBANK), value);
	WRITE_REG32(value, qos_reg_base + QOSCTRL_MEMBANK);

	if (!support_exe_membank) {
		usleep_range(WAIT_SWITCH_BANK_US_MIN,
			WAIT_SWITCH_BANK_US_MAX);
	} else {
		int timeout = WAIT_RETRY_COUNT;
		__u32 memory_bank;

		while (timeout--) {
			memory_bank = READ_REG32(qos_reg_base + QOSCTRL_MEMBANK);
			if (((memory_bank & EXE_MEMBANK_MASK) >> 8)
						== (memory_bank & 0x00000001)) {
				break;
			}
			udelay(WAIT_SWITCH_BANK_US);
		}

		if (timeout <= 0) {
			ret = -ETIMEDOUT;
			pr_err("rcar_qos_switch_membank: timeout switch membank[errno=%d]\n",
				ret);
		}
	}

	return ret;
}
