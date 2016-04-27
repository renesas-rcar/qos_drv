/*************************************************************************/ /*
 qos_drv.c

 Copyright (C) 2015 Renesas Electronics Corporation

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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "qos_core.h"
#include "qos_reg.h"

/* #define DEBUG */

#ifdef DEBUG
#define QOS_DBG(fmt, args...) \
		pr_debug("%s: " fmt "\n", __func__, ##args)
#else
#define QOS_DBG(fmt, args...) do { } while (0)
#endif

static int qos_set_all_qos(unsigned long arg);
static int qos_switch_membank(unsigned long arg);

typedef int (*qos_ioctl_t)(unsigned long);

static const qos_ioctl_t qos_ioctls[QOS_IOCTL_MAX_NR] = {
	[_IOC_NR(QOS_IOCTL_SET_ALL_QOS)] = qos_set_all_qos,
	[_IOC_NR(QOS_IOCTL_SWITCH_MEMBANK)] = qos_switch_membank,
};


static int qos_open(struct inode *inode, struct file *filp)
{
	QOS_DBG("begin");

	QOS_DBG("end");

	return 0;
}

static long qos_unlocked_ioctl(struct file *filp, unsigned int cmd,
	unsigned long arg)
{
	int ret = -EINVAL;
	qos_ioctl_t func;

	QOS_DBG("begin");

	if (_IOC_NR(cmd) >= QOS_IOCTL_MAX_NR) {
		pr_err("COMMAND NUM error\n");
		return -ENOTTY;
	}

	func = qos_ioctls[_IOC_NR(cmd)];
	if (func == NULL) {
		pr_err("COMMAND NUM error\n");
		return -ENOTTY;
	}

	ret = func(arg);

	QOS_DBG("end");

	return ret;
}

static int qos_close(struct inode *inode, struct file *filp)
{
	QOS_DBG("begin");

	QOS_DBG("end");

	return 0;
}

const struct file_operations qos_fops = {
	.owner	  = THIS_MODULE,
	.unlocked_ioctl = qos_unlocked_ioctl,
	.open	   = qos_open,
	.release	= qos_close,
};

static struct miscdevice qos_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = QOS_DEVICE_NAME,
	.fops  = &qos_fops,
};


static int __init qos_init(void)
{
	int ret = -ENOMEM;

	QOS_DBG("begin");

	pr_info("QoS: install v%s\n", QOS_VERSION);

	ret = rcar_qos_init();
	if (ret) {
		pr_err("failed to rcar_qos_init()\n");
		return ret;
	}

	ret = misc_register(&qos_miscdev);
	if (ret) {
		pr_err("failed to misc_register (MISC_DYNAMIC_MINOR)\n");
		return ret;
	}

	pr_info("QoS Driver is Successfully loaded\n");

	QOS_DBG("end");

	return ret;
}

static void __exit qos_exit(void)
{
	QOS_DBG("begin");

	misc_deregister(&qos_miscdev);

	rcar_qos_exit();

	pr_info("QoS Driver is unloaded\n");

	QOS_DBG("end");

}

module_init(qos_init);
module_exit(qos_exit);
MODULE_LICENSE("Dual MIT/GPL");

static int qos_set_all_qos(unsigned long arg)
{
	int ret = 0;
	struct qos_ioc_set_all_qos_param param;
	struct qos_ioc_set_all_qos_param tmp;

	QOS_DBG("begin");

	param.fix_qos = NULL;
	param.be_qos = NULL;

	param.fix_qos = kmalloc(QOS_FIX_BANK_SIZE, GFP_KERNEL);
	if (param.fix_qos == NULL) {
		ret = -ENOMEM;
		goto err_i1;
	}

	param.be_qos = kmalloc(QOS_BE_BANK_SIZE, GFP_KERNEL);
	if (param.be_qos == NULL) {
		ret = -ENOMEM;
		goto err_i1;
	}

	if (copy_from_user(&tmp, (void __user *)arg, sizeof(tmp))) {
		pr_err("QoS(qos_set_all_qos): copy param error\n");
		ret = -EFAULT;
		goto err_i1;
	}

	if (copy_from_user(param.fix_qos,
		(void __user *)(tmp.fix_qos), QOS_FIX_BANK_SIZE)) {
		pr_err("QoS(qos_set_all_qos): copy param error\n");
		ret = -EFAULT;
		goto err_i1;
	}

	if (copy_from_user(param.be_qos,
		(void __user *)(tmp.be_qos), QOS_BE_BANK_SIZE)) {
		pr_err("QoS(qos_set_all_qos): copy param error\n");
		ret = -EFAULT;
		goto err_i1;
	}

	ret = rcar_qos_set_all_qos(&param);
	if (ret) {
		pr_err("QoS(qos_set_all_qos): failed to rcar_qos_set_all_qos() errno=[%d]\n",
			ret);
		goto err_i1;
	}

err_i1:
	if (param.fix_qos != NULL)
		kfree(param.fix_qos);
	if (param.be_qos != NULL)
		kfree(param.be_qos);

	QOS_DBG("end");

	return ret;
}

static int qos_switch_membank(unsigned long arg)
{
	int ret = 0;

	QOS_DBG("begin");

	ret = rcar_qos_switch_membank();
	if (ret) {
		pr_err("QoS(qos_switch_membank): failed to rcar_qos_switch_membank() errno=[%d]\n",
			ret);
		return ret;
	}

	QOS_DBG("end");

	return ret;
}