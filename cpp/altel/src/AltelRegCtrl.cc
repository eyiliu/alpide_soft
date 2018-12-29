#include "JadeRegCtrl.hh"

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

#define RBCP_DISP_MODE_NO 0
#define RBCP_DISP_MODE_INTERACTIVE 1
#define RBCP_DISP_MODE_DEBUG 2

struct rbcp_header{
  unsigned char type;
  unsigned char command;
  unsigned char id;
  unsigned char length;
  unsigned int address;
};

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <thread>
#include <algorithm>
#include <sstream>

using namespace std::chrono_literals;
 
//+++++++++++++++++++++++++++++++++++++++++
//AltelRegCtrl.hh

class AltelRegCtrl: public JadeRegCtrl{
public:
  AltelRegCtrl(const JadeOption &opt);
  ~AltelRegCtrl() override {};
  void Open() override;
  void Close() override;
  std::string SendCommand(const std::string &cmd, const std::string &para) override;
  void WriteByte(uint64_t addr, uint8_t val) override;
  uint8_t ReadByte(uint64_t addr) override;
  
  void WriteAlpideRegister(uint16_t addr, uint16_t val);
  void BroadcastAlpide(uint16_t cmd);
  void SetAlpideChipID(uint8_t id);
  void SetAlpideGapLength(uint8_t len);
  void InitAlpide();
  void SetEventNumber(uint32_t num);  
  void DigitalPulse();

  int rbcp_com(const char* ipAddr, unsigned int port, struct rbcp_header* sendHeader, char* sendData, char* recvData, char dispMode);  
private:
  JadeOption m_opt;

  std::string m_ip_address;
  uint16_t m_ip_udp_port;
  uint8_t m_id ;
};

//+++++++++++++++++++++++++++++++++++++++++
//AltelRegCtrl.cc
namespace{
  auto _test_index_ = JadeUtils::SetTypeIndex(std::type_index(typeid(AltelRegCtrl)));
  auto _test_ = JadeFactory<JadeRegCtrl>::Register<AltelRegCtrl, const JadeOption&>(typeid(AltelRegCtrl));
}

AltelRegCtrl::AltelRegCtrl(const JadeOption &opt)
  :m_opt(opt), m_id(0), m_ip_udp_port(4660), JadeRegCtrl(opt){
  m_ip_address  = m_opt.GetStringValue("IP_ADDRESS");
  if(m_ip_address.empty()){
    std::cerr<<"altelregctrl: error ip address\n";
  }
  m_ip_udp_port = (uint16_t) m_opt.GetIntValue("IP_UDP_PORT");
}

void AltelRegCtrl::Open(){
  SendCommand("INIT", "");
  SendCommand("START", "");
}

void AltelRegCtrl::Close(){
  SendCommand("STOP", "");
}

void AltelRegCtrl::DigitalPulse(){
  WriteAlpideRegister(0x487,0xFFFF);
  WriteAlpideRegister(0x500,0x3);
  WriteAlpideRegister(0x4,0x0);
  WriteAlpideRegister(0x5,0x28);                
  WriteAlpideRegister(0x7,0x32);
  WriteAlpideRegister(0x8,0x3E8);
  WriteAlpideRegister(0x1,0x3D);
  WriteByte(0x20000000, 0);
}


void AltelRegCtrl::WriteByte(uint64_t addr, uint8_t val){
  m_id++;
  rbcp_header sndHeader;
  sndHeader.type=RBCP_VER;
  sndHeader.command= RBCP_CMD_WR;
  sndHeader.id=m_id;
  sndHeader.length=1;
  sndHeader.address=htonl(addr); //TODO: check
  char rcvdBuf[1024];
  rbcp_com(m_ip_address.c_str(), m_ip_udp_port, &sndHeader, (char*) &val, rcvdBuf, RBCP_DISP_MODE_NO);
  //TODO: if failure
}

uint8_t AltelRegCtrl::ReadByte(uint64_t addr){
  m_id++;
  rbcp_header sndHeader;
  sndHeader.type=RBCP_VER;
  sndHeader.command= RBCP_CMD_RD;
  sndHeader.id=m_id;
  sndHeader.length=1;
  sndHeader.address=htonl(addr);
  char rcvdBuf[1024];
  if (rbcp_com(m_ip_address.c_str(), m_ip_udp_port, &sndHeader, NULL, rcvdBuf, RBCP_DISP_MODE_NO)!=1){
    std::cout<< "here error"<<std::endl;
  }
  return rcvdBuf[0];
}


void AltelRegCtrl::WriteAlpideRegister(uint16_t addr, uint16_t val){
  uint64_t addr_base_l = 0x00000000;
  uint64_t addr_base_h = 0x00010000;
  uint8_t val_l = val & 0xff;
  uint8_t val_h = (val >> 8) & 0xff;
  WriteByte(addr_base_l+addr, val_l);
  WriteByte(addr_base_h+addr, val_h);
}

void AltelRegCtrl::BroadcastAlpide(uint16_t cmd){
  uint64_t addr_base = 0x10000000;
  WriteByte(addr_base+cmd, 0);
}

void AltelRegCtrl::SetAlpideChipID(uint8_t id){
  uint64_t addr_base = 0x40000000;
  uint8_t val = id & 0x7f;
  WriteByte(addr_base, val);
}

void AltelRegCtrl::SetAlpideGapLength(uint8_t len){
  uint64_t addr_base = 0xc0000000;
  WriteByte(addr_base+len, 0);
}

void AltelRegCtrl::SetEventNumber(uint32_t num){
  uint64_t addr_base = 0x90000000;
  WriteByte(addr_base+num, 0);
}

void AltelRegCtrl::InitAlpide(){
  SetAlpideChipID(0x10);
  BroadcastAlpide(0xD2);
  WriteAlpideRegister(0x10,0x70);
  WriteAlpideRegister(0x4,0x10);
  WriteAlpideRegister(0x5,0x28);
  WriteAlpideRegister(0x601,0x75);
  WriteAlpideRegister(0x602,0x93);
  WriteAlpideRegister(0x603,0x56);
  WriteAlpideRegister(0x604,0x32);
  WriteAlpideRegister(0x605,0xFF);
  WriteAlpideRegister(0x606,0x0);
  WriteAlpideRegister(0x607,0x39);
  WriteAlpideRegister(0x608,0x0);
  WriteAlpideRegister(0x609,0x0);
  WriteAlpideRegister(0x60A,0x0);
  WriteAlpideRegister(0x60B,0x32);
  WriteAlpideRegister(0x60C,0x40);
  WriteAlpideRegister(0x60D,0x40);
  WriteAlpideRegister(0x60E,0x32);
  WriteAlpideRegister(0x701,0x400);
  WriteAlpideRegister(0x487,0xFFFF);
  WriteAlpideRegister(0x500,0x0);
  WriteAlpideRegister(0x500,0x1);
  WriteAlpideRegister(0x1,0x3C);
  BroadcastAlpide(0x63);

  //PLL
  WriteAlpideRegister(0x14,0x008d);
  WriteAlpideRegister(0x15,0x0088);
  WriteAlpideRegister(0x14,0x0085);
  WriteAlpideRegister(0x14,0x0185);
  WriteAlpideRegister(0x14,0x0085);

  //

  //continuous mode
  WriteAlpideRegister(0x487,0xFFFF);
  WriteAlpideRegister(0x500,0x0);
  WriteAlpideRegister(0x487,0xFFFF);
  WriteAlpideRegister(0x500,0x1);
  WriteAlpideRegister(0x4,0x10);
  WriteAlpideRegister(0x5,4);   //strobe duration / 25 ns     
  WriteAlpideRegister(0x1,0x3D);
  BroadcastAlpide(0x63);

  SetAlpideGapLength(185);
  SetEventNumber(0);

}


std::string AltelRegCtrl::SendCommand(const std::string &cmd, const std::string &para){
  if(cmd=="INIT"){
    InitAlpide();
  }

  if(cmd=="START"){
    WriteByte(0xa0000000, 0);
  }

  if(cmd=="STOP"){
    WriteByte(0xb0000000, 0);
  }
  
  return "";
}

int AltelRegCtrl::rbcp_com(const char* ipAddr, unsigned int port, struct rbcp_header* sendHeader, char* sendData, char* recvData, char dispMode){

   struct sockaddr_in sitcpAddr;
   int sock;

  struct timeval timeout;
  fd_set setSelect;
  
  int sndDataLen;
  int cmdPckLen;

  char sndBuf[1024];
  int i, j = 0;
  int rcvdBytes;
  char rcvdBuf[1024];
  int numReTrans =0;

  /* Create a Socket */
  if(dispMode==RBCP_DISP_MODE_DEBUG) puts("\nCreate socket...\n");

  sock = socket(AF_INET, SOCK_DGRAM, 0);

  sitcpAddr.sin_family      = AF_INET;
  sitcpAddr.sin_port        = htons(port);
  sitcpAddr.sin_addr.s_addr = inet_addr(ipAddr);

  sndDataLen = (int)sendHeader->length;

  if(dispMode==RBCP_DISP_MODE_DEBUG) printf(" Length = %i\n",sndDataLen);

  /* Copy header data */
  memcpy(sndBuf,sendHeader, sizeof(struct rbcp_header));

  if(sendHeader->command==RBCP_CMD_WR){
    memcpy(sndBuf+sizeof(struct rbcp_header),sendData,sndDataLen);
    cmdPckLen=sndDataLen + sizeof(struct rbcp_header);
  }else{
    cmdPckLen=sizeof(struct rbcp_header);
  }


  if(dispMode==RBCP_DISP_MODE_DEBUG){
    for(i=0; i< cmdPckLen;i++){
      if(j==0) {
	printf("\t[%.3x]:%.2x ",i,(unsigned char)sndBuf[i]);
	j++;
      }else if(j==3){
	printf("%.2x\n",(unsigned char)sndBuf[i]);
	j=0;
      }else{
	printf("%.2x ",(unsigned char)sndBuf[i]);
	j++;
      }
    }
    if(j!=3) printf("\n");
  }

  /* send a packet*/

  sendto(sock, sndBuf, cmdPckLen, 0, (struct sockaddr *)&sitcpAddr, sizeof(sitcpAddr));
  if(dispMode==RBCP_DISP_MODE_DEBUG) puts("The packet have been sent!\n");

  /* Receive packets*/
  
  if(dispMode==RBCP_DISP_MODE_DEBUG) puts("\nWait to receive the ACK packet...");


  while(numReTrans<3){

    FD_ZERO(&setSelect);
    FD_SET(sock, &setSelect);

    timeout.tv_sec  = 1;
    timeout.tv_usec = 0;
 
    if(select(sock+1, &setSelect, NULL, NULL,&timeout)==0){
      /* time out */
      puts("\n***** Timeout ! *****");
      sendHeader->id++;
      memcpy(sndBuf,sendHeader, sizeof(struct rbcp_header));
      sendto(sock, sndBuf, cmdPckLen, 0, (struct sockaddr *)&sitcpAddr, sizeof(sitcpAddr));
      numReTrans++;
      FD_ZERO(&setSelect);
      FD_SET(sock, &setSelect);
    } else {
      /* receive packet */
      if(FD_ISSET(sock,&setSelect)){
	rcvdBytes=recvfrom(sock, rcvdBuf, 2048, 0, NULL, NULL);

	if(rcvdBytes<sizeof(struct rbcp_header)){
	  puts("ERROR: ACK packet is too short");
	  close(sock);
	  return -1;
	}

	if((0x0f & rcvdBuf[1])!=0x8){
	  puts("ERROR: Detected bus error");
	  close(sock);
	  return -1;
	}

	rcvdBuf[rcvdBytes]=0;

	if(RBCP_CMD_RD){
	  memcpy(recvData,rcvdBuf+sizeof(struct rbcp_header),rcvdBytes-sizeof(struct rbcp_header));
	}

	if(dispMode==RBCP_DISP_MODE_DEBUG){
	  puts("\n***** A pacekt is received ! *****.");
	  puts("Received data:");

	  j=0;

	  for(i=0; i<rcvdBytes; i++){
	    if(j==0) {
	      printf("\t[%.3x]:%.2x ",i, sendHeader,(unsigned char)rcvdBuf[i]);
	      j++;
	    }else if(j==3){
	      printf("%.2x\n",(unsigned char)rcvdBuf[i]);
	      j=0;
	    }else{
	      printf("%.2x ",(unsigned char)rcvdBuf[i]);
	      j++;
	    }
	    if(i==7) printf("\n Data:\n");
	  }

	  if(j!=3) puts(" ");
	}else if(dispMode==RBCP_DISP_MODE_INTERACTIVE){
	  if(sendHeader->command==RBCP_CMD_RD){
	    j=0;
	    puts(" ");

	    for(i=8; i<rcvdBytes; i++){
	      if(j==0) {
		printf(" [0x%.8x] %.2x ",ntohl(sendHeader->address)+i-8,(unsigned char)rcvdBuf[i]);
		j++;
	      }else if(j==7){
		printf("%.2x\n",(unsigned char)rcvdBuf[i]);
		j=0;
	      }else if(j==4){
		printf("- %.2x ",(unsigned char)rcvdBuf[i]);
		j++;
	      }else{
		printf("%.2x ",(unsigned char)rcvdBuf[i]);
		j++;
	      }
	    }
	    
	    if(j!=15) puts(" ");
	  }else{
	    printf(" 0x%x: OK\n",ntohl(sendHeader->address));
	  }
	}
	numReTrans = 4;
	close(sock);
	return(rcvdBytes);
      }
    }
  }
  close(sock);
  return -3;
}
