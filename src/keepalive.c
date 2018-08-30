/*
 * keepalive.c
 *
 *  Created on: Jul 16, 2018
 *      Author: root
 */

#include "common_args.h"
#include "cJSON.h"
#include "keepalive.h"
#include "event2/event_struct.h"

struct event evConnectToMasterServer;
struct event evConnectToKeepAliveServer;


HB_VOID recv_msg_from_keepalive_server_cb(struct bufferevent *pbevConnectToKeepAliveServer, HB_HANDLE hArg);
static HB_VOID connect_to_keepalive_server(evutil_socket_t fd, short events, void *arg);
HB_VOID connect_to_master_server(evutil_socket_t fd, HB_S16 events, HB_HANDLE hArg);


/************************************************************/
/*********************连接长连接服务器slave********************/
/************************************************************/

HB_VOID connect_to_keepalive_server_cb(struct bufferevent *pbevConnectToKeepAliveServer, HB_S16 what, HB_HANDLE hArg)
{
	static HB_S32 iConnectToKeepAliveServerErrTimes = 0;
	if (what & BEV_EVENT_CONNECTED)
	{
		iConnectToKeepAliveServerErrTimes = 0;
		HB_CHAR cSendBuf[512] = {0};
		CMD_HEADER_OBJ stCmdHeader;
		memset(&stCmdHeader, 0, sizeof(CMD_HEADER_OBJ));
		stCmdHeader.cmd_format = E_CMD_JSON;
		stCmdHeader.cmd_func = E_BOX_REGIST_ALIVE_SERVER;
		strncpy(stCmdHeader.header, "BoxKeepAlive@yDt", 16);

//		snprintf(cSendBuf, sizeof(cSendBuf)-sizeof(CMD_HEADER_OBJ), "\"DevType\":\"ydt_box\",\"DevModel\":\"TM-X01-A\",\"DevId\":\"251227033271384\",\"DevVer\":\"3.0.0.1\"", );
		snprintf(cSendBuf+sizeof(CMD_HEADER_OBJ), sizeof(cSendBuf)-sizeof(CMD_HEADER_OBJ), "{\"DevType\":\"ydt_box\",\"DevModel\":\"%s\",\"DevId\":\"%s\",\"DevVer\":\"%s\"}",\
						glParam.cDevModel, glParam.cDevId, glParam.cDevVer);

		stCmdHeader.cmd_length = strlen(cSendBuf+sizeof(CMD_HEADER_OBJ));
		memcpy(cSendBuf, &stCmdHeader, sizeof(CMD_HEADER_OBJ));

		//连接成功发送注册消息
		bufferevent_write(pbevConnectToKeepAliveServer, cSendBuf, sizeof(stCmdHeader)+stCmdHeader.cmd_length);
	}
	else
	{
		bufferevent_free(pbevConnectToKeepAliveServer);
		pbevConnectToKeepAliveServer = NULL;

		++iConnectToKeepAliveServerErrTimes;
		if(iConnectToKeepAliveServerErrTimes >= 3)
		{
			iConnectToKeepAliveServerErrTimes = 0;
			TRACE_ERR("Can't connect to keep alive server, get new keep alive server...\n");
			struct timeval tv = {0, 0};
			event_assign(&evConnectToMasterServer, glParam.pEventBase, -1, 0, connect_to_master_server, NULL);
			event_add(&evConnectToMasterServer, &tv);
		}
		else
		{
			printf("connect to keep alive server failed! reconnect !\n");
			struct timeval tv = {RECONNET_TO_SERVER_INTERVAL, 0};
			event_assign(&evConnectToKeepAliveServer, glParam.pEventBase, -1, 0, connect_to_keepalive_server, NULL);
			event_add(&evConnectToKeepAliveServer, &tv);
		}
	}
}

static HB_VOID connect_to_keepalive_server(evutil_socket_t fd, short events, void *arg)
{
	struct timeval tvConnectTimeOut = {5, 0}; //5s连接超时
	struct sockaddr_in stConnectToKeepAliveServerAddr;
	struct bufferevent *pbevConnectToKeepAliveServer = bufferevent_socket_new(glParam.pEventBase, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE);

	bzero(&stConnectToKeepAliveServerAddr, sizeof(stConnectToKeepAliveServerAddr));
	stConnectToKeepAliveServerAddr.sin_family = AF_INET;
	stConnectToKeepAliveServerAddr.sin_port = htons(glParam.iKeepAliveServerPort);
	inet_pton(AF_INET, glParam.cKeepAliveServerIp, (void *) &stConnectToKeepAliveServerAddr.sin_addr);
	printf("connect to slave ServerIp[%s], ServerPort[%d]\n", glParam.cKeepAliveServerIp, glParam.iKeepAliveServerPort);

	bufferevent_set_timeouts(pbevConnectToKeepAliveServer, NULL, &tvConnectTimeOut);
	bufferevent_setcb(pbevConnectToKeepAliveServer, recv_msg_from_keepalive_server_cb, NULL, connect_to_keepalive_server_cb, pbevConnectToKeepAliveServer);
	bufferevent_socket_connect(pbevConnectToKeepAliveServer, (struct sockaddr*) &stConnectToKeepAliveServerAddr, sizeof(struct sockaddr_in));
	bufferevent_enable(pbevConnectToKeepAliveServer, EV_READ | EV_WRITE);
}

static HB_VOID send_heartbate_err_cb(struct bufferevent *pbevConnectToKeepAliveServer, HB_S16 what, HB_HANDLE hArg)
{
	HB_S32 err = EVUTIL_SOCKET_ERROR();

	if (what & BEV_EVENT_TIMEOUT)//超时
	{
		TRACE_ERR("\n###########  send_heartbate_err_cb(%d) : send frame  timeout(%s) !\n", err, evutil_socket_error_to_string(err));
	}
	else if (what & BEV_EVENT_ERROR)  //错误
	{
		TRACE_ERR("\n###########  send_heartbate_err_cb(%d) : send frame failed(%s) !\n", err, evutil_socket_error_to_string(err));
	}
	else if (what & BEV_EVENT_EOF) //对端主动关闭
	{
		TRACE_YELLOW("\n###########  send_heartbate_err_cb(%d) : connection closed by peer(%s)!\n", err, evutil_socket_error_to_string(err));
	}
	else
	{
		TRACE_ERR("\n###########  send_heartbate_err_cb(%d) : (%s) !\n", err, evutil_socket_error_to_string(err));
	}

	event_del(glParam.evHeartBeatTimer);
	event_free(glParam.evHeartBeatTimer);
	glParam.evHeartBeatTimer = NULL;
	bufferevent_free(pbevConnectToKeepAliveServer);
	pbevConnectToKeepAliveServer = NULL;
	struct timeval tv = {RECONNET_TO_SERVER_INTERVAL, 0};
	event_assign(&evConnectToKeepAliveServer, glParam.pEventBase, -1, 0, connect_to_keepalive_server, NULL);
	event_add(&evConnectToKeepAliveServer, &tv);
}

static HB_VOID send_heartbeat(evutil_socket_t fd, short events, HB_HANDLE hArg)
{
	printf("curtain time : %ld.\n", time(NULL));
	struct bufferevent *pbevConnectToKeepAliveServer = (struct bufferevent *)hArg;
	HB_CHAR cSendBuf[512] = {0};
	CMD_HEADER_OBJ stCmdHeader;
	memset(&stCmdHeader, 0, sizeof(CMD_HEADER_OBJ));
	stCmdHeader.cmd_format = E_CMD_JSON;
	stCmdHeader.cmd_func = E_BOX_KEEP_ALIVE;
	strncpy(stCmdHeader.header, "BoxKeepAlive@yDt", 16);

	snprintf(cSendBuf+sizeof(CMD_HEADER_OBJ), sizeof(cSendBuf)-sizeof(CMD_HEADER_OBJ), "{\"HeartBeat\":\"keep_alive\",\"DevId\":\"%s\"}", glParam.cDevId);

	stCmdHeader.cmd_length = strlen(cSendBuf+sizeof(CMD_HEADER_OBJ));
	memcpy(cSendBuf, &stCmdHeader, sizeof(CMD_HEADER_OBJ));

	struct timeval tvRecvTimeOut = {5, 0}; //设置读超时
	bufferevent_setcb(pbevConnectToKeepAliveServer, recv_msg_from_keepalive_server_cb, NULL, send_heartbate_err_cb, pbevConnectToKeepAliveServer);
	bufferevent_write(pbevConnectToKeepAliveServer, cSendBuf, sizeof(stCmdHeader)+stCmdHeader.cmd_length);
	bufferevent_enable(pbevConnectToKeepAliveServer, EV_READ | EV_WRITE);
	bufferevent_set_timeouts(pbevConnectToKeepAliveServer, &tvRecvTimeOut, NULL);
}

//用于接收注册成功或失败的结果信息
HB_VOID recv_msg_from_keepalive_server_cb(struct bufferevent *pbevConnectToKeepAliveServer, HB_HANDLE hArg)
{
	CMD_HEADER_OBJ stCmdHeader;
	HB_CHAR cRecvBuf[1024] = { 0 };

	for(;;)
	{
		struct evbuffer *src = bufferevent_get_input(pbevConnectToKeepAliveServer);//获取输入缓冲区
		HB_S32 len = evbuffer_get_length(src);//获取输入缓冲区中数据的长度，也就是可以读取的长度。

		if(len < sizeof(CMD_HEADER_OBJ))
		{
//			printf("data small than head len! continue ...\n");
			return;
		}

		evbuffer_copyout(src, (void*)(&stCmdHeader), sizeof(CMD_HEADER_OBJ));
		if (strncmp(stCmdHeader.header, "BoxKeepAlive@yDt", 16) != 0)
		{
			//头消息错误，直接返回
			TRACE_ERR("st_MsgHead.header error recv:[%s]\n", stCmdHeader.header);
			return ;
		}

		if(evbuffer_get_length(src) < (stCmdHeader.cmd_length + sizeof(CMD_HEADER_OBJ)))
		{
			printf("\n2222222222recv len=%d   msg_len=%d\n", (HB_S32)evbuffer_get_length(src), (HB_S32)(stCmdHeader.cmd_length));
			return;
		}
		bufferevent_read(pbevConnectToKeepAliveServer, (HB_VOID*)(cRecvBuf), (stCmdHeader.cmd_length + sizeof(CMD_HEADER_OBJ)));

		switch(stCmdHeader.cmd_func)
		{
			case E_BOX_REGIST_ALIVE_SERVER_REPLY:
			{
				cJSON *pRoot;
				pRoot = cJSON_Parse(cRecvBuf+sizeof(CMD_HEADER_OBJ));
				cJSON *pResult = cJSON_GetObjectItem(pRoot, "Result");
				if (pResult->valueint != 0)
				{
					cJSON *pMsg = cJSON_GetObjectItem(pRoot, "Msg");
					TRACE_ERR("E_BOX_REGIST_ALIVE_SERVER_REPLY regist failed : [%s]\n", pMsg->valuestring);
					cJSON_Delete(pRoot);
					bufferevent_free(pbevConnectToKeepAliveServer);
					pbevConnectToKeepAliveServer = NULL;
					struct timeval tv = {RECONNET_TO_SERVER_INTERVAL, 0};
					event_assign(&evConnectToKeepAliveServer, glParam.pEventBase, -1, 0, connect_to_keepalive_server, NULL);
					event_add(&evConnectToKeepAliveServer, &tv);
					return ;
				}
				cJSON_Delete(pRoot);
				//注册成功，创建定时器，定时发送心跳包
				struct timeval tv = {HEARTBATE_TIME_VAL, 0};
				printf("E_BOX_REGIST_ALIVE_SERVER_REPLY recv reponse :[%s]\n", cRecvBuf+sizeof(CMD_HEADER_OBJ));
				glParam.evHeartBeatTimer = event_new(glParam.pEventBase, -1, EV_READ | EV_PERSIST, send_heartbeat, (HB_HANDLE)pbevConnectToKeepAliveServer);
				event_add(glParam.evHeartBeatTimer, &tv);
			}
			break;
			case E_BOX_KEEP_ALIVE_REPLY:
			{
				cJSON *pRoot;
				pRoot = cJSON_Parse(cRecvBuf+sizeof(CMD_HEADER_OBJ));
				cJSON *pResult = cJSON_GetObjectItem(pRoot, "Result");
				if (pResult->valueint != 0)
				{
					cJSON *pMsg = cJSON_GetObjectItem(pRoot, "Msg");
					TRACE_ERR("E_BOX_KEEP_ALIVE_REPLY recv heartbeat failed : [%s]\n", pMsg->valuestring);
					cJSON_Delete(pRoot);
					return;
				}
				cJSON_Delete(pRoot);
				printf("E_BOX_KEEP_ALIVE_REPLY recv reponse :[%s]\n", cRecvBuf+sizeof(CMD_HEADER_OBJ));
				bufferevent_set_timeouts(pbevConnectToKeepAliveServer, NULL, NULL);
			}
			break;
			default:
				TRACE_DBG("recv_msg_from_keepalive_server_cb default recv reponse :[%s]\n", cRecvBuf+sizeof(CMD_HEADER_OBJ));
				break;
		}
	}
}

/************************************************************/
/*********************连接长连接服务器slave********************/
/************************************************************/


/************************************************************/
/*********************连接调度服务器master*********************/
/************************************************************/

//用于接收注册成功或失败的结果信息
static HB_VOID recv_msg_from_master_server_cb(struct bufferevent *pbevConnectToMasterServer, HB_HANDLE hArg)
{
	CMD_HEADER_OBJ stCmdHeader;
	HB_CHAR cRecvBuf[1024] = { 0 };

#if 1
	struct evbuffer *src = bufferevent_get_input(pbevConnectToMasterServer);//获取输入缓冲区
	HB_S32 len = evbuffer_get_length(src);//获取输入缓冲区中数据的长度，也就是可以读取的长度。

	if(len < sizeof(CMD_HEADER_OBJ))
	{
		printf("data small than head len! continue ...\n");
		return;
	}

	evbuffer_copyout(src, (void*)(&stCmdHeader), sizeof(CMD_HEADER_OBJ));
	if (strncmp(stCmdHeader.header, "BoxKeepAlive@yDt", 16) != 0)
	{
		//头消息错误，直接返回
		TRACE_ERR("st_MsgHead.header error recv:[%s]\n", stCmdHeader.header);
		return ;
	}

	if(evbuffer_get_length(src) < (stCmdHeader.cmd_length + sizeof(CMD_HEADER_OBJ)))
	{
		printf("\n2222222222recv len=%d   msg_len=%d\n", (HB_S32)evbuffer_get_length(src), (HB_S32)(stCmdHeader.cmd_length));
		return;
	}
	bufferevent_read(pbevConnectToMasterServer, (HB_VOID*)(cRecvBuf), (stCmdHeader.cmd_length + sizeof(CMD_HEADER_OBJ)));

	bufferevent_free(pbevConnectToMasterServer);
	pbevConnectToMasterServer = NULL;

	switch(stCmdHeader.cmd_func)
	{
		case E_MASTER_DEV_GIVE_ME_SLAVE_REPLY:
		{
			cJSON *pRoot;
			pRoot = cJSON_Parse(cRecvBuf+sizeof(CMD_HEADER_OBJ));
			cJSON *pSlaveIp = cJSON_GetObjectItem(pRoot, "SlaveIp");
			if (NULL != pSlaveIp)
			{
				memset(glParam.cKeepAliveServerIp, 0, sizeof(glParam.cKeepAliveServerIp));
				strncpy(glParam.cKeepAliveServerIp, pSlaveIp->valuestring, sizeof(glParam.cKeepAliveServerIp));
			}
			else
			{
				cJSON_Delete(pRoot);
				TRACE_ERR("E_MASTER_DEV_GIVE_ME_SLAVE_REPLY failed!\n");
				struct timeval tv = {RECONNET_TO_SERVER_INTERVAL, 0};
				event_assign(&evConnectToMasterServer, glParam.pEventBase, -1, 0, connect_to_master_server, NULL);
				event_add(&evConnectToMasterServer, &tv);
				break;
			}

			cJSON_Delete(pRoot);
			printf("event_del(&evConnectToMasterServer):%d\n", event_del(&evConnectToMasterServer));
			printf("E_MASTER_DEV_GIVE_ME_SLAVE_REPLY recv reponse :[%s]\n", cRecvBuf+sizeof(CMD_HEADER_OBJ));
			glParam.iKeepAliveServerPort = SLAVE_SERVER_KEEP_ALAVE_PORT;
			struct timeval tv = {0, 0};
			event_assign(&evConnectToKeepAliveServer, glParam.pEventBase, -1, 0, connect_to_keepalive_server, NULL);
			event_add(&evConnectToKeepAliveServer, &tv);
		}
		break;
		default:
			printf("default recv reponse :[%s]\n", cRecvBuf+sizeof(CMD_HEADER_OBJ));
			struct timeval tv = {RECONNET_TO_SERVER_INTERVAL, 0};
			event_assign(&evConnectToMasterServer, glParam.pEventBase, -1, 0, connect_to_master_server, NULL);
			event_add(&evConnectToMasterServer, &tv);
			break;
	}
#endif
}

static HB_VOID connect_to_master_server_cb(struct bufferevent *pbevConnectToKeepAliveServer, HB_S16 what, HB_HANDLE hArg)
{
	if (what & BEV_EVENT_CONNECTED)
	{
		HB_CHAR cSendBuf[512] = {0};
		CMD_HEADER_OBJ stCmdHeader;
		memset(&stCmdHeader, 0, sizeof(CMD_HEADER_OBJ));
		stCmdHeader.cmd_format = E_CMD_JSON;
		stCmdHeader.cmd_func = E_MASTER_DEV_GIVE_ME_SLAVE;
		strncpy(stCmdHeader.header, "BoxKeepAlive@yDt", 16);

		snprintf(cSendBuf+sizeof(CMD_HEADER_OBJ), sizeof(cSendBuf)-sizeof(CMD_HEADER_OBJ), "{\"DevId\":\"%s\"}", glParam.cDevId);

		stCmdHeader.cmd_length = strlen(cSendBuf+sizeof(CMD_HEADER_OBJ));
		memcpy(cSendBuf, &stCmdHeader, sizeof(CMD_HEADER_OBJ));

		//连接成功发送注册消息
		bufferevent_write(pbevConnectToKeepAliveServer, cSendBuf, sizeof(stCmdHeader)+stCmdHeader.cmd_length);
	}
	else
	{
		bufferevent_free(pbevConnectToKeepAliveServer);
		pbevConnectToKeepAliveServer = NULL;
		printf("connect to master server failed! reconnect !\n");
		struct timeval tv = {RECONNET_TO_SERVER_INTERVAL, 0};
		event_assign(&evConnectToMasterServer, glParam.pEventBase, -1, 0, connect_to_master_server, NULL);
		event_add(&evConnectToMasterServer, &tv);
	}
}

HB_VOID connect_to_master_server(evutil_socket_t fd, HB_S16 events, HB_HANDLE hArg)
{
	struct timeval tvConnectTimeOut = {5, 0}; //5s连接超时
	struct sockaddr_in stConnectToMasterServerAddr;
	struct bufferevent *pbevConnectToMasterServer = bufferevent_socket_new(glParam.pEventBase, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE);

	bzero(&stConnectToMasterServerAddr, sizeof(stConnectToMasterServerAddr));
	stConnectToMasterServerAddr.sin_family = AF_INET;
	stConnectToMasterServerAddr.sin_port = htons(MASTER_SERVER_PORT);
	inet_pton(AF_INET, MASTER_SERVER_IP, (void *) &stConnectToMasterServerAddr.sin_addr);
	printf("############ Connect to master ServerIp[%s], ServerPort[%d]\n", MASTER_SERVER_IP, MASTER_SERVER_PORT);

	bufferevent_set_timeouts(pbevConnectToMasterServer, NULL, &tvConnectTimeOut);
	bufferevent_setcb(pbevConnectToMasterServer, recv_msg_from_master_server_cb, NULL, connect_to_master_server_cb, pbevConnectToMasterServer);
	bufferevent_socket_connect(pbevConnectToMasterServer, (struct sockaddr*) &stConnectToMasterServerAddr, sizeof(struct sockaddr_in));
	bufferevent_enable(pbevConnectToMasterServer, EV_READ | EV_WRITE);
}

/************************************************************/
/*********************连接调度服务器master*********************/
/************************************************************/




HB_VOID keepalive_start()
{
	//首先连接Master服务器，获取长连接服务器的地址和端口
	struct timeval tv = {0, 0};
	event_assign(&evConnectToMasterServer, glParam.pEventBase, -1, 0, connect_to_master_server, NULL);
	event_add(&evConnectToMasterServer, &tv);
}


