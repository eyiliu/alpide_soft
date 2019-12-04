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

#include "json11.hpp"
#include "JadeSystem.hh"

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


void  WriteByte(uint64_t address, uint64_t value){ FormatPrint(std::cout, "WriteByte( address=%#016x ,  value=%#016x )\n", address, value); };
uint64_t ReadByte(uint64_t address){uint64_t value = address&0xff; FormatPrint(std::cout, "ReadByte( address=%#016x) return value=%#016x\n", address, value); return value;};


void SetFirmwareRegister(const std::string& name, uint64_t value);
void SetAlpideRegister(const std::string& name, uint64_t value);
void SendFirmwareCommand(const std::string& name);
void SendAlpideCommand(const std::string& name);
void SendAlpideBroadcast(const std::string& name);
uint64_t GetFirmwareRegister(const std::string& name);
uint64_t GetAlpideRegister(const std::string& name);

static json11::Json s_json;

int main(int argc, char **argv){
  std::string file_context = LoadFileToString("reg.json");
  std::string json_err;
  s_json = json11::Json::parse(file_context, json_err);
  if(!json_err.empty()){
    std::cerr<<"json11 report error: \n"<< json_err <<"\n";
    return 1;
  }
  
  // auto& json_chip_reg = s_json["CHIP_REG_LIST"];
  // auto& json_fw_reg = s_json["FIRMWARE_REG_LIST_V2"];

  // std::multimap<uint64_t, json11::Json> addr_mmap;
  // for(auto& e: json_chip_reg.array_items()){
  //   auto addr_obj = e["address"];
  //   if(!addr_obj.is_string()){
  //     continue;
  //   }
  //   uint64_t addr = std::stoull(addr_obj.string_value(),0,16);
  //   addr_mmap.insert(std::pair<uint64_t, json11::Json>(addr, e));
  // }
  
  // for(auto& e: addr_mmap){
  //   std::cout<< e.first<<" "<< e.second.dump()<<std::endl;
  // }

  // FormatPrint(std::cout, "function name is<%s>\n", __func__ );
  // return 0;
  
  SetFirmwareRegister("GAP_INT_TRIG", 0x00001312);
  std::cout<<std::endl;
  GetFirmwareRegister("GAP_INT_TRIG");
  std::cout<<std::endl;
  SendFirmwareCommand("RESET");
  std::cout<<std::endl;

  SetAlpideRegister("DISABLE_REGIONS", 0x11121314);
  std::cout<<std::endl;
  GetAlpideRegister("DISABLE_REGIONS");
  std::cout<<std::endl;
  SendAlpideCommand("GRST");
  std::cout<<std::endl;
  SendAlpideBroadcast("TRIGGER_B1");
  return 0;
}





void SetFirmwareRegister(const std::string& name, uint64_t value){
  FormatPrint(std::cout, "INFO<%s>: %s( name=%s ,  value=%#016x )\n", __func__, __func__, name.c_str(), value);
  static const std::string array_name("FIRMWARE_REG_LIST_V2");
  auto& json_array = s_json[array_name].array_items();
  if(json_array.empty()){
    FormatPrint(std::cerr, "ERROR<%s>:   unable to find array<%s>\n", __func__, array_name.c_str());
    throw;
  }
  bool flag_found_reg = false;
  for(auto& json_reg: json_array){
    if( json_reg["name"].string_value() != name )
      continue;
    auto& json_addr = json_reg["address"];
    if(json_addr.is_string()){
      uint64_t address = std::stoull(json_reg["address"].string_value(), 0, 16);
      WriteByte(address, value); //word = byte
    }
    else if(json_addr.is_array()){
      auto& json_bytes = json_reg["bytes"];
      if(!json_bytes.is_string()){
	FormatPrint(std::cerr, "ERROR<%s>:   bytes<%s> is not a string\n", __func__, json_bytes.dump().c_str());
	throw;
      }
      auto& json_words = json_reg["words"];
      if(!json_words.is_string()){
	FormatPrint(std::cerr, "ERROR<%s>:   words<%s> is not a string\n", __func__, json_words.dump().c_str());
	throw;
      }
      uint64_t n_bytes = std::stoull(json_bytes.string_value());
      uint64_t n_words = std::stoull(json_words.string_value());
      if((!n_bytes)||(!n_words)||(n_bytes%n_words)){
	FormatPrint(std::cerr, "ERROR<%s>: incorrect bytes<%s> or words<%s>\n" , __func__, json_bytes.dump().c_str(), json_words.dump().c_str());
	throw;
      }
      uint64_t n_bits_per_word = 8*n_bytes/n_words;
      
      auto& json_endian = json_reg["endian"];
      bool flag_is_little_endian;
      if(json_endian.string_value()=="LE"){
	flag_is_little_endian = true;
      }
      else if(json_endian.string_value()=="BE"){
	flag_is_little_endian = false;
      }
      else{
	FormatPrint(std::cerr, "ERROR<%s>: unknown endian<%s>\n", __func__, json_endian.dump().c_str());
	throw;
      }

      auto& addr_array = json_addr.array_items();
      if(n_words != addr_array.size()){
	FormatPrint(std::cerr, "ERROR<%s>: address<%s> array's size does not match the word number which is %u\n", __func__ , json_addr.dump().c_str(), n_words);
	throw;
      }
      uint64_t i=0;
      for(auto& name_in_array: addr_array ){
	if(!name_in_array.is_string()){
	  FormatPrint(std::cerr, "ERROR<%s>: name<%s> is not a string value\n", __func__, name_in_array.dump().c_str());
	  throw;
	}
	std::string name_in_array_str = name_in_array.string_value();
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
      FormatPrint(std::cerr, "ERROR<%s>: unknown address format<%s>\n", __func__, json_addr.dump().c_str());
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
  auto& json_array = s_json[array_name].array_items();
  if(json_array.empty()){
    FormatPrint(std::cerr, "ERROR<%s>:   unable to find array<%s>\n", __func__, array_name.c_str());
    throw;
  }   
  
  bool flag_found_reg = false;
  for(auto& json_reg: json_array){
    if( json_reg["name"].string_value() != name )
      continue;
    auto& json_addr = json_reg["address"];
    if(json_addr.is_string()){
      uint64_t address = std::stoull(json_reg["address"].string_value(), 0, 16);
      SetFirmwareRegister("ADDR_CHIP_REG", address);
      SetFirmwareRegister("DATA_WRITE", value);
      SendFirmwareCommand("WRITE");
    }
    else if(json_addr.is_array()){
      auto& json_bytes = json_reg["bytes"];
      if(!json_bytes.is_string()){
	FormatPrint(std::cerr, "ERROR<%s>:   bytes<%s> is not a string\n", __func__, json_bytes.dump().c_str());
	throw;
      }
      auto& json_words = json_reg["words"];
      if(!json_words.is_string()){
	FormatPrint(std::cerr, "ERROR<%s>:   words<%s> is not a string\n", __func__, json_words.dump().c_str());
	throw;
      }
      uint64_t n_bytes = std::stoull(json_bytes.string_value());
      uint64_t n_words = std::stoull(json_words.string_value());
      if((!n_bytes)||(!n_words)||(n_bytes%n_words)){
	FormatPrint(std::cerr, "ERROR<%s>: incorrect bytes<%s> or words<%s>\n" , __func__, json_bytes.dump().c_str(), json_words.dump().c_str());
	throw;
      }
      uint64_t n_bits_per_word = 8*n_bytes/n_words;

      auto& json_endian = json_reg["endian"];
      bool flag_is_little_endian;
      if(json_endian.string_value()=="LE"){
	flag_is_little_endian = true;
      }
      else if(json_endian.string_value()=="BE"){
	flag_is_little_endian = false;
      }
      else{
	FormatPrint(std::cerr, "ERROR<%s>: unknown endian<%s>\n", __func__, json_endian.dump().c_str());
	throw;
      }
      
      auto& addr_array = json_addr.array_items();
      if(n_words != addr_array.size()){
	FormatPrint(std::cerr, "ERROR<%s>: address<%s> array's size does not match the word number which is %u\n", __func__ , json_addr.dump().c_str(), n_words);
	throw;
      }
      uint64_t i=0;
      for(auto& name_in_array: addr_array ){
	if(!name_in_array.is_string()){
	  FormatPrint(std::cerr, "ERROR<%s>: name<%s> is not a string value\n", __func__, name_in_array.dump().c_str());
	  throw;
	}
	std::string name_in_array_str = name_in_array.string_value();
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
      FormatPrint(std::cerr, "ERROR<%s>: unknown address format<%s>\n", __func__, json_addr.dump().c_str());
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
  auto& json_array = s_json[array_name].array_items();
  if(json_array.empty()){
    FormatPrint(std::cerr, "ERROR<%s>:   unable to find array<%s>\n", __func__, array_name.c_str());
    throw;
  }
  bool flag_found_cmd = false;
  for(auto& json_cmd: json_array){
    if( json_cmd["name"].string_value() != name )
      continue;
    auto& json_value = json_cmd["value"];
    if(!json_value.is_string()){
      FormatPrint(std::cerr, "ERROR<%s>:   command value<%s> is not a string\n", __func__, json_value.dump().c_str());
      throw;
    }
    uint64_t cmd_value = std::stoull(json_value.string_value(),0,16);
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
  auto& json_array = s_json[array_name].array_items();
  if(json_array.empty()){
    FormatPrint(std::cerr, "ERROR<%s>:   unable to find array<%s>\n", __func__, array_name.c_str());
    throw;
  }
  bool flag_found_cmd = false;
  for(auto& json_cmd: json_array){
    if( json_cmd["name"].string_value() != name )
      continue;
    auto& json_value = json_cmd["value"];
    if(!json_value.is_string()){
      FormatPrint(std::cerr, "ERROR<%s>:   command value<%s> is not a string\n", __func__, json_value.dump().c_str());
      throw;
    }
    uint64_t cmd_value = std::stoull(json_value.string_value(),0,16);
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
  auto& json_array = s_json[array_name].array_items();
  if(json_array.empty()){
    FormatPrint(std::cerr, "ERROR<%s>:   unable to find array<%s>\n", __func__, array_name.c_str());
    throw;
  }
  bool flag_found_cmd = false;
  for(auto& json_cmd: json_array){
    if( json_cmd["name"].string_value() != name )
      continue;
    auto& json_value = json_cmd["value"];
    if(!json_value.is_string()){
      FormatPrint(std::cerr, "ERROR<%s>:   command value<%s> is not a string\n", __func__, json_value.dump().c_str());
      throw;
    }
    uint64_t cmd_value = std::stoull(json_value.string_value(),0,16);
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
  auto& json_array = s_json[array_name].array_items();
  if(json_array.empty()){
    FormatPrint(std::cerr, "ERROR<%s>:   unable to find array<%s>\n", __func__, array_name.c_str());
    throw;
  }
  uint64_t value;
  bool flag_found_reg = false;
  for(auto& json_reg: json_array){
    if( json_reg["name"].string_value() != name )
      continue;
    auto& json_addr = json_reg["address"];
    if(json_addr.is_string()){
      uint64_t address = std::stoull(json_reg["address"].string_value(), 0, 16);
      value = ReadByte(address);
    }
    else if(json_addr.is_array()){
      auto& json_bytes = json_reg["bytes"];
      if(!json_bytes.is_string()){
	FormatPrint(std::cerr, "ERROR<%s>:   bytes<%s> is not a string\n", __func__, json_bytes.dump().c_str());
	throw;
      }
      auto& json_words = json_reg["words"];
      if(!json_words.is_string()){
	FormatPrint(std::cerr, "ERROR<%s>:   words<%s> is not a string\n", __func__, json_words.dump().c_str());
	throw;
      }
      uint64_t n_bytes = std::stoull(json_bytes.string_value());
      uint64_t n_words = std::stoull(json_words.string_value());
      if((!n_bytes)||(!n_words)||(n_bytes%n_words)){
	FormatPrint(std::cerr, "ERROR<%s>: incorrect bytes<%s> or words<%s>\n" , __func__, json_bytes.dump().c_str(), json_words.dump().c_str());
	throw;
      }
      uint64_t n_bits_per_word = 8*n_bytes/n_words;
      
      auto& json_endian = json_reg["endian"];
      bool flag_is_little_endian;
      if(json_endian.string_value()=="LE"){
	flag_is_little_endian = true;
      }
      else if(json_endian.string_value()=="BE"){
	flag_is_little_endian = false;
      }
      else{
	FormatPrint(std::cerr, "ERROR<%s>: unknown endian<%s>\n", __func__, json_endian.dump().c_str());
	throw;
      }

      auto& addr_array = json_addr.array_items();
      if(n_words != addr_array.size()){
	FormatPrint(std::cerr, "ERROR<%s>: address<%s> array's size does not match the word number which is %u\n", __func__ , json_addr.dump().c_str(), n_words);
	throw;
      }
      uint64_t i=0;
      value = 0;
      for(auto& name_in_array: addr_array ){
	if(!name_in_array.is_string()){
	  FormatPrint(std::cerr, "ERROR<%s>: name<%s> is not a string value\n", __func__, name_in_array.dump().c_str());
	  throw;
	}
	std::string name_in_array_str = name_in_array.string_value();
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
      FormatPrint(std::cerr, "ERROR<%s>: unknown address format<%s>\n", __func__, json_addr.dump().c_str());
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
  auto& json_array = s_json[array_name].array_items();
  if(json_array.empty()){
    FormatPrint(std::cerr, "ERROR<%s>:   unable to find array<%s>\n", __func__, array_name.c_str());
    throw;
  }
  uint64_t value;
  bool flag_found_reg = false;
  for(auto& json_reg: json_array){
    if( json_reg["name"].string_value() != name )
      continue;
    auto& json_addr = json_reg["address"];
    if(json_addr.is_string()){
      uint64_t address = std::stoull(json_reg["address"].string_value(), 0, 16);
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
    else if(json_addr.is_array()){
      auto& json_bytes = json_reg["bytes"];
      if(!json_bytes.is_string()){
	FormatPrint(std::cerr, "ERROR<%s>:   bytes<%s> is not a string\n", __func__, json_bytes.dump().c_str());
	throw;
      }
      auto& json_words = json_reg["words"];
      if(!json_words.is_string()){
	FormatPrint(std::cerr, "ERROR<%s>:   words<%s> is not a string\n", __func__, json_words.dump().c_str());
	throw;
      }
      uint64_t n_bytes = std::stoull(json_bytes.string_value());
      uint64_t n_words = std::stoull(json_words.string_value());
      if((!n_bytes)||(!n_words)||(n_bytes%n_words)){
	FormatPrint(std::cerr, "ERROR<%s>: incorrect bytes<%s> or words<%s>\n" , __func__, json_bytes.dump().c_str(), json_words.dump().c_str());
	throw;
      }
      uint64_t n_bits_per_word = 8*n_bytes/n_words;
      
      auto& json_endian = json_reg["endian"];
      bool flag_is_little_endian;
      if(json_endian.string_value()=="LE"){
	flag_is_little_endian = true;
      }
      else if(json_endian.string_value()=="BE"){
	flag_is_little_endian = false;
      }
      else{
	FormatPrint(std::cerr, "ERROR<%s>: unknown endian<%s>\n", __func__, json_endian.dump().c_str());
	throw;
      }

      auto& addr_array = json_addr.array_items();
      if(n_words != addr_array.size()){
	FormatPrint(std::cerr, "ERROR<%s>: address<%s> array's size does not match the word number which is %u\n", __func__ , json_addr.dump().c_str(), n_words);
	throw;
      }
      uint64_t i=0;
      value = 0;
      for(auto& name_in_array: addr_array ){
	if(!name_in_array.is_string()){
	  FormatPrint(std::cerr, "ERROR<%s>: name<%s> is not a string value\n", __func__, name_in_array.dump().c_str());
	  throw;
	}
	std::string name_in_array_str = name_in_array.string_value();
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
      FormatPrint(std::cerr, "ERROR<%s>: unknown address format<%s>\n", __func__, json_addr.dump().c_str());
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
