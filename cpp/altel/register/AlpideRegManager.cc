#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <ctime>
#include <regex>
#include <map>
#include <vector>
#include <utility>
#include <algorithm>

#include "mysystem.hh"
#include "rbcp.h"

template<typename ... Args>
static std::string FormatString( const std::string& format, Args ... args ){
  std::size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1;
  std::unique_ptr<char[]> buf( new char[ size ] ); 
  std::snprintf( buf.get(), size, format.c_str(), args ... );
  return std::string( buf.get(), buf.get() + size - 1 );
}

template<typename ... Args>
static std::size_t FormatPrint(std::ostream &os, const std::string& format, Args ... args ){
  std::size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1;
  std::unique_ptr<char[]> buf( new char[ size ] ); 
  std::snprintf( buf.get(), size, format.c_str(), args ... );
  std::string formated_string( buf.get(), buf.get() + size - 1 );
  os<<formated_string<<std::flush;
  return formated_string.size();
}

std::string LoadFileToString(const std::string& path){
  std::ifstream ifs(path);
  if(!ifs.good()){
    std::cerr<<"LoadFileToString:: ERROR, unable to load file<"<<path<<">\n";
    throw;
  }
  std::string str;
  str.assign((std::istreambuf_iterator<char>(ifs) ),
             (std::istreambuf_iterator<char>()));
  return str;
}

using namespace rapidjson;
template<typename T>
const std::string Stringify(const T& o){
  rapidjson::StringBuffer sb;
  rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
  o.Accept(writer);
  return sb.GetString();
}

template<typename T>
void PrintJson(const T& o){
  rapidjson::StringBuffer sb;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> w(sb);
  o.Accept(w);
  std::fwrite(sb.GetString(), 1, sb.GetSize(), stdout);
}

static rapidjson::Document s_json;

int rbcp_write(const std::string& ip, uint16_t udp_port, uint32_t reg_addr, uint8_t reg_value);
int rbcp_read(const std::string& ip, uint16_t udp_port, uint32_t reg_addr, uint8_t& reg_value);

void  WriteByte(uint64_t address, uint64_t value){
  FormatPrint(std::cout, "WriteByte( address=%#016x ,  value=%#016x )\n", address, value);
  // rbcp_write("131.169.133.173", 4660, static_cast<uint32_t>(address), static_cast<uint8_t>(value));
  rbcp r("131.169.133.173",4660);
  std::string recvStr(100, 0);
  r.DispatchCommand("wrb",  address, value, NULL, 2);
};

uint64_t ReadByte(uint64_t address){
  FormatPrint(std::cout, "ReadByte( address=%#016x)\n", address);
  uint8_t reg_value;
  // rbcp_read("131.169.133.173", 4660, static_cast<uint32_t>(address), reg_value);
  rbcp r("131.169.133.173",4660);
  std::string recvStr(100, 0);
  r.DispatchCommand("rd", address, 1, &recvStr, 2); 
  reg_value=recvStr[0];
  FormatPrint(std::cout, "ReadByte( address=%#016x) return value=%#016x\n", address, reg_value);
  return reg_value;
};

void SetFirmwareRegister(const std::string& name, uint64_t value);
void SetAlpideRegister(const std::string& name, uint64_t value);
void SendFirmwareCommand(const std::string& name);
void SendAlpideCommand(const std::string& name);
void SendAlpideBroadcast(const std::string& name);
uint64_t GetFirmwareRegister(const std::string& name);
uint64_t GetAlpideRegister(const std::string& name);


void SetFirmwareRegister(const std::string& name, uint64_t value){
  FormatPrint(std::cout, "INFO<%s>: %s( name=%s ,  value=%#016x )\n", __func__, __func__, name.c_str(), value);
  static const std::string array_name("FIRMWARE_REG_LIST_V2");
  auto& json_array = s_json[array_name];
  if(json_array.Empty()){
    FormatPrint(std::cerr, "ERROR<%s>:   unable to find array<%s>\n", __func__, array_name.c_str());
    throw;
  }
  bool flag_found_reg = false;
  for(auto& json_reg: json_array.GetArray()){
    if( json_reg["name"] != name )
      continue;
    auto& json_addr = json_reg["address"];
    if(json_addr.IsString()){
      uint64_t address = std::stoull(json_reg["address"].GetString(), 0, 16);
      WriteByte(address, value); //word = byte
    }
    else if(json_addr.IsArray()){
      auto& json_bytes = json_reg["bytes"];
      auto& json_words = json_reg["words"];
      std::cout<< ">>>"<<Stringify(json_bytes)<<std::endl;
      PrintJson(json_bytes);
      if(!json_bytes.IsUint64()){
	FormatPrint(std::cerr, "ERROR<%s>:   bytes<%s> is not an int\n", __func__, Stringify(json_bytes).c_str());
	throw;
      }
      if(!json_words.IsUint64()){
	FormatPrint(std::cerr, "ERROR<%s>:   words<%s> is not an int\n", __func__, Stringify(json_words).c_str());
	throw;
      }
      uint64_t n_bytes = json_bytes.GetUint64();
      uint64_t n_words = json_words.GetUint64();
      if((!n_bytes)||(!n_words)||(n_bytes%n_words)){
	FormatPrint(std::cerr, "ERROR<%s>: incorrect bytes<%u> or words<%u>\n" , __func__, json_bytes.GetUint64(), json_words.GetUint64());
	throw;
      }
      uint64_t n_bits_per_word = 8*n_bytes/n_words;
      
      auto& json_endian = json_reg["endian"];
      bool flag_is_little_endian;
      if(json_endian=="LE"){
	flag_is_little_endian = true;
      }
      else if(json_endian=="BE"){
	flag_is_little_endian = false;
      }
      else{
	FormatPrint(std::cerr, "ERROR<%s>: unknown endian<%s>\n", __func__, Stringify(json_endian).c_str());
	throw;
      }

      if(n_words != json_addr.Size()){
	FormatPrint(std::cerr, "ERROR<%s>: address<%s> array's size does not match the word number which is %u\n", __func__ , Stringify(json_addr).c_str(), n_words);
	throw;
      }
      uint64_t i=0;
      for(auto& name_in_array: json_addr.GetArray()){
	if(!name_in_array.IsString()){
	  FormatPrint(std::cerr, "ERROR<%s>: name<%s> is not a string value\n", __func__, Stringify(name_in_array).c_str());
	  throw;
	}
	std::string name_in_array_str = name_in_array.GetString();
	uint64_t sub_value;
	if(flag_is_little_endian){
	  uint64_t f = 8*sizeof(value)-n_bits_per_word*(i+1);
	  uint64_t b = 8*sizeof(value)-n_bits_per_word;
	  sub_value = (value<<f)>>b;
	  FormatPrint(std::cout, "INFO<%s>: %s value=%#016x << %u  >>%u sub_value=%#016x \n", __func__, "LE", value, f, b, sub_value);
	}
	else{
	  uint64_t f = 8*sizeof(value)-n_bits_per_word*(n_words-i);
	  uint64_t b = 8*sizeof(value)-n_bits_per_word;
	  sub_value = (value<<f)>>b;
	  FormatPrint(std::cout, "INFO<%s>: %s value=%#016x << %u  >>%u sub_value=%#016x \n", __func__, "BE", value, f, b, sub_value);
	}
	SetFirmwareRegister(name_in_array_str, sub_value);
	i++;
      }
    }
    else{
      FormatPrint(std::cerr, "ERROR<%s>: unknown address format<%s>\n", __func__, Stringify(json_addr).c_str());
      throw;
    }
    flag_found_reg = true;
    break;
  }
  if(!flag_found_reg){
    FormatPrint(std::cerr, "ERROR<%s>: unable to find register<%s> in array<%s>\n", __func__, name.c_str(), array_name.c_str());
    throw;
  }
}

void SetAlpideRegister(const std::string& name, uint64_t value){
  FormatPrint(std::cout, "INFO<%s>: %s( name=%s ,  value=%#016x )\n", __func__, __func__, name.c_str(), value);
  static const std::string array_name("CHIP_REG_LIST");
  auto& json_array = s_json[array_name];
  if(json_array.Empty()){
    FormatPrint(std::cerr, "ERROR<%s>:   unable to find array<%s>\n", __func__, array_name.c_str());
    throw;
  }   
  
  bool flag_found_reg = false;
  for(auto& json_reg: json_array.GetArray()){
    if( json_reg["name"] != name )
      continue;
    auto& json_addr = json_reg["address"];
    if(json_addr.IsString()){
      uint64_t address = std::stoull(json_reg["address"].GetString(), 0, 16);
      SetFirmwareRegister("ADDR_CHIP_REG", address);
      SetFirmwareRegister("DATA_WRITE", value);
      SendFirmwareCommand("WRITE");
    }
    else if(json_addr.IsArray()){
      auto& json_bytes = json_reg["bytes"];
      auto& json_words = json_reg["words"];
      if(!json_bytes.IsUint64()){
	FormatPrint(std::cerr, "ERROR<%s>:   bytes<%s> is not an int\n", __func__, Stringify(json_bytes).c_str());
	throw;
      }
      if(!json_words.IsUint64()){
	FormatPrint(std::cerr, "ERROR<%s>:   words<%s> is not an int\n", __func__, Stringify(json_words).c_str());
	throw;
      }
      uint64_t n_bytes = json_bytes.GetUint64();
      uint64_t n_words = json_words.GetUint64();
      if((!n_bytes)||(!n_words)||(n_bytes%n_words)){
	FormatPrint(std::cerr, "ERROR<%s>: incorrect bytes<%u> or words<%u>\n" , __func__, json_bytes.GetUint64(), json_words.GetUint64());
	throw;
      }
      uint64_t n_bits_per_word = 8*n_bytes/n_words;

      auto& json_endian = json_reg["endian"];
      bool flag_is_little_endian;
      if(json_endian=="LE"){
	flag_is_little_endian = true;
      }
      else if(json_endian=="BE"){
	flag_is_little_endian = false;
      }
      else{
	FormatPrint(std::cerr, "ERROR<%s>: unknown endian<%s>\n", __func__, Stringify(json_endian).c_str());
	throw;
      }
      
      if(n_words != json_addr.Size()){
	FormatPrint(std::cerr, "ERROR<%s>: address<%s> array's size does not match the word number which is %u\n", __func__ ,Stringify(json_addr).c_str(), n_words);
	throw;
      }
      uint64_t i=0;
      for(auto& name_in_array: json_addr.GetArray() ){
	if(!name_in_array.IsString()){
	  FormatPrint(std::cerr, "ERROR<%s>: name<%s> is not a string value\n", __func__, Stringify(name_in_array).c_str());
	  throw;
	}
	std::string name_in_array_str = name_in_array.GetString();
	uint64_t sub_value;
	if(flag_is_little_endian){
	  uint64_t f = (8*sizeof(value)-n_bits_per_word*(i+1));
	  uint64_t b = (8*sizeof(value)-n_bits_per_word);
	  sub_value = (value<<f)>>b;
	  FormatPrint(std::cout, "INFO<%s>:  %s value=%#016x << %u  >>%u sub_value=%#016x \n", __func__, "LE", value, f, b, sub_value);
	}
	else{
	  uint64_t f = (8*sizeof(value)-n_bits_per_word*(n_words-i));
	  uint64_t b = (8*sizeof(value)-n_bits_per_word);
	  sub_value = (value<<f)>>b;
	  FormatPrint(std::cout, "INFO<%s>:  %s value=%#016x << %u  >>%u sub_value=%#016x \n", __func__, "BE", value, f, b, sub_value);
	}
	SetAlpideRegister(name_in_array_str, sub_value);
	i++;  
      }
    }
    else{
      FormatPrint(std::cerr, "ERROR<%s>: unknown address format<%s>\n", __func__, Stringify(json_addr).c_str());
      throw;
    }
    flag_found_reg = true;
    break;
  }
  if(!flag_found_reg){
    FormatPrint(std::cerr, "ERROR<%s>: unable to find register<%s> in array<%s>\n", __func__, name.c_str(), array_name.c_str());
    throw;
  }
}

void SendFirmwareCommand(const std::string& name){
  FormatPrint(std::cout, "INFO<%s>:  %s( name=%s )\n", __func__, __func__, name.c_str());
  static const std::string array_name("FIRMWARE_CMD_LIST_V2");
  auto& json_array = s_json[array_name];
  if(json_array.Empty()){
    FormatPrint(std::cerr, "ERROR<%s>:   unable to find array<%s>\n", __func__, array_name.c_str());
    throw;
  }
  bool flag_found_cmd = false;
  for(auto& json_cmd: json_array.GetArray()){
    if( json_cmd["name"] != name )
      continue;
    auto& json_value = json_cmd["value"];
    if(!json_value.IsString()){
      FormatPrint(std::cerr, "ERROR<%s>:   command value<%s> is not a string\n", __func__, Stringify(json_value).c_str());
      throw;
    }
    uint64_t cmd_value = std::stoull(json_value.GetString(),0,16);
    SetFirmwareRegister("FIRMWARE_CMD", cmd_value);
    flag_found_cmd = true;
  }
  if(!flag_found_cmd){
    FormatPrint(std::cerr, "ERROR<%s>: unable to find command<%s> in array<%s>\n", __func__, name.c_str(), array_name.c_str());
    throw;
  }  
}

void SendAlpideCommand(const std::string& name){
  FormatPrint(std::cout, "INFO<%s>:  %s( name=%s )\n", __func__, __func__, name.c_str());
  static const std::string array_name("CHIP_CMD_LIST");
  auto& json_array = s_json[array_name];
  if(json_array.Empty()){
    FormatPrint(std::cerr, "ERROR<%s>:   unable to find array<%s>\n", __func__, array_name.c_str());
    throw;
  }
  bool flag_found_cmd = false;
  for(auto& json_cmd: json_array.GetArray()){
    if( json_cmd["name"] != name )
      continue;
    auto& json_value = json_cmd["value"];
    if(!json_value.IsString()){
      FormatPrint(std::cerr, "ERROR<%s>:   command value<%s> is not a string\n", __func__, Stringify(json_value).c_str());
      throw;
    }
    uint64_t cmd_value = std::stoull(json_value.GetString(),0,16);
    SetAlpideRegister("CHIP_CMD", cmd_value);
    flag_found_cmd = true;
  }
  if(!flag_found_cmd){
    FormatPrint(std::cerr, "ERROR<%s>: unable to find command<%s> in array<%s>\n", __func__, name.c_str(), array_name.c_str());
    throw;
  } 
}

void SendAlpideBroadcast(const std::string& name){
  FormatPrint(std::cout, "INFO<%s>:  %s( name=%s )\n", __func__, __func__, name.c_str());
  static const std::string array_name("CHIP_CMD_LIST");
  auto& json_array = s_json[array_name];
  if(json_array.Empty()){
    FormatPrint(std::cerr, "ERROR<%s>:   unable to find array<%s>\n", __func__, array_name.c_str());
    throw;
  }
  bool flag_found_cmd = false;
  for(auto& json_cmd: json_array.GetArray()){
    if( json_cmd["name"] != name )
      continue;
    auto& json_value = json_cmd["value"];
    if(!json_value.IsString()){
      FormatPrint(std::cerr, "ERROR<%s>:   command value<%s> is not a string\n", __func__, Stringify(json_value).c_str());
      throw;
    }
    uint64_t cmd_value = std::stoull(json_value.GetString(),0,16);
    SetFirmwareRegister("BROADCAST_OPCODE", cmd_value);
    SendFirmwareCommand("BROADCAST");
    flag_found_cmd = true;
  }
  if(!flag_found_cmd){
    FormatPrint(std::cerr, "ERROR<%s>: unable to find command<%s> in array<%s>\n", __func__, name.c_str(), array_name.c_str());
    throw;
  }
}


uint64_t GetFirmwareRegister(const std::string& name){
  FormatPrint(std::cout, "INFO<%s>:  %s( name=%s )\n", __func__, __func__, name.c_str());
  static const std::string array_name("FIRMWARE_REG_LIST_V2");
  auto& json_array = s_json[array_name];
  if(json_array.Empty()){
    FormatPrint(std::cerr, "ERROR<%s>:   unable to find array<%s>\n", __func__, array_name.c_str());
    throw;
  }
  uint64_t value;
  bool flag_found_reg = false;
  for(auto& json_reg: json_array.GetArray()){
    if( json_reg["name"] != name )
      continue;
    auto& json_addr = json_reg["address"];
    if(json_addr.IsString()){
      uint64_t address = std::stoull(json_reg["address"].GetString(), 0, 16);
      value = ReadByte(address);
    }
    else if(json_addr.IsArray()){
      auto& json_bytes = json_reg["bytes"];
      auto& json_words = json_reg["words"];
      if(!json_bytes.IsUint64()){
	FormatPrint(std::cerr, "ERROR<%s>:   bytes<%s> is not an int\n", __func__, Stringify(json_bytes).c_str());
	throw;
      }
      if(!json_words.IsUint64()){
	FormatPrint(std::cerr, "ERROR<%s>:   words<%s> is not an int\n", __func__, Stringify(json_words).c_str());
	throw;
      }
      uint64_t n_bytes = json_bytes.GetUint64();
      uint64_t n_words = json_words.GetUint64();
      if((!n_bytes)||(!n_words)||(n_bytes%n_words)){
	FormatPrint(std::cerr, "ERROR<%s>: incorrect bytes<%u> or words<%u>\n" , __func__, json_bytes.GetUint64(), json_words.GetUint64());
	throw;
      }
      uint64_t n_bits_per_word = 8*n_bytes/n_words;

      auto& json_endian = json_reg["endian"];
      bool flag_is_little_endian;
      if(json_endian=="LE"){
	flag_is_little_endian = true;
      }
      else if(json_endian=="BE"){
	flag_is_little_endian = false;
      }
      else{
	FormatPrint(std::cerr, "ERROR<%s>: unknown endian<%s>\n", __func__, Stringify(json_endian).c_str());
	throw;
      }

      if(n_words != json_addr.Size()){
	FormatPrint(std::cerr, "ERROR<%s>: address<%s> array's size does not match the word number which is %u\n", __func__ , Stringify(json_addr).c_str(), n_words);
	throw;
      }
      uint64_t i=0;
      value = 0;
      for(auto& name_in_array: json_addr.GetArray() ){
	if(!name_in_array.IsString()){
	  FormatPrint(std::cerr, "ERROR<%s>: name<%s> is not a string value\n", __func__, Stringify(name_in_array).c_str());
	  throw;
	}
	std::string name_in_array_str = name_in_array.GetString();
	uint64_t sub_value = GetFirmwareRegister(name_in_array_str);
	uint64_t add_value;
	if(flag_is_little_endian){
	  uint64_t f = n_bits_per_word*i;
	  uint64_t b = 0;
	  add_value = (sub_value<<f)>>b;
	  FormatPrint(std::cout, "INFO<%s>:  %s sub_value=%#016x << %u  >>%u add_value=%#016x \n", __func__, "LE", sub_value, f, b, add_value);
	}
	else{
	  uint64_t f = n_bits_per_word*(n_words-1-i);
	  uint64_t b = 0;
	  add_value = (sub_value<<f)>>b;
	  FormatPrint(std::cout, "INFO<%s>:  %s sub_value=%#016x << %u  >>%u add_value=%#016x \n", __func__, "BE", sub_value, f, b, add_value);
	}
	value += add_value;
	i++;
      }
    }
    else{
      FormatPrint(std::cerr, "ERROR<%s>: unknown address format<%s>\n", __func__, Stringify(json_addr).c_str());
      throw;
    }
    flag_found_reg = true;
    break;
  }
  if(!flag_found_reg){
    FormatPrint(std::cerr, "ERROR<%s>: unable to find register<%s> in array<%s>\n", __func__, name.c_str(), array_name.c_str());
    throw;
  }
  FormatPrint(std::cout, "INFO<%s>: %s( name=%s ) return value=%#016x \n", __func__, __func__, name.c_str(), value);
  return value;
}


uint64_t GetAlpideRegister(const std::string& name){  
  FormatPrint(std::cout, "INFO<%s>:  %s( name=%s )\n",__func__, __func__, name.c_str());
  static const std::string array_name("CHIP_REG_LIST");
  auto& json_array = s_json[array_name];
  if(json_array.Empty()){
    FormatPrint(std::cerr, "ERROR<%s>:   unable to find array<%s>\n", __func__, array_name.c_str());
    throw;
  }
  uint64_t value;
  bool flag_found_reg = false;
  for(auto& json_reg: json_array.GetArray()){
    if( json_reg["name"] != name )
      continue;
    auto& json_addr = json_reg["address"];
    if(json_addr.IsString()){
      uint64_t address = std::stoull(json_reg["address"].GetString(), 0, 16);
      SetFirmwareRegister("ADDR_CHIP_REG", address);
      uint64_t nr_old = GetFirmwareRegister("COUNT_READ");
      SendFirmwareCommand("READ");
      std::chrono::system_clock::time_point  tp_timeout = std::chrono::system_clock::now() +  std::chrono::milliseconds(100);
      bool flag_enable_counter_check = false; //TODO: enable it for a real hardware;
      if(!flag_enable_counter_check){
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	FormatPrint(std::cout, "WARN<%s>: checking of the read count is disabled\n", __func__);
      }
      while(flag_enable_counter_check){
	uint64_t nr_new = GetFirmwareRegister("COUNT_READ");
	if(nr_new != nr_old){
	  break;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	if(std::chrono::system_clock::now() > tp_timeout){
	  FormatPrint(std::cerr, "ERROR<%s>:  timeout to read back Alpide register<>\n", __func__, name.c_str());
	  throw;
	}
      }
      value = GetFirmwareRegister("DATA_READ");
    }
    else if(json_addr.IsArray()){
      auto& json_bytes = json_reg["bytes"];
      auto& json_words = json_reg["words"];
      if(!json_bytes.IsUint64()){
	FormatPrint(std::cerr, "ERROR<%s>:   bytes<%s> is not an int\n", __func__, Stringify(json_bytes).c_str());
	throw;
      }
      if(!json_words.IsUint64()){
	FormatPrint(std::cerr, "ERROR<%s>:   words<%s> is not an int\n", __func__, Stringify(json_words).c_str());
	throw;
      }
      uint64_t n_bytes = json_bytes.GetUint64();
      uint64_t n_words = json_words.GetUint64();
      if((!n_bytes)||(!n_words)||(n_bytes%n_words)){
	FormatPrint(std::cerr, "ERROR<%s>: incorrect bytes<%u> or words<%u>\n" , __func__, json_bytes.GetUint64(), json_words.GetUint64());
	throw;
      }
      uint64_t n_bits_per_word = 8*n_bytes/n_words;

      auto& json_endian = json_reg["endian"];
      bool flag_is_little_endian;
      if(json_endian=="LE"){
	flag_is_little_endian = true;
      }
      else if(json_endian=="BE"){
	flag_is_little_endian = false;
      }
      else{
	FormatPrint(std::cerr, "ERROR<%s>: unknown endian<%s>\n", __func__, Stringify(json_endian).c_str());
	throw;
      }

      if(n_words != json_addr.Size()){
	FormatPrint(std::cerr, "ERROR<%s>: address<%s> array's size does not match the word number which is %u\n", __func__ , Stringify(json_addr).c_str(), n_words);
	throw;
      }
      uint64_t i=0;
      value = 0;
      for(auto& name_in_array: json_addr.GetArray() ){
	if(!name_in_array.IsString()){
	  FormatPrint(std::cerr, "ERROR<%s>: name<%s> is not a string value\n", __func__, Stringify(name_in_array).c_str());
	  throw;
	}
	std::string name_in_array_str = name_in_array.GetString();
	uint64_t sub_value = GetAlpideRegister(name_in_array_str);
	uint64_t add_value;
	if(flag_is_little_endian){
	  uint64_t f = n_bits_per_word*i;
	  uint64_t b = 0;
	  add_value = (sub_value<<f)>>b;
	  FormatPrint(std::cout, "INFO<%s>:  %s sub_value=%#016x << %u  >>%u add_value=%#016x \n", __func__, "LE", sub_value, f, b, add_value);
	}
	else{
	  uint64_t f = n_bits_per_word*(n_words-1-i);
	  uint64_t b = 0;
	  add_value = (sub_value<<f)>>b;
	  FormatPrint(std::cout, "INFO<%s>:  %s sub_value=%#016x << %u  >>%u add_value=%#016x \n", __func__, "BE", sub_value, f, b, add_value);
	}
	value += add_value;
	i++;
      }
    }
    else{
      FormatPrint(std::cerr, "ERROR<%s>: unknown address format<%s>\n", __func__, Stringify(json_addr).c_str());
      throw;
    }
    flag_found_reg = true;
    break;
  }
  if(!flag_found_reg){
    FormatPrint(std::cerr, "ERROR<%s>: unable to find register<%s> in array<%s>\n", __func__, name.c_str(), array_name.c_str());
    throw;
  }  
  FormatPrint(std::cout, "INFO<%s>: %s( name=%s ) return value=%#016x \n", __func__, __func__, name.c_str(), value);
  return value;  
}



#include "getopt.h"

int main(int argc, char **argv){
  const std::string help_usage("\n\
Usage:\n\
-c json_file: path to json file\n\
-h : Print usage information to standard error and stop\n\
");
  
  int c;
  std::string c_opt;
  while ( (c = getopt(argc, argv, "c:h")) != -1) {
    switch (c) {
    case 'c':
      c_opt = optarg;
      break;
    case 'h':
      std::cout<<help_usage;
      return 0;
      break;
    default:
      std::cerr<<help_usage;
      return 1;
    }
  }
  if (optind < argc) {
    std::cerr<<"non-option ARGV-elements: ";
    while (optind < argc)
      std::cerr<<argv[optind++]<<" \n";
    std::cerr<<"\n";
    return 1;
  }

  ////////////////////////
  //test if all opts
  if(c_opt.empty()){
    std::cerr<<"json file file [-c] is not specified\n";
    std::cerr<<help_usage;
    return 1;
  }
  ///////////////////////

  std::string json_file_path = c_opt;

  std::string file_context = LoadFileToString(json_file_path);
  s_json.Parse(file_context.c_str());
  if(s_json.HasParseError()){
    fprintf(stderr, "JSON parse error: %s (at string positon %u)", rapidjson::GetParseError_En(s_json.GetParseError()), s_json.GetErrorOffset());
    return 1;
  }
  
  SetFirmwareRegister("GAP_INT_TRIG", 0x00001312);
  std::cout<<std::endl;
  GetFirmwareRegister("GAP_INT_TRIG");
  // std::cout<<std::endl;
  // SendFirmwareCommand("RESET");
  // std::cout<<std::endl;
  // SetAlpideRegister("DISABLE_REGIONS", 0x11121314);
  // std::cout<<std::endl;
  // GetAlpideRegister("DISABLE_REGIONS");
  // std::cout<<std::endl;
  // SendAlpideCommand("GRST");
  // std::cout<<std::endl;
  // SendAlpideBroadcast("TRIGGER_B1");  
  return 0;
}


int rbcp_write(const std::string& ip, uint16_t udp_port, uint32_t reg_addr, uint8_t reg_value){
  //TODO: mutex lock
  static uint8_t s_id = 0;
  static const uint8_t _RBCP_VER    = 0xFF;
  static const uint8_t _RBCP_CMD_WR = 0x80;
  static const uint8_t _RBCP_CMD_RD = 0xC0;
  
  struct sockaddr_in sitcpAddr;
  sitcpAddr.sin_family      = AF_INET;
  sitcpAddr.sin_port        = htons(udp_port);
  sitcpAddr.sin_addr.s_addr = inet_addr(ip.c_str());
  
  uint8_t reg_addr_be_0 = static_cast<uint8_t>(reg_addr>>24);
  uint8_t reg_addr_be_1 = static_cast<uint8_t>(reg_addr>>16); 
  uint8_t reg_addr_be_2 = static_cast<uint8_t>(reg_addr>>8); 
  uint8_t reg_addr_be_3 = static_cast<uint8_t>(reg_addr); 

  std::vector<uint8_t> package_data{_RBCP_VER, _RBCP_CMD_WR, s_id, 1,
				    reg_addr_be_0, reg_addr_be_1, reg_addr_be_2, reg_addr_be_3};
  
  package_data.push_back(reg_value);
  socket_t sock = socket(AF_INET, SOCK_DGRAM, 0);
  sendto(sock, reinterpret_cast<char*>(&package_data[0]), static_cast<int>(package_data.size()),
	 0, (struct sockaddr *)(&sitcpAddr), sizeof(sitcpAddr));
  
  int numReTrans =0;
  while(1){
    numReTrans++;
    if(numReTrans>3){
      close(sock);
      puts("\n***** Timeout ! abort *****");
      return -3;
    }
    
    fd_set setSelect;
    FD_ZERO(&setSelect);
    FD_SET(sock, &setSelect);
    struct timeval timeout;
    timeout.tv_sec  = 1;
    timeout.tv_usec = 0;
    if(select(sock+1, &setSelect, NULL, NULL,&timeout)==0){
      puts("\n***** Timeout ! retry *****");
      package_data[2]++;
      sendto(sock, reinterpret_cast<char*>(&package_data[0]), static_cast<int>(package_data.size()),
	     0, (struct sockaddr *)(&sitcpAddr), sizeof(sitcpAddr));
      continue;
    }
    if(FD_ISSET(sock,&setSelect))
      break;
  }
  
  char rcvdBuf[1024];
  int rcvdBytes=recvfrom(sock, rcvdBuf, 1024, 0, NULL, NULL);
  if(rcvdBytes<8){
    puts("ERROR: ACK packet is too short (<8)");
    close(sock);
    return -1;
  }
  if((0x0f & rcvdBuf[1])!=0x8){
    puts("ERROR: Detected bus error");
    close(sock);
    return -1;
  }
  close(sock);
  return rcvdBytes;  
}

int rbcp_read(const std::string& ip, uint16_t udp_port, uint32_t reg_addr, uint8_t& reg_value){
  static uint8_t s_id = 0;
  static const uint8_t _RBCP_VER    = 0xFF;
  static const uint8_t _RBCP_CMD_WR = 0x80;
  static const uint8_t _RBCP_CMD_RD = 0xC0;
  
  struct sockaddr_in sitcpAddr;
  sitcpAddr.sin_family      = AF_INET;
  sitcpAddr.sin_port        = htons(udp_port);
  sitcpAddr.sin_addr.s_addr = inet_addr(ip.c_str());

  uint8_t reg_addr_be_0 = static_cast<uint8_t>(reg_addr>>24);
  uint8_t reg_addr_be_1 = static_cast<uint8_t>(reg_addr>>16); 
  uint8_t reg_addr_be_2 = static_cast<uint8_t>(reg_addr>>8); 
  uint8_t reg_addr_be_3 = static_cast<uint8_t>(reg_addr); 

  std::vector<uint8_t> package_data{_RBCP_VER, _RBCP_CMD_WR, s_id, 1,
				    reg_addr_be_0, reg_addr_be_1, reg_addr_be_2, reg_addr_be_3};
  
  socket_t sock = socket(AF_INET, SOCK_DGRAM, 0);
  sendto(sock, reinterpret_cast<char*>(&package_data[0]), static_cast<int>(package_data.size()),
	 0, (struct sockaddr *)(&sitcpAddr), sizeof(sitcpAddr));

  int numReTrans =0;
  while(1){
    numReTrans++;
    if(numReTrans>3){
      close(sock);
      puts("\n***** Timeout ! abort *****");
      return -3;
    }
    
    fd_set setSelect;
    FD_ZERO(&setSelect);
    FD_SET(sock, &setSelect);
    struct timeval timeout;
    timeout.tv_sec  = 1;
    timeout.tv_usec = 0;
    if(select(sock+1, &setSelect, NULL, NULL,&timeout)==0){
      puts("\n***** Timeout ! retry *****");
      package_data[2]++;
      sendto(sock, reinterpret_cast<char*>(&package_data[0]), static_cast<int>(package_data.size()),
	     0, (struct sockaddr *)(&sitcpAddr), sizeof(sitcpAddr));
      continue;
    }
    if(FD_ISSET(sock,&setSelect))
      break;
  }
  
  uint8_t rcvdBuf[1024];
  int rcvdBytes=recvfrom(sock, reinterpret_cast<char*>(rcvdBuf), 1024, 0, NULL, NULL);
  close(sock);

  if(rcvdBytes<8+1){
    puts("ERROR: readback packet is too short");
    return -1;
  }
  if((0x0f & rcvdBuf[1])!=0x8){
    puts("ERROR: Detected bus error");
    return -1;
  }
  reg_value = rcvdBuf[8];
  return rcvdBytes; 

}
