/*************************************************************************/ /*
 qos_reg.h

 Copyright (C) 2015-2017 Renesas Electronics Corporation

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
#ifndef __QOS_REG_H__
#define __QOS_REG_H__

#define QOS_REG_BASE			(0xE67E0000)
#define QOS_REG_SIZE			(0x00004000)
#define QOS_BANK_SIZE			(0x00000008)
#define QOS_FIX_BANK_SIZE		(0x00001000)
#define QOS_BE_BANK_SIZE		(0x00001000)

#define QOS_FIX_0			(0x00000000)
#define QOS_FIX_1			(0x00001000)
#define QOS_BE_0			(0x00002000)
#define QOS_BE_1			(0x00003000)
#define QOS_MEMORY_BANK			(0x0000800C)

#define STATQEN_MASK			(0x00000001)
#define EXE_MEMBANK_MASK		(0x00000100)

#define MASTER_ID_MAX_H3_ES1		103
#define MASTER_ID_MAX_H3_ES2		109
#define MASTER_ID_MAX_M3_W		 99
#define MASTER_ID_MAX			511

#define PRR_REG_BASE			(0xFFF00000)
#define PRR_REG_SIZE			(0x00000048)
#define PRR				(0x00000044)

#define PRODUCT_ID_NUMBER_MASK		(0x00007F00)
#define CUT_NUMBER_MASK			(0x000000FF)

#define R_CAR_H3			(0x00004F00)
#define R_CAR_M3_W			(0x00005200)

#define ES10				(0x00000000)
#define ES11				(0x00000001)
#define ES20				(0x00000010)
#define ES30				(0x00000020)

#endif /* __QOS_REG_H__ */
