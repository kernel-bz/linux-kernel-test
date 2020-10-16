// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/basic/ptr-test.c
 *  Pointer Test
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct CONFIG
{
    uint8_t		Ver;
    uint8_t		Ptype;
    uint8_t		Dsize;
    uint8_t		Fisze;
    uint8_t		Pid[3];
    uint8_t		Btn;
    uint8_t		Did[3];
    uint8_t		Dtn;
    uint8_t		Dent;
    uint8_t		User_Config[51];
    uint8_t		Tail[16];
};

struct CONFIG_TABLE
{
    uint8_t			Idx[4];
    struct CONFIG	Main;
    struct CONFIG	LorySerial;
    struct CONFIG	Serial;
    struct CONFIG	Rs485;
    struct CONFIG	Do;
    struct CONFIG	Di;
    struct CONFIG	Ai;
    struct CONFIG	Rtd;
    struct CONFIG	Ro;
    uint8_t			Tail[64];
};

struct NOTI_ENTRY
{
    uint8_t		Sid[3];
    uint8_t		Stn;
    uint8_t		Sent;
    uint8_t		Dent;
    uint8_t		Size;
    uint8_t		Temp;
};

struct NOTI_COUNT
{
    uint8_t		Rx_ratio;
    uint8_t		Tx_ratio;
    uint8_t		Over_Cnt;
    uint8_t		Tx_Cnt[2];
    uint8_t		Rx_Cnt[2];
    uint8_t		Init;
    uint8_t		Temp3[7];
    uint8_t		Ack;
};

struct LOKET
{
    uint8_t		Sid[3];
    uint8_t		Stn;
    uint8_t		Sent;
    uint8_t		Did[3];
    uint8_t		Dtn;
    uint8_t		Dent;
    uint8_t		Len;
    uint8_t		Payload[117];
};

struct PORT_TABLE
{
    uint8_t				Stn;
    struct CONFIG		*Config;
    struct NOTI_ENTRY	Entry[4];
    struct NOTI_COUNT	Count;
    uint8_t				*Data;
    uint8_t				Tail[24];
};

struct MAIN_CFG
{
    uint8_t	Sid[3];					// 13	디바이스 자신의 ID
    uint8_t	LoraSerial_Ea;			// 16
    uint8_t	Uart_Ea;				// 17
    uint8_t	Rs485_Ea;				// 18
    uint8_t	Do_Ea;					// 19
    uint8_t	Di_Ea;					// 20
    uint8_t	Ai_Ea;					// 21
    uint8_t	Rtd_Ea;					// 22
    uint8_t	Ro_Ea;					// 23
};

struct MAIN_DATA
{
    uint8_t	Save_Config;				// 128 포트 프로세서에서의 Config 정보 변경 시 메인에서 Flash 에 저장하도록 한다.
    uint8_t	Reboot;						// 129
    uint8_t FW_Update[8];				// 130
    uint8_t SW_Date[4];					// 138
    uint8_t Update_Data[68];			// 142  68 바이트  seq(3) + crc(1) + dummy(64)
    uint8_t	RunTime[4];					// 210  동작 시간	(1=100 msec)
    uint8_t	temp[10];					// 214  동작 시간	(1=100 msec)
};

struct CONFIG_TABLE		CFG;
struct PORT_TABLE 		Port_Table[10];
struct MAIN_DATA		Main_Data;
struct MAIN_CFG 		*Main_Cfg;

#define TBL					Port_Table[Table_No]
#define Main_Stn			1
#define MAIN_FW_VERSION		0x11
#define DEVICE_TYPE			4
#define MAIN_Rx_FIFO_SIZE 	128


static void _ptr_test_start(int8_t Table_No)
{
    memset ((uint8_t *)&Main_Data, 0x00, sizeof (struct MAIN_DATA));	// 0 으로 초기화
    memset ((uint8_t *)&TBL.Entry, 0x00, 32);							// 0 으로 초기화
    memset ((uint8_t *)&TBL.Count, 0x00, 16);							// 0 으로 초기화

    TBL.Data	 = (uint8_t *)&Main_Data;								// 데이타 영역 메모리 주소 매핑
    TBL.Config	 = &CFG.Main;											// 메모리 주소 매핑
    Main_Cfg 	 = (struct MAIN_CFG *)&CFG.Main.User_Config;			// USer Config 영역 포인터 매핑

    //---------------------------------------  Set Config Info
    TBL.Stn	 	 		= Main_Stn;											// Stn 은 장치가 1번
    TBL.Config->Ver 	= MAIN_FW_VERSION;
    TBL.Config->Ptype 	= DEVICE_TYPE;
    TBL.Config->Dsize 	= sizeof (struct MAIN_DATA) / 16;
    //TBL.Config->Fsize 	= MAIN_Rx_FIFO_SIZE / 16;
}

void basic_ptr_test(void)
{
    _ptr_test_start(0);
}
