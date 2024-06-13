// Sample06.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../../include/Dongle_API.h"
#include <string.h> // Add this line

// #include <conio.h>

#define MAX_OUTPUT_LEN 128


int main(int argc, char *argv[])
{
	DWORD dwRet = 0;
	int nCount = 0;
	int i = 0;
	int nIndex = -1;
	int year, mon, day, hour, min, sec;
	int nRemainCount = 0;

	time_t tm;
	struct tm *ptm = NULL;
	DWORD dwTime = 0;

	DONGLE_HANDLE hDongle = NULL;
	DONGLE_INFO *pDongleInfo = NULL;

	// 枚举锁
	dwRet = Dongle_Enum(NULL, &nCount);
	printf("Enum %d Dongle ARM. \n", nCount);

	pDongleInfo = (DONGLE_INFO *)malloc(nCount * sizeof(DONGLE_INFO));

	dwRet = Dongle_Enum(pDongleInfo, &nCount);

	for (i = 0; i < nCount; i++)
	{ // 0xFF表示标准版, 0x00为时钟锁,0x01为带时钟的U盘锁,0x02为标准U盘锁
		if (pDongleInfo[i].m_Type == 0 || pDongleInfo[i].m_Type == 1)
		{
			nIndex = i;
		}
	}

	if (nIndex == -1)
	{ // 没有找到时钟锁
		printf("Can't Find Time Dongle ARM.\n");
		Dongle_Close(hDongle);
		return 0;
	}
}
