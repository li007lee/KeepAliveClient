/*
 * dev_opt.c
 *
 *  Created on: 2017年9月25日
 *      Author: lijian
 */

#include "common_args.h"
#include "dev_opt.h"

//根据网卡eth获取相应到mac地址
static HB_S32 get_mac_dev(HB_CHAR *mac_sn, HB_CHAR *dev)
{
	struct ifreq tmp;
	HB_S32 sock_mac;
	// HB_CHAR *tmpflag;
	//HB_CHAR mac_addr[30];
	sock_mac = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_mac == -1)
	{
		return -1;
	}
	memset(&tmp, 0, sizeof(tmp));
	strncpy(tmp.ifr_name, dev, sizeof(tmp.ifr_name) - 1);
	if ((ioctl(sock_mac, SIOCGIFHWADDR, &tmp)) < 0)
	{
		close(sock_mac);
		return -1;
	}

	close(sock_mac);
	memcpy(mac_sn, tmp.ifr_hwaddr.sa_data, 6);
	return 0;
}

//获取盒子到序列号
HB_S32 get_sys_sn(HB_CHAR *sn, HB_S32 sn_size)
{
	HB_U64 sn_num = 0;
	HB_CHAR sn_mac[32] = { 0 };
	HB_CHAR mac[32] = { 0 };
	get_mac_dev(mac, ETH_X);
	sprintf(sn_mac, "0x%02x%02x%02x%02x%02x%02x", (HB_U8) mac[0], (HB_U8) mac[1], (HB_U8) mac[2], (HB_U8) mac[3], (HB_U8) mac[4], (HB_U8) mac[5]);
	sn_num = strtoull(sn_mac, 0, 16);
	snprintf(sn, sn_size, "%llu", sn_num);

	return 0;
}



//获取盒子信息
HB_S32 get_dev_info()
{
	FILE *pFileFp = fopen(BOX_VERSION_FILE, "r");
	if (NULL != pFileFp)
	{
		HB_CHAR *ps1 = NULL;
		fgets(glParam.cDevModel, 32, pFileFp);//盒子型号
		fgets(glParam.cDevVer, 32, pFileFp); //盒子版本
		if ((ps1=strchr((const HB_CHAR *)glParam.cDevVer,'\r')) != NULL)
		{
			*ps1 = '\0';
		}
		else if ((ps1=strchr((const HB_CHAR *)glParam.cDevVer,'\n')) != NULL)
		{
			*ps1 = '\0';
		}
		fclose(pFileFp);
	}

	get_sys_sn(glParam.cDevId, sizeof(glParam.cDevId));

	return 0;
}
