#ifndef _RBCP_COM_H_
#define _RBCP_COM_H_

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 1024
#define MAX_PARAM_LENGTH 20
#define RBCP_VER 0xFF
#define RBCP_CMD_WR 0x80
#define RBCP_CMD_RD 0xC0
#define DEFAULT_IP 192.168.0.16
#define UDP_BUF_SIZE 2048

struct rbcp_header{
  unsigned char type;
  unsigned char command;
  unsigned char id;
  unsigned char length;
  unsigned int address;
};

int rbcp_com(char* ipAddr, unsigned int port, struct rbcp_header* sndHeader, char* sndData, char* readData, char dispMode);

int DispatchCommand(char* pszVerb,
		    char* pszArg1,
		    char* pszArg2,
		    char* ipAddr,
		    unsigned int rbcpPort,
		    struct rbcp_header* sndHeader,
		    char dispMode
		    );

int myScanf(char* inBuf, char* argBuf1, char* argBuf2,  char* argBuf3);
int myGetArg(char* inBuf, int i, char* argBuf);
unsigned int myAtoi(char* str);
int OnHelp();

#endif
