/*************************************************
*                                                *
* SiTCP Remote Bus Control Protocol              *
* Header file                                    *
*                                                *
* 2010/05/31 Tomohisa Uchida                     *
*                                                *
*************************************************/
#ifndef rbcp_H
#define rbcp_H 1

#define MAX_LINE_LENGTH 1024
#define MAX_PARAM_LENGTH 20
#define RBCP_VER 0xFF
#define RBCP_CMD_WR 0x80
#define RBCP_CMD_RD 0xC0
#define UDP_BUF_SIZE 2048
#define RBCP_DISP_MODE_NO 0
#define RBCP_DISP_MODE_INTERACTIVE 1
#define RBCP_DISP_MODE_DEBUG 2

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#include <string>

struct rbcp_header;

class rbcp{
 private:
  int argc;
  char **argv;
  const char* sitcpIpAddr;
  unsigned int sitcpPort;
  std::string ipaddr;
  
  char tempKeyBuf[MAX_LINE_LENGTH];
  char szVerb[MAX_PARAM_LENGTH];
  char szArg1[MAX_PARAM_LENGTH];
  char szArg2[MAX_PARAM_LENGTH];
  int rtnValue;

  FILE *fin;

 public:
  rbcp();
  rbcp(int c, char **v);
  rbcp(std::string ip, int udp = 4660);
  ~rbcp();
  int run();
  unsigned int myAtoi(char* str);
  int myScanf(char* inBuf, char* argBuf1, char* argBuf2,  char* argBuf3);
  int myGetArg(char* inBuf, int i, char* argBuf);
  int rbcp_com(const char* ipAddr, unsigned int port,
	       struct rbcp_header* sendHeader,
	       char* sendData, char* recvData, char dispMode);
    
  int OnHelp();
  
  int DispatchCommand(char* pszVerb,
		      char* pszArg1,
		      char* pszArg2,
		      const char* ipAddr,
		      unsigned int rbcpPort,
		      char dispMode
		      );
    
  int DispatchCommand(const char* pszVerb,
		      unsigned int addrReg,
		      unsigned int dataReg,
		      std::string *recvStr,
		      char dispMode = RBCP_DISP_MODE_INTERACTIVE
		      );

};

#endif
