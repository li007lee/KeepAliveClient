/*
 * keepalive.h
 *
 *  Created on: Jul 16, 2018
 *      Author: root
 */

#ifndef SRC_KEEPALIVE_H_
#define SRC_KEEPALIVE_H_

typedef enum _tagCMD_FUNC
{
	/*box和Slave之间的命令*/
	E_BOX_REGIST_ALIVE_SERVER=200,     //盒子注册到长连接服务器命令
	E_BOX_REGIST_ALIVE_SERVER_REPLY,   //盒子注册到长连接服务器命令的回复
	E_BOX_KEEP_ALIVE,                  //盒子心跳命令
	E_BOX_KEEP_ALIVE_REPLY,             //盒子心跳命令的回复


	/*Master和Slave之间的命令*/
	E_MASTER_SLAVE_REGIST,
	E_MASTER_SLAVE_REGIST_REPLY,
	E_MASTER_SLAVE_KEEP_ALIVE,
	E_MASTER_SLAVE_KEEP_ALIVE_REPLY,
	E_MASTER_SLAVE_DEV_ADD,
	E_MASTER_SLAVE_DEV_DEL,


	/*Master和Dev之间的命令*/
	E_MASTER_DEV_GIVE_ME_SLAVE,
	E_MASTER_DEV_GIVE_ME_SLAVE_REPLY,

	/*通用消息命令*/
	COMMOM_MSG_TYPE

}CMD_FUNC_E;


typedef enum _tagCMD_FORMAT
{
	E_CMD_JSON=100,   //json消息
	E_CMD_XML,//xml消息
}CMD_FORMAT_E;
//命令头定义
typedef struct _tagCMD_HEADER
{
	HB_CHAR          header[16]; //固定为"BoxKeepAlive@yDt"
	HB_U32           cmd_length; //头消息后面的数据长度
	CMD_FORMAT_E     cmd_format; //消息类型，目前只支持json和xml消息
	CMD_FUNC_E       cmd_func;   //信令功能
}CMD_HEADER_OBJ, *CMD_HEADER_HANDLE;

HB_VOID keepalive_start();

#endif /* SRC_KEEPALIVE_H_ */
