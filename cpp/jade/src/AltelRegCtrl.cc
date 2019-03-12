#include "JadeRegCtrl.hh"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "json11.hpp"

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

  void FirmwareWriteReg(const std::string &reg_name, uint64_t reg_val);
  uint64_t FirmwareReadReg(const std::string &reg_name);
  void FirmwareSendCommand(const std::string &cmd_name);
  void ChipWriteReg(const std::string &reg_name, uint64_t reg_val);
  uint64_t ChipReadReg(const std::string &reg_name);
  void ChipSendCommand(const std::string &cmd_name);
  void ChipSendBroadcast(const std::string &cmd_name);

  std::string SendCommand(const std::string &cmd, const std::string &para) override;

    
  void InitALPIDE();
  void StartWorking();  
  
  int rbcp_com(const char* ipAddr, unsigned int port, struct rbcp_header* sendHeader, char* sendData, char* recvData, char dispMode);  
private:
  JadeOption m_opt;
  std::unique_ptr<json11::Json>  m_reg_json;
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
  std::string reg_json_file_path = m_opt.GetStringValue("REG_JSON_FILE_PATH");
  if(reg_json_file_path.empty()){
    std::cerr<<"altelregctrl: error no reg file\n";
  }
  std::string reg_json_str = JadeUtils::LoadFileToString(reg_json_file_path);
  std::string err_str;
  m_reg_json = std::make_shared<json11::Json>(json11::Json::parse(reg_json, err_str));
  if(!err_str.empty()){
    std::cerr<<"json11: error<"<<err_str
	       <<">, unable to parse string: "<<str<<"\n";
    throw;
  }
}

void AltelRegCtrl::FirmwareWriteReg(const std::string &reg_name, uint64_t reg_val){
  auto& reg_obj = (*m_reg_json)["FIRMWARE_REG_LIST_V2"][reg_name];
  auto& addr_obj = reg_obj["address"];
  if(addr_obj.is_array()){
    for(auto &n : addr_obj.array_items()) {
      std::string name = n["name"].string_value();
      uint64_t offset = n["offset"].int_value();
      std::string mask_str = n["mask"].string_value();
      uint64_t mask = std::stoull(mask_str);
      uint64_t val =  (reg_val>>offset)&mask;
      FirmwareWriteReg(name, val);
    }
  }  
  else{
    std::string addr_str = addr_obj.string_value();
    uint64_t addr = std::stoull(addr_str);
    uint8_t val = std::static_cast<uint8_t>reg_val
    m_id++;
    rbcp_header sndHeader;
    sndHeader.type=RBCP_VER;
    sndHeader.command= RBCP_CMD_WR;
    sndHeader.id=m_id;
    sndHeader.length=1;
    sndHeader.address=htonl(addr); //TODO: check
    char rcvdBuf[1024];
    rbcp_com(m_ip_address.c_str(), m_ip_udp_port, &sndHeader, (char*) &val, rcvdBuf, RBCP_DISP_MODE_NO);
  }
  return;
}

uint64_t AltelRegCtrl::FirmwareReadReg(const std::string &reg_name){
  auto& reg_obj = (*m_reg_json)["FIRMWARE_REG_LIST_V2"][reg_name];
  auto& addr_obj = reg_obj["address"];
  uint64_t return_val = 0;
  if(addr_obj.is_array()){
    for(auto &n : addr_obj.array_items()) {
      std::string name = n["name"].string_value();
      uint64_t offset = n["offset"].int_value();
      std::string mask_str = n["mask"].string_value();
      uint64_t mask = std::stoull(mask_str);
      uint64_t val =  FirmwareReadReg(name)
      return_val += (val&mask)<<offest;
    }
  }
  else{
    std::string addr_str = addr_obj.string_value();
    uint64_t addr = std::stoull(addr_str);
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
    return_val = rcvdBuf[0];
  }
  return return_val;
}

void AltelRegCtrl::FirmwareSendCommand(const std::string &cmd_name){
  auto& cmd_obj = (*m_reg_json)["FIRMWARE_CMD_LIST_V2"][cmd_name];
  uint64_t cmd = reg_obj["value"].int_value();
  FirmwareWriteReg("FIRMWARE_CMD", cmd);
  return;
}

void AltelRegCtrl::ChipWriteReg(const std::string &reg_name, uint64_t reg_val){
  auto& reg_obj = (*m_reg_json)["CHIP_REG_LIST"][reg_name];
  auto& addr_obj = reg_obj["address"];
  if(addr_obj.is_array()){
    for(auto &n : addr_obj.array_items()) {
      std::string name = n["name"].string_value();
      uint64_t offset = n["offset"].int_value();
      std::string mask_str = n["mask"].string_value();
      uint64_t mask = std::stoull(mask_str);
      uint64_t val =  (reg_val>>offset)&mask;
      ChipWriteReg(name, val);
    }
  }
  else{
    std::string addr_str = addr_obj.string_value();
    uint64_t addr = std::stoull(addr_str);
    FirmwareWriteReg("ADDR_CHIP_REG", addr);    
    FirmwareWriteReg("DATA_WRITE", reg_val);
    FirmwareSendCommand("WRITE");
  }
  return;
}

uint64_t AltelRegCtrl::ChipReadReg(const std::string &reg_name){
  auto& reg_obj = (*m_reg_json)["CHIP_REG_LIST"][reg_name];
  auto& addr_obj = reg_obj["address"];
  uint64_t return_val = 0;
  if(addr_obj.is_array()){
    for(auto &n : addr_obj.array_items()) {
      std::string name = n["name"].string_value();
      uint64_t offset = n["offset"].int_value();
      std::string mask_str = n["mask"].string_value();
      uint64_t mask = std::stoull(mask_str);
      uint64_t val =  ChipReadReg(name)
      return_val += (val&mask)<<offest;
    }
  }
  else{
    std::string addr_str = addr_obj.string_value();
    uint64_t addr = std::stoull(addr_str);
    FirmwareWriteReg("ADDR_CHIP_REG", addr);    
    FirmwareSendCommand("READ");
    return_val = FirmwareReadReg("DATA_READ");
  }
  return return_val;
}

void AltelRegCtrl::ChipSendCommand(const std::string &cmd_name){
  auto& cmd_obj = (*m_reg_json)["CHIP_CMD_LIST"][cmd_name];
  uint64_t cmd = reg_obj["value"].int_value();
  ChipWriteReg("CHIP_CMD", cmd);
  return;
}

void AltelRegCtrl::ChipSendBroadcast(const std::string &cmd_name){
  auto& cmd_obj = (*m_reg_json)["CHIP_CMD_LIST"][cmd_name];
  uint64_t cmd = reg_obj["value"].int_value();
  FirmwareWriteReg("BROADCAST_OPCODE", cmd);
  FirewareSendCommand("BROADCAST");
  return;
}

void AltelRegCtrl::Open(){
  std::cout<<"sending INIT"<<std::endl;
  SendCommand("INIT", "");
  std::cout<<"sending START"<<std::endl;
  SendCommand("START", "");
  std::cout<<"open done"<<std::endl;
}

void AltelRegCtrl::Close(){
  SendCommand("STOP", "");
}

void AltelRegCtrl::InitALPIDE(){
  // SetFPGAMode(0);
  FirmwareWriteReg("FIRMWARE_MODE", 0x0)
  
  //ResetDAQ();

  // ChipID(0x10);
  FirmwareWriteReg("ADDR_CHIP_ID",0x10);
  
  // Broadcast(0xD2);
  ChipSendBroadcast("GRST");

  // WriteReg(0x10,0x70);
  ChipWriteReg("CMU_DMU_CONF", 0x70);

  // WriteReg(0x4,0x10);
  ChipWriteReg("FROMU_CONF_1", 0x10);

  // WriteReg(0x5,0x28);
  ChipWriteReg("FROMU_CONF_2", 0x28);

  // WriteReg(0x601,0x75);
  ChipWriteReg("VRESETP", 0x75);
    
  // WriteReg(0x602,0x93);
  ChipWriteReg("VRESETD", 0x93);

  // WriteReg(0x603,0x56);
  ChipWriteReg("VCASP", 0x56);

  // WriteReg(0x604,0x32);
  ChipWriteReg("VCASN", 0x32);

  // WriteReg(0x605,0xFF);
  ChipWriteReg("VPULSEH", 0xff);
  
  // WriteReg(0x606,0x0);
  ChipWriteReg("VPULSEL", 0x0);

  // WriteReg(0x607,0x39);
  ChipWriteReg("VCASN2", 0x39);  
  
  // WriteReg(0x608,0x0);
  ChipWriteReg("VCLIP", 0x0);

  // WriteReg(0x609,0x0);
  ChipWriteReg("VTEMP", 0x0);

  // WriteReg(0x60A,0x0);
  ChipWriteReg("IAUX2", 0x0);

  // WriteReg(0x60B,0x32);
  ChipWriteReg("IRESET", 0x32);

  // WriteReg(0x60C,0x40);
  ChipWriteReg("IDB", 0x40);

  // WriteReg(0x60D,0x40);
  ChipWriteReg("IBIAS", 0x40);

  // WriteReg(0x60E,60); //empty 0x32; 0x12 data, not full. 
  ChipWriteReg("ITHR", 60);

  // WriteReg(0x701,0x400);
  ChipWriteReg("BUFF_BYPASS", 0x400);
  
  // WriteReg(0x487,0xFFFF);
  ChipWriteReg("TODO_MASK_PULSE", 0xFFFF);
  
  // WriteReg(0x500,0x0);
  ChipWriteReg("PIX_CONF_GLOBAL", 0x0);

  // WriteReg(0x500,0x1);
  ChipWriteReg("PIX_CONF_GLOBAL", 0x1);

  // WriteReg(0x1,0x3C);
  ChipWriteReg("CHIP_MODE", 0x3c);
  
  // Broadcast(0x63);
  ChipSendBroadcast("PORST");

  // StartPLL();
  // WriteReg(0x14,0x008d);
  ChipWriteReg("DTU_CONF", 0x008d);
  // WriteReg(0x15,0x0088);
  ChipWriteReg("DTU_DAC", 0x0088);
  // WriteReg(0x14,0x0085);
  ChipWriteReg("DTU_CONF", 0x0085);
  // WriteReg(0x14,0x0185);
  ChipWriteReg("DTU_CONF", 0x0185);
  // WriteReg(0x14,0x0085);
  ChipWriteReg("DTU_CONF", 0x0085);
  //end of PLL setup
}

void AltelRegCtrl::StartWorking(){
  // WriteReg(0x487,0xFFFF);
  ChipWriteReg("TODO_MASK_PULSE", 0xFFFF);

  // WriteReg(0x500,0x0);
  ChipWriteReg("PIX_CONF_GLOBAL", 0x0);

  // WriteReg(0x487,0xFFFF);
  ChipWriteReg("TODO_MASK_PULSE", 0xFFFF);  
  
  // WriteReg(0x500,0x1);
  ChipWriteReg("PIX_CONF_GLOBAL", 0x1);

  // WriteReg(0x4,0x10);
  ChipWriteReg("FROMU_CONF_1", 0X10);

  // WriteReg(0x5,156);   //3900ns
  ChipWriteReg("FROMU_CONF_2", 156); //3900ns 
  
  // WriteReg(0x1,0x3D);
  ChipWriteReg("CHIP_MODE", 0x3D);

  // Broadcast(0x63);
  ChipSendBroadcast("PORST");

  // Broadcast(0xe4);
  ChipSendBroadcast("PRST");
  
  // SetFrameDuration(16);   //400ns
  FirmwareWriteReg("TRIG_DELAY", 16);

  // SetInTrigGap(20);
  FirmwareWriteReg("GAP_INI_TRIG", 20);
  
  // SetFPGAMode(0x1);
  FirmwareWriteReg("FIRMWARE_MODE", 0x1);
}


std::string AltelRegCtrl::SendCommand(const std::string &cmd, const std::string &para){
  if(cmd=="INIT"){
    InitALPIDE();
  }

  if(cmd=="START"){
    StartWorking();
  }

  if(cmd=="STOP"){
    // SetFPGAMode(0);
    FirmwareWriteReg("FIRMWARE_MODE", 0x0);
  }

  if(cmd=="RESET"){
    // ResetDAQ();
    FirmwareSendCommand("RESET");
  }

  return "";
}

int AltelRegCtrl::rbcp_com(const char* ipAddr, unsigned int port, struct rbcp_header* sendHeader, char* sendData, char* recvData, char dispMode){

  struct sockaddr_in sitcpAddr;
  
  int  sock;

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
