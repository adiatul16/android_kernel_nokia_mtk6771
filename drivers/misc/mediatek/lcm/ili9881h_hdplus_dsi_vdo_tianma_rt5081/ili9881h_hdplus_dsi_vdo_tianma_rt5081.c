﻿/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#define LOG_TAG "LCM"

#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/time.h>
#endif

#include "lcm_drv.h"


#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <asm-generic/gpio.h>
#include <linux/string.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#endif


#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif

#define LCM_ID_NT35695 (0xf5)
#define LONG_V_MODE
#define ILI_CE_1

static const unsigned int BL_MIN_LEVEL = 20;
static struct LCM_UTIL_FUNCS lcm_util;
static int lcm_init_isdone = 0;

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define MDELAY(n)		(lcm_util.mdelay(n))
#define UDELAY(n)		(lcm_util.udelay(n))

#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
		lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
	  lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
		lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
/* #include <linux/jiffies.h> */
/* #include <linux/delay.h> */
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#endif

/* static unsigned char lcd_id_pins_value = 0xFF; */
static const unsigned char LCD_MODULE_ID = 0x01;
#define LCM_DSI_CMD_MODE									0
#define FRAME_WIDTH										(720)
#define FRAME_HEIGHT									(1520)

/* physical size in um */
#define LCM_PHYSICAL_WIDTH									(63720)
#define LCM_PHYSICAL_HEIGHT									(134520)
#define LCM_DENSITY											(320)

#define REGFLAG_DELAY		0xFFFC
#define REGFLAG_UDELAY	0xFFFB
#define REGFLAG_END_OF_TABLE	0xFFFD
#define REGFLAG_RESET_LOW	0xFFFE
#define REGFLAG_RESET_HIGH	0xFFFF

//static LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode_cmd;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

//zachary add
extern void FIH_tp_lcm_resume(void);
extern void FIH_tp_lcm_suspend(void);
extern void FIH_ili_tp_lcm_resume(void);
extern void FIH_ili_tp_lcm_suspend(void);
extern unsigned int islcmconnected;
extern int gdouble_tap_enable_nvt;
//zachary add

extern unsigned short fih_hwid;

extern unsigned int g_fih_panelid;

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 0, {} },
	{REGFLAG_DELAY, 20, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 100, {} },
};

static struct LCM_setting_table init_setting_cmd[] = {
	/*{REGFLAG_DELAY, 200, {} },*/
	{0x11, 0, {} },
	{REGFLAG_DELAY, 120, {} },
	{0x29, 0, {} },
	/*{REGFLAG_DELAY, 200, {} },*/
	/* ///////////////////CABC SETTING///////// */
	{0x51, 1, {0x00} },
	{0x5E, 1, {0x00} },
	{0x53, 1, {0x24} },
	{0x55, 1, {0x00} },
};

#if 0
static struct LCM_setting_table init_setting_vdo_preEVT[] = {
	{0xFF, 1, {0x20} },
	{REGFLAG_DELAY, 10, {} }, 
	{0xFB, 1, {0x01} },
	{0x05, 1, {0x89} },
	{0x07, 1, {0x50} },
	{0x08, 1, {0xAD} },
	{0x0E, 1, {0x69} },
	{0x0F, 1, {0x46} },
	{0x69, 1, {0xA9} },
	{0x87, 1, {0x02} },
	{0x89, 1, {0x86} },
	{0x8A, 1, {0x86} },
	{0x8B, 1, {0x86} },
	{0x8C, 1, {0x86} },
	{0x94, 1, {0x00} },
	{0x95, 1, {0xF5} },
	{0x96, 1, {0xF5} },
	{0xFF, 1, {0x24} },
	{REGFLAG_DELAY, 10, {} }, 
	{0xFB, 1, {0x01} },
	{0x00, 1, {0x00} },
	{0x01, 1, {0x1E} },
	{0x02, 1, {0x1F} },
	{0x03, 1, {0x08} },
	{0x04, 1, {0x08} },
	{0x05, 1, {0x00} },
	{0x06, 1, {0x00} },
	{0x07, 1, {0x04} },
	{0x08, 1, {0x05} },
	{0x09, 1, {0x15} },
	{0x0A, 1, {0x14} },
	{0x0B, 1, {0x17} },
	{0x0C, 1, {0x16} },
	{0x16, 1, {0x00} },
	{0x17, 1, {0x1E} },
	{0x18, 1, {0x1F} },
	{0x19, 1, {0x08} },
	{0x1A, 1, {0x08} },
	{0x1B, 1, {0x00} },
	{0x1C, 1, {0x00} },
	{0x1D, 1, {0x04} },
	{0x1E, 1, {0x05} },
	{0x1F, 1, {0x0D} },
	{0x20, 1, {0x0C} },
	{0x21, 1, {0x0F} },
	{0x22, 1, {0x0E} },
	{0x37, 1, {0x44} },
	{0x2F, 1, {0x0A} },
	{0x30, 1, {0x08} },
	{0x33, 1, {0x08} },
	{0x34, 1, {0x0A} },
	{0x39, 1, {0x00} },
	{0x3A, 1, {0x60} },
	{0x3B, 1, {0x8C} },
	{0x3D, 1, {0x52} },
	{0x3F, 1, {0x0E} },
	{0x43, 1, {0x0E} },
	{0x47, 1, {0x44} },
	{0x4A, 1, {0x05} },
	{0x4B, 1, {0x50} },
	{0x4C, 1, {0x51} },
	{0x4D, 1, {0x12} },
	{0x4E, 1, {0x34} },
	{0x51, 1, {0x43} },
	{0x52, 1, {0x21} },
	{0x55, 1, {0x47} },
	{0x56, 1, {0x44} },
	{0x58, 1, {0x21} },
	{0x59, 1, {0x00} },
	{0x5A, 1, {0x0A} },
	{0x5B, 1, {0x96} },
	{0x5C, 1, {0x8F} },
	{0x5D, 1, {0x0A} },
	{0x5E, 1, {0x04} },
	{0x60, 1, {0x80} },
	{0x61, 1, {0x7C} },
	{0x64, 1, {0x10} },
	{0x68, 1, {0x12} },
	{0x69, 1, {0x34} },
	{0x6A, 1, {0x43} },
	{0x6B, 1, {0x21} },
	{0x6C, 1, {0x47} },
	{0x6D, 1, {0x44} },
	{0x6E, 1, {0x21} },
	{0x6F, 1, {0x00} },
	{0x70, 1, {0x0A} },
	{0x71, 1, {0x96} },
	{0x72, 1, {0x8F} },
	{0x73, 1, {0x0A} },
	{0x74, 1, {0x04} },
	{0x85, 1, {0x02} },
	{0x92, 1, {0xB0} },
	{0x93, 1, {0x06} },
	{0x94, 1, {0x0A} },
	{0x9D, 1, {0x30} },
	{0xAB, 1, {0x00} },
	{0xAC, 1, {0x00} },
	{0xAD, 1, {0x00} },
	{0xB0, 1, {0x14} },
	{0xB1, 1, {0x9C} },
	{0xFF, 1, {0x25} },
	{REGFLAG_DELAY, 10, {} },    
	{0xFB, 1, {0x01} },
	{0x0A, 1, {0x82} },
	{0x0B, 1, {0x9B} },
	{0x0C, 1, {0x01} },
	{0x17, 1, {0x82} },
	{0x18, 1, {0x06} },
	{0x19, 1, {0x0F} },
	{0x1E, 1, {0x00} },
	{0x1F, 1, {0x0A} },
	{0x20, 1, {0x96} },
	{0x21, 1, {0x0A} },
	{0x22, 1, {0x96} },
	{0x23, 1, {0x14} },
	{0x24, 1, {0x9C} },
	{0x25, 1, {0x00} },
	{0x26, 1, {0x0A} },
	{0x27, 1, {0x96} },
	{0x28, 1, {0x0A} },
	{0x29, 1, {0x96} },
	{0x2A, 1, {0x14} },
	{0x2B, 1, {0x9C} },
	{0x2F, 1, {0x55} },
	{0x30, 1, {0x00} },
	{0x31, 1, {0x00} },
	{0x32, 1, {0x00} },
	{0x33, 1, {0x0A} },
	{0x34, 1, {0x96} },
	{0x35, 1, {0x0A} },
	{0x36, 1, {0x96} },
	{0x37, 1, {0x14} },
	{0x38, 1, {0x9C} },
	{0x40, 1, {0x11} },
	{0x41, 1, {0x80} },
	{0x42, 1, {0x0A} },
	{0x43, 1, {0x8C} },
	{0x44, 1, {0x0A} },
	{0x45, 1, {0x8C} },
	{0x46, 1, {0x14} },
	{0x47, 1, {0x82} },
	{0x4A, 1, {0x00} },
	{0x4B, 1, {0x05} },
	{0x4C, 1, {0x8C} },
	{0x4F, 1, {0x0A} },
	{0x50, 1, {0x96} },
	{0x51, 1, {0x0A} },
	{0x52, 1, {0x96} },
	{0x53, 1, {0x14} },
	{0x54, 1, {0x9C} },
	{0x55, 1, {0x8F} },
	{0x56, 1, {0x21} },
	{0x58, 1, {0x00} },
	{0x59, 1, {0x00} },
	{0x5A, 1, {0x22} },
	{0x5B, 1, {0xC0} },
	{0x5C, 1, {0x00} },
	{0x5D, 1, {0x05} },
	{0x5E, 1, {0xB0} },
	{0x61, 1, {0x0A} },
	{0x62, 1, {0x96} },
	{0x63, 1, {0x0A} },
	{0x64, 1, {0x96} },
	{0x65, 1, {0x14} },
	{0x66, 1, {0x9C} },
	{0x6B, 1, {0x44} },
	{0x6C, 1, {0x0D} },
	{0x6D, 1, {0x0D} },
	{0x6E, 1, {0x0F} },
	{0x6F, 1, {0x0F} },
	{0x78, 1, {0x00} },
	{0x79, 1, {0x60} },
	{0x7A, 1, {0x00} },
	{0x7B, 1, {0x01} },
	{0x8A, 1, {0x02} },
	{0xCA, 1, {0x1C} },
	{0xCC, 1, {0x1C} },
	{0xCD, 1, {0x00} },
	{0xCE, 1, {0x00} },
	{0xCF, 1, {0x1C} },
	{0xD0, 1, {0x00} },
	{0xD1, 1, {0x00} },
	{0xD3, 1, {0x11} },
	{0xD4, 1, {0xCC} },
	{0xD5, 1, {0x11} },
	{0xFF, 1, {0x26} },
	{REGFLAG_DELAY, 10, {} },  
	{0xFB, 1, {0x01} },
	{0x00, 1, {0xA1} },
	{0x04, 1, {0x4B} },
	{0x0C, 1, {0x0B} },
	{0x0D, 1, {0x01} },
	{0x0E, 1, {0x02} },
	{0x0F, 1, {0x06} },
	{0x10, 1, {0x07} },
	{0x11, 1, {0x00} },
	{0x13, 1, {0x2A} },
	{0x14, 1, {0x88} },
	{0x16, 1, {0x81} },
	{0x19, 1, {0x15} },
	{0x1A, 1, {0x0D} },
	{0x1B, 1, {0x12} },
	{0x1C, 1, {0x82} },
	{0x1D, 1, {0x00} },
	{0x1E, 1, {0xB0} },
	{0x1F, 1, {0xB0} },
	{0x24, 1, {0x01} },
	{0x25, 1, {0xB0} },
	{0x2F, 1, {0x03} },
	{0x30, 1, {0x96} },
	{0x31, 1, {0x11} },
	{0x32, 1, {0x11} },
	{0x34, 1, {0x02} },
	{0x35, 1, {0xB0} },
	{0x36, 1, {0x67} },
	{0x37, 1, {0x11} },
	{0x38, 1, {0x11} },
	{0x3F, 1, {0x04} },
	{0x40, 1, {0xB0} },
	{0x41, 1, {0x00} },
	{0x42, 1, {0x00} },
	{0x49, 1, {0x00} },
	{0x58, 1, {0xD7} },
	{0x59, 1, {0xD7} },
	{0x5A, 1, {0xD7} },
	{0x5B, 1, {0xD7} },
	{0x5C, 1, {0x01} },
	{0x5D, 1, {0x06} },
	{0x5E, 1, {0x04} },
	{0x5F, 1, {0x00} },
	{0x60, 1, {0x00} },
	{0x61, 1, {0x00} },
	{0x62, 1, {0x00} },
	{0x63, 1, {0x60} },
	{0x64, 1, {0xB0} },
	{0x65, 1, {0x05} },
	{0x66, 1, {0xB0} },
	{0x67, 1, {0x0A} },
	{0x68, 1, {0x96} },
	{0x69, 1, {0x0A} },
	{0x6A, 1, {0x96} },
	{0x6B, 1, {0x00} },
	{0x6C, 1, {0x00} },
	{0x6D, 1, {0x00} },
	{0x70, 1, {0x14} },
	{0x71, 1, {0x9C} },
	{0x73, 1, {0x0A} },
	{0x74, 1, {0x96} },
	{0x75, 1, {0x0A} },
	{0x76, 1, {0x96} },
	{0x77, 1, {0x14} },
	{0x78, 1, {0x9C} },
	{0x7A, 1, {0x0A} },
	{0x7B, 1, {0x96} },
	{0x7C, 1, {0x0A} },
	{0x7D, 1, {0x96} },
	{0x7E, 1, {0x14} },
	{0x7F, 1, {0x9C} },
	{0x82, 1, {0x60} },
	{0x83, 1, {0xB0} },
	{0x84, 1, {0x05} },
	{0x85, 1, {0xB0} },
	{0x86, 1, {0x0A} },
	{0x87, 1, {0x96} },
	{0x88, 1, {0x0A} },
	{0x89, 1, {0x96} },
	{0x8A, 1, {0x14} },
	{0x8B, 1, {0x9C} },
	{0x8C, 1, {0x00} },
	{0x8D, 1, {0x00} },
	{0x8E, 1, {0x01} },
	{0x8F, 1, {0x00} },
	{0x90, 1, {0x00} },
	{0x91, 1, {0x00} },
	{0x92, 1, {0x05} },
	{0x93, 1, {0xF0} },
	{0x94, 1, {0x00} },
	{0x96, 1, {0x00} },
	{0x99, 1, {0x0D} },
	{0x9A, 1, {0x36} },
	{0x9B, 1, {0x0C} },
	{0x9C, 1, {0x9E} },
	{0xFF, 1, {0x23} },
	{REGFLAG_DELAY, 10, {} },    
	{0xFB, 1, {0x01} },
	{0x12, 1, {0xAB} },
	{0x15, 1, {0xF5} },
	{0x16, 1, {0x0B} },
	{0xFF, 1, {0x27} },
	{REGFLAG_DELAY, 10, {} },   
	{0xFB, 1, {0x01} },
	{0x13, 1, {0x00} },
	{0xFF, 1, {0xE0} },
	{REGFLAG_DELAY, 10, {} },	    
	{0xFB, 1, {0x01} },
	{0x9E, 1, {0x00} },
	{0xFF, 1, {0x10} },
	{REGFLAG_DELAY, 10, {} },   
	{0xFB, 1, {0x01} },
//	{0xB0, 1, {0x01} },
	{0xBA, 1, {0x02} },//MIPI Lane 0x02:3lane 0x03:4lane

	/*{REGFLAG_DELAY, 200, {} },*/
	{0x11, 0, {} },
	{REGFLAG_DELAY, 120, {} },
	{0x29, 0, {} },
};
#endif

#if 0 //20180808@ray: for 4-lanes configuration
static struct LCM_setting_table init_setting_vdo[] = {
	{0xFF, 3, {0x98, 0x81, 0x00} },
	{REGFLAG_DELAY, 100, {} },

	/* Sleep Out */
	{0x11, 1, {0x00} },
	{REGFLAG_DELAY, 100, {} },
	
	/* Display ON */
	{0x29, 1, {0x00} },
	{REGFLAG_DELAY, 100, {} },
	{0x35, 1, {0x00} },    //TE enable     
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};
#endif

static struct LCM_setting_table init_setting_vdo_3lane[] = {
#ifdef ILI_CE_0
	{0xFF, 3, {0x98, 0x81, 0x04} },
	{0x00, 1, {0x83} },
	{0x02, 1, {0x41} },
	{0x0A, 1, {0x23} },
	{0x0B, 1, {0x25} },
	{0x0C, 1, {0x24} },
	{0x0D, 1, {0x25} },
	{0x0E, 1, {0x25} },
	{0x0F, 1, {0x25} },
	{0x10, 1, {0x26} },
	{0x11, 1, {0x26} },
	{0x12, 1, {0x25} },
	{0x13, 1, {0x25} },
	{0x14, 1, {0x24} },
	{0x15, 1, {0x22} },
	{0x16, 1, {0x06} },
	{0x17, 1, {0x0D} },
	{0x18, 1, {0x0F} },
	{0x19, 1, {0x0A} },
	{0x1A, 1, {0x07} },
	{0x1B, 1, {0x0A} },
	{0x1C, 1, {0x0B} },
	{0x1D, 1, {0x09} },
	{0x1E, 1, {0x05} },
	{0x1F, 1, {0x01} },
	{0x20, 1, {0x00} },
	{0x21, 1, {0x23} },
#elif defined ILI_CE_1
	{0xFF, 3, {0x98, 0x81, 0x04} },
	{0x00, 1, {0x83} },
	{0x02, 1, {0x41} },
	{0x0A, 1, {0x01} },
	{0x0B, 1, {0x22} },
	{0x0C, 1, {0x22} },
	{0x0D, 1, {0x23} },
	{0x0E, 1, {0x23} },
	{0x0F, 1, {0x22} },
	{0x10, 1, {0x22} },
	{0x11, 1, {0x22} },
	{0x12, 1, {0x21} },
	{0x13, 1, {0x21} },
	{0x14, 1, {0x00} },
	{0x15, 1, {0x02} },
	{0x16, 1, {0x09} },
	{0x17, 1, {0x0F} },
	{0x18, 1, {0x12} },
	{0x19, 1, {0x10} },
	{0x1A, 1, {0x0F} },
	{0x1B, 1, {0x15} },
	{0x1C, 1, {0x14} },
	{0x1D, 1, {0x13} },
	{0x1E, 1, {0x0D} },
	{0x1F, 1, {0x08} },
	{0x20, 1, {0x08} },
	{0x21, 1, {0x05} },
#elif defined ILI_CE_2
	{0xFF, 3, {0x98, 0x81, 0x04} },
	{0x00, 1, {0x83} },
	{0x02, 1, {0x41} },
	{0x0A, 1, {0x07} },
	{0x0B, 1, {0x04} },
	{0x0C, 1, {0x02} },
	{0x0D, 1, {0x01} },
	{0x0E, 1, {0x01} },
	{0x0F, 1, {0x02} },
	{0x10, 1, {0x02} },
	{0x11, 1, {0x03} },
	{0x12, 1, {0x04} },
	{0x13, 1, {0x04} },
	{0x14, 1, {0x05} },
	{0x15, 1, {0x07} },
	{0x16, 1, {0x0E} },
	{0x17, 1, {0x16} },
	{0x18, 1, {0x1A} },
	{0x19, 1, {0x19} },
	{0x1A, 1, {0x18} },
	{0x1B, 1, {0x1D} },
	{0x1C, 1, {0x1E} },
	{0x1D, 1, {0x1F} },
	{0x1E, 1, {0x17} },
	{0x1F, 1, {0x10} },
	{0x20, 1, {0x0E} },
	{0x21, 1, {0x0B} },
#elif defined ILI_CE_3
	{0xFF, 3, {0x98, 0x81, 0x04} },
	{0x00, 1, {0x83} },
	{0x02, 1, {0x41} },
	{0x0A, 1, {0x0D} },
	{0x0B, 1, {0x0A} },
	{0x0C, 1, {0x08} },
	{0x0D, 1, {0x06} },
	{0x0E, 1, {0x05} },
	{0x0F, 1, {0x05} },
	{0x10, 1, {0x05} },
	{0x11, 1, {0x06} },
	{0x12, 1, {0x08} },
	{0x13, 1, {0x09} },
	{0x14, 1, {0x0A} },
	{0x15, 1, {0x0C} },
	{0x16, 1, {0x12} },
	{0x17, 1, {0x19} },
	{0x18, 1, {0x1F} },
	{0x19, 1, {0x1F} },
	{0x1A, 1, {0x1E} },
	{0x1B, 1, {0x1E} },
	{0x1C, 1, {0x1E} },
	{0x1D, 1, {0x1F} },
	{0x1E, 1, {0x1A} },
	{0x1F, 1, {0x15} },
	{0x20, 1, {0x11} },
	{0x21, 1, {0x11} },
#endif
	// 3-lanes configuration
	{0xFF, 3, {0x98, 0x81, 0x06} },
	{0x7C, 1, {0x40} }, 
	{0xDD, 1, {0x10} }, 
	
	{0xFF, 3, {0x98, 0x81, 0x00} },
	/* Sleep Out */
	{0x11, 1, {0x00} },
	
	/* Display ON */
	{REGFLAG_DELAY, 80, {} },
	{0x29, 1, {0x00} },
	//{0x35, 1, {0x00} },    //TE enable     
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

#if 0
static struct LCM_setting_table lcm_set_window[] = {
	{0x2A, 4, {0x00, 0x00, (FRAME_WIDTH >> 8), (FRAME_WIDTH & 0xFF)} },
	{0x2B, 4, {0x00, 0x00, (FRAME_HEIGHT >> 8), (FRAME_HEIGHT & 0xFF)} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};
#endif
#if 0
static struct LCM_setting_table lcm_sleep_out_setting[] = {
	/* Sleep Out */
	{0x11, 1, {0x00} },
	{REGFLAG_DELAY, 120, {} },

	/* Display ON */
	{0x29, 1, {0x00} },
	{REGFLAG_DELAY, 20, {} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	/* Display off sequence */
	{0x28, 1, {0x00} },
	{REGFLAG_DELAY, 20, {} },

	/* Sleep Mode On */
	{0x10, 1, {0x00} },
	{REGFLAG_DELAY, 120, {} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};
#endif
static struct LCM_setting_table bl_level[] = {
	{0x51, 1, {0xFF} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static void push_table(void *cmdq, struct LCM_setting_table *table,
	unsigned int count, unsigned char force_update)
{
	unsigned int i;
	unsigned cmd;

	for (i = 0; i < count; i++) {
		cmd = table[i].cmd;

		switch (cmd) {

		case REGFLAG_DELAY:
			if (table[i].count <= 10)
				MDELAY(table[i].count);
			else
				MDELAY(table[i].count);
			break;

		case REGFLAG_UDELAY:
			UDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

		default:
			dsi_set_cmdq_V22(cmdq, cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}


static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}


static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->physical_width = LCM_PHYSICAL_WIDTH/1000;
	params->physical_height = LCM_PHYSICAL_HEIGHT/1000;
	params->physical_width_um = LCM_PHYSICAL_WIDTH;
	params->physical_height_um = LCM_PHYSICAL_HEIGHT;
	params->density            = LCM_DENSITY;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
	lcm_dsi_mode = CMD_MODE;
#else
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
	lcm_dsi_mode = SYNC_PULSE_VDO_MODE;
#endif
	LCM_LOGI("[LCM-tianma] lcm_get_params lcm_dsi_mode %d\n", lcm_dsi_mode);
	params->dsi.switch_mode_enable = 0;

	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_THREE_LANE;
	//params->dsi.LANE_NUM = LCM_FOUR_LANE;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	params->dsi.packet_size = 256;
	/* video mode timing */

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active = 2;
#ifdef LONG_V_MODE
	params->dsi.vertical_backporch = 20;
	params->dsi.vertical_frontporch = 240;
#else
	params->dsi.vertical_backporch = 8;
	params->dsi.vertical_frontporch = 38;
#endif
//	params->dsi.vertical_frontporch_for_low_power = 620;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 10;
#ifdef LONG_V_MODE
	params->dsi.horizontal_backporch = 10;
	params->dsi.horizontal_frontporch = 32;
#else
	params->dsi.horizontal_backporch = 90;
	params->dsi.horizontal_frontporch = 100;
#endif
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	/* params->dsi.ssc_disable                                                   = 1; */
#ifndef CONFIG_FPGA_EARLY_PORTING
#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 420;	/* this value must be in MTK suggested table */
#else
	#ifdef LONG_V_MODE
	params->dsi.PLL_CLOCK =347;	/* this value must be in MTK suggested table */
	#else
	params->dsi.PLL_CLOCK =363;	/* this value must be in MTK suggested table */
	#endif
	//params->dsi.PLL_CLOCK =220;	/* this value must be in MTK suggested table */
#endif
	params->dsi.PLL_CK_CMD = 420;
	params->dsi.PLL_CK_VDO = 347;
#else
	params->dsi.pll_div1 = 0;
	params->dsi.pll_div2 = 0;
	params->dsi.fbk_div = 0x1;
#endif
	params->dsi.CLK_HS_POST = 36;
	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 0;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;

#ifdef CONFIG_NT35695_LANESWAP
	params->dsi.lane_swap_en = 1;

	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_0] = MIPITX_PHY_LANE_CK;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_1] = MIPITX_PHY_LANE_2;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_2] = MIPITX_PHY_LANE_3;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_3] = MIPITX_PHY_LANE_0;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_CK] = MIPITX_PHY_LANE_1;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_RX] = MIPITX_PHY_LANE_1;
#endif
	/* for ARR 2.0 */
//	params->max_refresh_rate = 60;
//	params->min_refresh_rate = 45;
//	params->min_refresh_rate = 60;
}

static void lcm_init_power(void)
{
	display_bias_enable();
}

static void lcm_suspend_power(void)
{
	pr_err("[LCM-tianma] %s:islcmconnected=%d \n", __func__,islcmconnected);

#if 1
	if (!gdouble_tap_enable_nvt)
	{
	    pr_err("%s-tianma: TP&LCM power down\n", __func__ );
	    display_bias_disable();
	}
	else
	    pr_err("%s-tianma: TP&LCM keep power source\n", __func__ );
	MDELAY(5);
//	SET_RESET_PIN(0);
#endif
}

static void lcm_resume_power(void)
{
	pr_debug("[LCM-tianma KPI] %s: power up start \n", __func__);
	
#if 1
	if (!gdouble_tap_enable_nvt)
	{
	    pr_err("%s-tianma: TP&LCM power up\n", __func__ );
	    display_bias_enable();
	}
	else
		pr_err("%s-tianma: TP&LCM power source already Up.\n", __func__ );
	MDELAY(4);
#endif
	pr_debug("[LCM-tianma KPI] %s: power up end \n", __func__);
}

static void lcm_init(void)
{
	pr_debug("[LCM-tianma KPI] %s: RST start \n", __func__);

	SET_RESET_PIN(1);
	MDELAY(5);
	SET_RESET_PIN(0);
	MDELAY(5);
	SET_RESET_PIN(1);
	MDELAY(51);

	pr_debug("[LCM-tianma KPI] %s: RST end \n", __func__);

	pr_debug("[LCM-tianma KPI] %s: Inin code start \n", __func__);
	if (lcm_dsi_mode == CMD_MODE) {
		push_table(NULL, init_setting_cmd, sizeof(init_setting_cmd) / sizeof(struct LCM_setting_table), 1);
		LCM_LOGI("ili9881----tianma----lcm mode = cmd mode :%d----\n", lcm_dsi_mode);
	} else {
		push_table(NULL, init_setting_vdo_3lane, sizeof(init_setting_vdo_3lane) / sizeof(struct LCM_setting_table), 1);
		LCM_LOGI("ili9881----tianma----lcm mode = vdo mode :%d----\n", lcm_dsi_mode);
	}
        lcm_init_isdone = 1;
	pr_debug("[LCM-tianma KPI] %s: Inin code end \n", __func__);
}

static void lcm_suspend(void)
{
#if 1
	if (islcmconnected==1)
	    FIH_ili_tp_lcm_suspend();
#endif
	push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
	MDELAY(10);
	pr_debug("[LCM-tianma] %s:ili9881_fhd_dsi_vdo_tianma_rt5081\n", __func__);
	/* SET_RESET_PIN(0); */
}

static void lcm_resume(void)
{
#if 1
	struct timespec before_tp, after_tp;
	int display_deley = 0;
#endif

	pr_debug("[LCM-tianma] %s:ili9881_fhd_dsi_vdo_tianma_rt5081\n", __func__);
	lcm_init();
#if 1
	pr_err("[LCM-tianma] %s:islcmconnected=%d \n", __func__,islcmconnected);
	if (islcmconnected==1){
		getnstimeofday(&before_tp);
		FIH_ili_tp_lcm_resume();
		getnstimeofday(&after_tp);
		pr_debug("[LCM-tianma KPI]before = %lu.%lu s\n", before_tp.tv_sec, (before_tp.tv_nsec));
		pr_debug("[LCM-tianma KPI]after = %lu.%lu s\n", after_tp.tv_sec, (after_tp.tv_nsec));
		if ( after_tp.tv_sec == before_tp.tv_sec ){
			display_deley = (int)(after_tp.tv_nsec - before_tp.tv_nsec)/1000000;
			pr_debug("[LCM-tianma KPI]1. time diff = %d ms\n", display_deley);
		} else if ( after_tp.tv_sec > before_tp.tv_sec ) {
			pr_debug("[LCM-tianma KPI]2. %lu s\n", (after_tp.tv_sec - before_tp.tv_sec));
			display_deley = (int)(1000 - before_tp.tv_nsec/1000000) + after_tp.tv_nsec/1000000;
			pr_debug("[LCM-tianma KPI]2. time diff = %d ms\n", display_deley);
		} else {
			pr_err("[LCM-tianma] Time error!!!\n");
		}
		pr_debug("[LCM-tianma KPI] Delay start!\n");
		if ( display_deley <= 100 )
			MDELAY(100-display_deley);
		pr_debug("[LCM-tianma KPI] Delay end!\n");
	}
#endif
}

static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);
	unsigned char y0_MSB = ((y0 >> 8) & 0xFF);
	unsigned char y0_LSB = (y0 & 0xFF);
	unsigned char y1_MSB = ((y1 >> 8) & 0xFF);
	unsigned char y1_LSB = (y1 & 0xFF);

	unsigned int data_array[16];

	data_array[0] = 0x00053902;
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00053902;
	data_array[1] = (y1_MSB << 24) | (y0_LSB << 16) | (y0_MSB << 8) | 0x2b;
	data_array[2] = (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);
}

// static unsigned int lcm_compare_id(void)
// {
	// unsigned int id = 0, version_id = 0;
	// unsigned char buffer[2];
	// unsigned int array[16];

	// SET_RESET_PIN(1);
	// SET_RESET_PIN(0);
	// MDELAY(1);

	// SET_RESET_PIN(1);
	// MDELAY(20);

	// array[0] = 0x00023700;	/* read id return two byte,version and id */
	// dsi_set_cmdq(array, 1, 1);

	// read_reg_v2(0xF4, buffer, 2);
	// id = buffer[0];     /* we only need ID */

	// read_reg_v2(0xDB, buffer, 1);
	// version_id = buffer[0];

	// LCM_LOGI("%s,ili9881_id=0x%08x,version_id=0x%x\n", __func__, id, version_id);

	// if (id == LCM_ID_NT35695 && version_id == 0x81)
		// return 1;
	// else
		// return 0;

// }


/* return TRUE: need recovery */
/* return FALSE: No need recovery */
static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK
	char buffer[3];
	int array[4];

	array[0] = 0x00013700;
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x53, buffer, 1);

	if (buffer[0] != 0x24) {
		LCM_LOGI("[LCM ERROR] [0x53]=0x%02x\n", buffer[0]);
		return TRUE;
	}
	LCM_LOGI("[LCM NORMAL] [0x53]=0x%02x\n", buffer[0]);
	return FALSE;
#else
	return FALSE;
#endif

}

#if 0 //gaty
static unsigned int lcm_ata_check(unsigned char *buffer)
{
#ifndef BUILD_LK
	unsigned int ret = 0;
	unsigned int x0 = FRAME_WIDTH / 4;
	unsigned int x1 = FRAME_WIDTH * 3 / 4;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);

	unsigned int data_array[3];
	unsigned char read_buf[4];

	LCM_LOGI("ATA check size = 0x%x,0x%x,0x%x,0x%x\n", x0_MSB, x0_LSB, x1_MSB, x1_LSB);
	data_array[0] = 0x0005390A;	/* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00043700;	/* read id return two byte,version and id */
	dsi_set_cmdq(data_array, 1, 1);

	read_reg_v2(0x2A, read_buf, 4);

	if ((read_buf[0] == x0_MSB) && (read_buf[1] == x0_LSB)
	    && (read_buf[2] == x1_MSB) && (read_buf[3] == x1_LSB))
		ret = 1;
	else
		ret = 0;

	x0 = 0;
	x1 = FRAME_WIDTH - 1;

	x0_MSB = ((x0 >> 8) & 0xFF);
	x0_LSB = (x0 & 0xFF);
	x1_MSB = ((x1 >> 8) & 0xFF);
	x1_LSB = (x1 & 0xFF);

	data_array[0] = 0x0005390A;	/* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	return ret;
#else
	return 0;
#endif
}
#endif

static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{

	LCM_LOGI("%s,ili9881 backlight: level = %d\n", __func__, level);

	bl_level[0].para_list[0] = level;

	push_table(handle, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}

#if 0 //gaty
static void *lcm_switch_mode(int mode)
{
#ifndef BUILD_LK
/* customization: 1. V2C config 2 values, C2V config 1 value; 2. config mode control register */
	if (mode == 0) {	/* V2C */
		lcm_switch_mode_cmd.mode = CMD_MODE;
		lcm_switch_mode_cmd.addr = 0xBB;	/* mode control addr */
		lcm_switch_mode_cmd.val[0] = 0x13;	/* enabel GRAM firstly, ensure writing one frame to GRAM */
		lcm_switch_mode_cmd.val[1] = 0x10;	/* disable video mode secondly */
	} else {		/* C2V */
		lcm_switch_mode_cmd.mode = SYNC_PULSE_VDO_MODE;
		lcm_switch_mode_cmd.addr = 0xBB;
		lcm_switch_mode_cmd.val[0] = 0x03;	/* disable GRAM and enable video mode */
	}
	return (void *)(&lcm_switch_mode_cmd);
#else
	return NULL;
#endif
}
#endif

#if (LCM_DSI_CMD_MODE)

/* partial update restrictions:
 * 1. roi width must be 1080 (full lcm width)
 * 2. vertical start (y) must be multiple of 16
 * 3. vertical height (h) must be multiple of 16
 */
static void lcm_validate_roi(int *x, int *y, int *width, int *height)
{
	unsigned int y1 = *y;
	unsigned int y2 = *height + y1 - 1;
	unsigned int x1, w, h;

	x1 = 0;
	w = FRAME_WIDTH;

	y1 = round_down(y1, 16);
	h = y2 - y1 + 1;

	/* in some cases, roi maybe empty. In this case we need to use minimu roi */
	if (h < 16)
		h = 16;

	h = round_up(h, 16);

	/* check height again */
	if (y1 >= FRAME_HEIGHT || y1 + h > FRAME_HEIGHT) {
		/* assign full screen roi */
		pr_warn("%s calc error,assign full roi:y=%d,h=%d\n", __func__, *y, *height);
		y1 = 0;
		h = FRAME_HEIGHT;
	}

	/*pr_err("lcm_validate_roi (%d,%d,%d,%d) to (%d,%d,%d,%d)\n",*/
	/*	*x, *y, *width, *height, x1, y1, w, h);*/

	*x = x1;
	*width = w;
	*y = y1;
	*height = h;
}
#endif

//#if (LCM_DSI_CMD_MODE)
//LCM_DRIVER nt35695B_fhd_dsi_cmd_auo_rt5081_lcm_drv = {
//	.name = "nt35695B_fhd_dsi_cmd_auo_rt5081_drv",
//#else

struct LCM_DRIVER ili9881h_hdplus_dsi_vdo_tianma_rt5081_lcm_drv = {
	.name = "ili9881h_hdplus_dsi_vdo_tianma_rt5081_drv",
//#endif
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
//	.compare_id = lcm_compare_id,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.esd_check = lcm_esd_check,
	.set_backlight_cmdq = lcm_setbacklight_cmdq,
//	.ata_check = lcm_ata_check,
	.update = lcm_update,
//	.switch_mode = lcm_switch_mode,
#if (LCM_DSI_CMD_MODE)
	.validate_roi = lcm_validate_roi,
#endif

};

static int lcm_platform_probe(struct platform_device *pdev)
{
#if 0
	unsigned int lcd_id0 = 0;
	unsigned int lcd_id1 = 0;

	pr_err("[LCM-ILI-TM]%s\n", __func__);
	if ( (g_fih_panelid & FIH_LCM_PANEL_ID_HWID_MASK) >> FIH_LCM_PANEL_ID_HWID_SHIFT != FIH_LCM_HWID_ILI_TM )
		return 0;

	pr_err("[LCM-ILI-TM] Config panel id\n");
	lcd_id0 = of_get_named_gpio(pdev->dev.of_node, "lcd_id_0", 0);
	gpio_request(lcd_id0, "FIH_PANEL_ID0");
	gpio_direction_output(lcd_id0, 0);
	gpio_set_value(lcd_id0, 1);

	lcd_id1 = of_get_named_gpio(pdev->dev.of_node, "lcd_id_1", 0);
	gpio_request(lcd_id1, "FIH_PANEL_ID1");
	gpio_direction_output(lcd_id1, 0);
	gpio_set_value(lcd_id1, 0);

	return 0;
#else
	int ret = 0;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pinctrl_default;

	pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(pinctrl)) {
		pr_err("[LCM-ILI-TM] Failed to get pinctrl\n");
		ret = PTR_ERR(pinctrl);
		return ret;
	}

	pinctrl_default = pinctrl_lookup_state(pinctrl, "panelid_default");
	if (IS_ERR(pinctrl_default)) {
		pr_err("[LCM-ILI-TM] Failed to get pinctrl state\n");
		ret = PTR_ERR(pinctrl_default);
		return ret;
	}

	pinctrl_select_state(pinctrl, pinctrl_default);

	return ret;
#endif
}

static int lcm_platform_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id lcm_platform_of_match[] = {
        {.compatible = "fih,panelid"},
        {},
};
MODULE_DEVICE_TABLE(of, lcm_platform_of_match);

static struct platform_driver lcm_driver = {
	.probe = lcm_platform_probe,
	.remove = lcm_platform_remove,
	.driver = {
		   .name = "ili9881h_tianma",
		   .owner = THIS_MODULE,
		   .of_match_table = lcm_platform_of_match,
		   },
};

static int __init lcm_panelid_init(void)
{
	pr_err("[LCM-ILI-TM]%s\n", __func__);
	if (platform_driver_register(&lcm_driver)) {
		pr_err("[LCM-ILI-TM]%s\n fail to register this driver", __func__);
		return -ENODEV;
	}

	return 0;
}

static void __exit lcm_panelid_exit(void)
{
	platform_driver_unregister(&lcm_driver);
}

late_initcall(lcm_panelid_init);
module_exit(lcm_panelid_exit);
MODULE_AUTHOR("fih");
MODULE_DESCRIPTION("LCM panel ID driver");
MODULE_LICENSE("GPL");

