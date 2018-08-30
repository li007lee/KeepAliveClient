/*
 * common.h
 *
 *  Created on: 2018年5月10日
 *      Author: root
 */

#ifndef INC_COMMON_H_
#define INC_COMMON_H_

#include "my_include.h"
#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/bufferevent.h"
#include "event2/listener.h"
#include "event2/thread.h"

#define ETH_X	"ens33"
#define MASTER_SERVER_IP "192.168.118.56"
#define MASTER_SERVER_PORT	(9502)

#define SLAVE_SERVER_KEEP_ALAVE_PORT	(9001)
//#define SLAVE_SERVER_KEEP_ALAVE_PORT	(19001)
#define RECONNET_TO_SERVER_INTERVAL (30)  //连接心跳服务器失败后重连时间间隔
#define HEARTBATE_TIME_VAL (30)  //心跳包发送时间间隔

typedef struct _gl_param
{
	HB_CHAR cDevModel[32]; //盒子型号
	HB_CHAR cDevVer[16]; //盒子版本号
	HB_CHAR cDevId[128];	//盒子序列号
	struct event_base *pEventBase;
//	struct bufferevent *bevConnectToKeepAliveServer;//与心跳服务器连接的事件
	struct event *evHeartBeatTimer;	//心跳定时器
	HB_CHAR cKeepAliveServerIp[16]; //长连接服务器ip
	HB_S32	iKeepAliveServerPort;	//长连接服务器端口
}GLOBLE_PARAM_OBJ;

extern GLOBLE_PARAM_OBJ glParam;

#endif /* SRC_COMMON_H_ */
