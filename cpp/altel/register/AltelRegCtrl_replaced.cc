#include "JadeRegCtrl.hh"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>


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
  std::string SendCommand(const std::string &cmd, const std::string &para) override;
  void WriteByte(uint64_t addr, uint8_t val) override;
  uint8_t ReadByte(uint64_t addr) override;

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
  :m_opt(opt), JadeRegCtrl(opt){
  m_ip_address  = m_opt.GetStringValue("IP_ADDRESS");
  if(m_ip_address.empty()){
    std::cerr<<"altelregctrl: error ip address\n";
  }
}

void AltelRegCtrl::Open(){
  std::cout<<"sending INIT"<<std::endl;
  SendCommand("INIT", "");
  std::cout<<"sending INIT"<<std::endl;
  SendCommand("START", "");
  std::cout<<"open done"<<std::endl;
}

void AltelRegCtrl::Close(){
  SendCommand("STOP", "");
}

std::string AltelRegCtrl::SendCommand(const std::string &cmd, const std::string &para){
  if(cmd=="INIT"){
    fw->SetFirmwareRegister("FIRMWARE_MODE", 0);
    fw->SetFirmwareRegister("ADDR_CHIP_ID", 0x10);
    fw->SendAlpideBroadcast("GRST");

    fw->SetAlpideRegister("CMU_DMU_CONF", 0x70);
    fw->SetAlpideRegister("FROMU_CONF_1", 0x10);
    fw->SetAlpideRegister("FROMU_CONF_2", 0x28);
    fw->SetAlpideRegister("VRESETP", 0x75);
    fw->SetAlpideRegister("VRESETD", 0x93);
    fw->SetAlpideRegister("VCASP", 0x56);
    fw->SetAlpideRegister("VCASN", 0x32);
    fw->SetAlpideRegister("VPULSEH", 0xff);
    fw->SetAlpideRegister("VPULSEL", 0x0);
    fw->SetAlpideRegister("VCASN2", 0x39); // TODO: reset value is 0x40
    fw->SetAlpideRegister("VCLIP", 0x0);
    fw->SetAlpideRegister("VTEMP", 0x0);
    fw->SetAlpideRegister("IAUX2", 0x0);
    fw->SetAlpideRegister("IRESET", 0x32);
    fw->SetAlpideRegister("IDB", 0x40);
    fw->SetAlpideRegister("IBIAS", 0x40);
    fw->SetAlpideRegister("ITHR", 0x32); //empty 0x32; 0x12 data, not full.
    fw->SetAlpideRegister("TEST_CTRL", 0x400); // ?
  
    fw->SetAlpideRegister("TODO_MASK_PULSE", 0xffff);
    fw->SetAlpideRegister("PIX_CONF_GLOBAL", 0x0);
    fw->SetAlpideRegister("PIX_CONF_GLOBAL", 0x1);
    fw->SetAlpideRegister("CHIP_MODE", 0x3c);
    fw->SendAlpideBroadcast("RORST");

    //PLL part
    fw->SetAlpideRegister("DTU_CONF", 0x008d);
    fw->SetAlpideRegister("DTU_DAC",  0x0088);
    fw->SetAlpideRegister("DTU_CONF", 0x0085);
    fw->SetAlpideRegister("DTU_CONF", 0x0185);
    fw->SetAlpideRegister("DTU_CONF", 0x0085);
  }

  if(cmd=="START"){
    fw->SetAlpideRegister("TODO_MASK_PULSE", 0xffff);
    fw->SetAlpideRegister("PIX_CONF_GLOBAL", 0x0);
    fw->SetAlpideRegister("TODO_MASK_PULSE", 0xffff);
    fw->SetAlpideRegister("PIX_CONF_GLOBAL", 0x1);
    fw->SetAlpideRegister("FROMU_CONF_1", 0x10);
    fw->SetAlpideRegister("FROMU_CONF_2", 156); //25ns per dig
    fw->SetAlpideRegister("CHIP_MODE", 0x3d);
    fw->SendAlpideBroadcast("RORST");
    fw->SendAlpideBroadcast("PRST");
    fw->SetFirmwareRegister("TRIG_DELAY", 100); //25ns per dig (FrameDuration?)
    fw->SetFirmwareRegister("GAP_INT_TRIG", 20);
    fw->SetFirmwareRegister("FIRMWARE_MODE", 1); //run ext trigger
  }

  if(cmd=="STOP"){
    fw->SetFirmwareRegister("FIRMWARE_MODE", 0);
  }

  if(cmd=="RESET"){
    fw->SendFirmwareCommand("RESET")  
  }

  return "";
}
