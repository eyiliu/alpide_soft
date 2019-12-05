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
public:
  rbcp(std::string ip, int udp);
  int rbcp_com(const char* ipAddr, unsigned int port,
	       struct rbcp_header* sendHeader,
	       char* sendData, char* recvData, char dispMode);
  
  int DispatchCommand(const std::string &pszVerb,
		      unsigned int addrReg,
		      unsigned int dataReg,
		      std::string *recvStr,
		      char dispMode = 2
		      );

private:
  std::string  m_sitcpIpAddr {"192.168.0.16"};
  unsigned int m_sitcpPort {4660};
  unsigned char m_id {0};
};

#endif
