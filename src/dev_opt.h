/*
 * dev_opt.h
 *
 *  Created on: 2017年9月25日
 *      Author: lijian
 */

#ifndef SRC_DEV_OPT_H_
#define SRC_DEV_OPT_H_

#include "my_include.h"

//#define BOX_VERSION_FILE "/ipnc/config/box_version"
#define BOX_VERSION_FILE "./box_version"

//获取盒子到序列号
HB_S32 get_sys_sn(HB_CHAR *sn, HB_S32 sn_size);
//获取盒子信息
HB_S32 get_dev_info();

#endif /* SRC_DEV_OPT_H_ */
