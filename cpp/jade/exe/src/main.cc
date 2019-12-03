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
uint64_t ReadByte(uint64_t address){FormatPrint(std::cout, "ReadByte( address=%#016x)\n", address); return address&0xff;};


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
  FormatPrint(std::cout, "SetFirmwareRegister( name=%s ,  value=%#016x )\n", name.c_str(), value);
  static const std::string array_name("FIRMWARE_REG_LIST_V2");
  auto& json_array = s_json[array_name].array_items();
  if(json_array.empty()){
    std::cerr<<"ERROR  SetFirmwareRegister:  unable to find array<"<<array_name<<">\n";
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
	std::cerr<<"ERROR  SetFirmwareRegister: bytes<"<< json_bytes.dump()<<"> is not a string\n";
	throw;
      }
      auto& json_words = json_reg["words"];
      if(!json_words.is_string()){
	std::cerr<<"ERROR  SetFirmwareRegister: words<"<< json_words.dump()<<"> is not a string\n";
	throw;
      }
      uint64_t n_bytes = std::stoull(json_bytes.string_value());
      uint64_t n_words = std::stoull(json_words.string_value());
      if((!n_bytes)||(!n_words)||(n_bytes%n_words)){
	std::cerr<<"ERROR  SetFirmwareRegister: incorrect bytes<"<<json_bytes.dump()<<"> or words<"<< json_words.dump()<<">\n";
	throw;
      }
      uint64_t n_bits_per_word = 8*n_words/n_bytes;
      
      auto& json_endian = json_reg["endian"];
      bool flag_is_little_endian;
      if(json_endian.string_value()=="LE"){
	flag_is_little_endian = true;
      }
      else if(json_endian.string_value()=="BE"){
	flag_is_little_endian = false;
      }
      else{
	std::cerr<<"ERROR  SetFirmwareRegister: unknown endian<"<< json_endian.dump()<<">\n";
	throw;
      }

      auto& addr_array = json_addr.array_items();
      if(n_words != addr_array.size()){
	std::cerr<<"ERROR  SetFirmwareRegister: address<"<< json_addr.dump()<<"> array's size does not match the word number which is "<<n_words<<"\n";
	throw;
      }
      uint64_t i=0;
      for(auto& name_in_array: addr_array ){
	if(!name_in_array.is_string()){
	  std::cerr<<"ERROR  SetFirmwareRegister: name<"<< name_in_array.dump()<<"> is not a string value\n";
	  throw;
	}
	std::string name_in_array_str = name_in_array.string_value();
	uint64_t sub_value;
	if(flag_is_little_endian){
	  sub_value = (value <<(8*sizeof(value)-n_bits_per_word*(i+1)) ) >>(8*sizeof(value)-n_bits_per_word);//TODO: check by real run
	}
	else{
	  sub_value = (value <<(8*sizeof(value)-n_bits_per_word*(n_words-i)) ) >>(8*sizeof(value)-n_bits_per_word);
	}
	SetFirmwareRegister(name_in_array_str, sub_value);
	i++;  
      }
    }
    else{
      std::cerr<<"ERROR  SetFirmwareRegister: unknown address format<" << json_addr.dump() <<">\n";
      throw;
    }
    flag_found_reg = true;
    break;
  }
  if(!flag_found_reg){
    std::cerr<<"ERROR  SetFirmwareRegister:  unable to find register<"<<name <<"> in array<"<<array_name<<">\n";
    throw;
  }
}

void SetAlpideRegister(const std::string& name, uint64_t value){
  FormatPrint(std::cout, "SetAlpideRegister( name=%s ,  value=%#016x )\n", name.c_str(), value);
  static const std::string array_name("CHIP_REG_LIST");
  auto& json_array = s_json[array_name].array_items();
  if(json_array.empty()){
    std::cerr<<"ERROR  SetAlpideRegister:  unable to find array<"<<array_name<<">\n";
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
	std::cerr<<"ERROR  SetAlpideRegister: bytes<"<< json_bytes.dump()<<"> is not a string\n";
	throw;
      }
      auto& json_words = json_reg["words"];
      if(!json_words.is_string()){
	std::cerr<<"ERROR  SetAlpideRegister: words<"<< json_words.dump()<<"> is not a string\n";
	throw;
      }
      uint64_t n_bytes = std::stoull(json_bytes.string_value());
      uint64_t n_words = std::stoull(json_words.string_value());
      if((!n_bytes)||(!n_words)||(n_bytes%n_words)){
	std::cerr<<"ERROR  SetAlpideRegister: incorrect bytes<"<<json_bytes.dump()<<"> or words<"<< json_words.dump()<<">\n";
	throw;
      }
      uint64_t n_bits_per_word = 8*n_words/n_bytes;

      auto& json_endian = json_reg["endian"];
      bool flag_is_little_endian;
      if(json_endian.string_value()=="LE"){
	flag_is_little_endian = true;
      }
      else if(json_endian.string_value()=="BE"){
	flag_is_little_endian = false;
      }
      else{
	std::cerr<<"ERROR  SetAlpideRegister: unknown endian<"<< json_endian.dump()<<">\n";
	throw;
      }
      
      auto& addr_array = json_addr.array_items();
      if(n_words != addr_array.size()){
	std::cerr<<"ERROR  SetAlpideRegister: address<"<< json_addr.dump()<<"> array's size does not match the word number which is "<<n_words<<"\n";
	throw;
      }
      uint64_t i=0;
      for(auto& name_in_array: addr_array ){
	if(!name_in_array.is_string()){
	  std::cerr<<"ERROR  SetAlpideRegister: name<"<< name_in_array.dump()<<"> is not a string value\n";
	  throw;
	}
	std::string name_in_array_str = name_in_array.string_value();
	uint64_t sub_value;
	if(flag_is_little_endian){
	  sub_value = (value <<(8*sizeof(value)-n_bits_per_word*(i+1)) ) >>(8*sizeof(value)-n_bits_per_word);//TODO: check by real run
	}
	else{
	  sub_value = (value <<(8*sizeof(value)-n_bits_per_word*(n_words-i)) ) >>(8*sizeof(value)-n_bits_per_word);
	}
	SetAlpideRegister(name_in_array_str, sub_value);
	i++;  
      }
    }
    else{
      std::cerr<<"ERROR  SetAlpideRegister: unknown address format<" << json_addr.dump() <<">\n";
      throw;
    }
    flag_found_reg = true;
    break;
  }
  if(!flag_found_reg){
    std::cerr<<"ERROR  SetAlpideRegister:  unable to find register<"<<name <<"> in array<"<<array_name<<">\n";
    throw;
  }
}

void SendFirmwareCommand(const std::string& name){
  FormatPrint(std::cout, "SendFirmwareCommand( name=%s )\n", name.c_str());
  static const std::string array_name("FIRMWARE_CMD_LIST_V2");
  auto& json_array = s_json[array_name].array_items();
  if(json_array.empty()){
    std::cerr<<"ERROR  SendFirmwareCommand:  unable to find array<"<<array_name<<">\n";
    throw;
  }
  bool flag_found_cmd = false;
  for(auto& json_cmd: json_array){
    if( json_cmd["name"].string_value() != name )
      continue;
    auto& json_value = json_cmd["value"];
    if(!json_value.is_string()){
      std::cerr<<"ERROR  SendFirmwareCommand:  commmand value<"<<json_value.dump()<<"> is not a string\n";
      throw;
    }
    uint64_t cmd_value = std::stoull(json_value.string_value(),0,16);
    SetFirmwareRegister("FIRMWARE_CMD", cmd_value);
    flag_found_cmd = true;
  }
  if(!flag_found_cmd){
    std::cerr<<"ERROR  SetFirmwareCommand:  unable to find command<"<<name <<"> in array<"<<array_name<<">\n";
    throw;
  }  
}

void SendAlpideCommand(const std::string& name){
  FormatPrint(std::cout, "SendAlpideCommand( name=%s )\n", name.c_str());
  static const std::string array_name("CHIP_CMD_LIST");
  auto& json_array = s_json[array_name].array_items();
  if(json_array.empty()){
    std::cerr<<"ERROR  SendAlpideCommand:  unable to find array<"<<array_name<<">\n";
    throw;
  }
  bool flag_found_cmd = false;
  for(auto& json_cmd: json_array){
    if( json_cmd["name"].string_value() != name )
      continue;
    auto& json_value = json_cmd["value"];
    if(!json_value.is_string()){
      std::cerr<<"ERROR  SendAlpideCommand:  commmand value<"<<json_value.dump()<<"> is not a string\n";
      throw;
    }
    uint64_t cmd_value = std::stoull(json_value.string_value(),0,16);
    SetAlpideRegister("CHIP_CMD", cmd_value);
    flag_found_cmd = true;
  }
  if(!flag_found_cmd){
    std::cerr<<"ERROR  SetAlpideCommand:  unable to find command<"<<name <<"> in array<"<<array_name<<">\n";
    throw;
  } 
}

void SendAlpideBroadcast(const std::string& name){
  FormatPrint(std::cout, "SendAlpideBroadcast( name=%s )\n", name.c_str());
  static const std::string array_name("CHIP_CMD_LIST");
  auto& json_array = s_json[array_name].array_items();
  if(json_array.empty()){
    std::cerr<<"ERROR  SendAlpideBroadcast:  unable to find array<"<<array_name<<">\n";
    throw;
  }
  bool flag_found_cmd = false;
  for(auto& json_cmd: json_array){
    if( json_cmd["name"].string_value() != name )
      continue;
    auto& json_value = json_cmd["value"];
    if(!json_value.is_string()){
      std::cerr<<"ERROR  SendAlpideBroadcast:  commmand value<"<<json_value.dump()<<"> is not a string\n";
      throw;
    }
    uint64_t cmd_value = std::stoull(json_value.string_value(),0,16);
    SetFirmwareRegister("BROADCAST_OPCODE", cmd_value);
    SendFirmwareCommand("BROADCAST");
    flag_found_cmd = true;
  }
  if(!flag_found_cmd){
    std::cerr<<"ERROR  SendAlpideBroadcast:  unable to find command<"<<name <<"> in array<"<<array_name<<">\n";
    throw;
  }
}


uint64_t GetFirmwareRegister(const std::string& name){
  FormatPrint(std::cout, "GetFirmwareRegister( name=%s )\n", name.c_str());
  static const std::string array_name("FIRMWARE_REG_LIST_V2");
  auto& json_array = s_json[array_name].array_items();
  if(json_array.empty()){
    std::cerr<<"ERROR  GetFirmwareRegister:  unable to find array<"<<array_name<<">\n";
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
	std::cerr<<"ERROR  GetFirmwareRegister: bytes<"<< json_bytes.dump()<<"> is not a string\n";
	throw;
      }
      auto& json_words = json_reg["words"];
      if(!json_words.is_string()){
	std::cerr<<"ERROR  GetFiremwareRegister: words<"<< json_words.dump()<<"> is not a string\n";
	throw;
      }
      uint64_t n_bytes = std::stoull(json_bytes.string_value());
      uint64_t n_words = std::stoull(json_words.string_value());
      if((!n_bytes)||(!n_words)||(n_bytes%n_words)){
	std::cerr<<"ERROR  GetFirmwareRegister: incorrect bytes<"<<json_bytes.dump()<<"> or words<"<< json_words.dump()<<">\n";
	throw;
      }
      uint64_t n_bits_per_word = 8*n_words/n_bytes;
      
      auto& json_endian = json_reg["endian"];
      bool flag_is_little_endian;
      if(json_endian.string_value()=="LE"){
	flag_is_little_endian = true;
      }
      else if(json_endian.string_value()=="BE"){
	flag_is_little_endian = false;
      }
      else{
	std::cerr<<"ERROR  GetFirmwareRegister: unknown endian<"<< json_endian.dump()<<">\n";
	throw;
      }

      auto& addr_array = json_addr.array_items();
      if(n_words != addr_array.size()){
	std::cerr<<"ERROR  GetFirmwareRegister: address<"<< json_addr.dump()<<"> array's size does not match the word number which is "<<n_words<<"\n";
	throw;
      }
      uint64_t i=0;
      for(auto& name_in_array: addr_array ){
	if(!name_in_array.is_string()){
	  std::cerr<<"ERROR  GetFirmwareRegister: name<"<< name_in_array.dump()<<"> is not a string value\n";
	  throw;
	}
	std::string name_in_array_str = name_in_array.string_value();
	uint64_t sub_value = GetFirmwareRegister(name_in_array_str);
	if(flag_is_little_endian){
	  sub_value = (sub_value <<(n_bits_per_word*i) );
	}
	else{
	  sub_value = (sub_value <<(n_bits_per_word*(n_words-1-i)) );
	}
	value += sub_value;
	i++;
      }
    }
    else{
      std::cerr<<"ERROR  GetFirmwareRegister: unknown address format<" << json_addr.dump() <<">\n";
      throw;
    }
    flag_found_reg = true;
    break;
  }
  if(!flag_found_reg){
    std::cerr<<"ERROR  GetFirmwareRegister:  unable to find register<"<<name <<"> in array<"<<array_name<<">\n";
    throw;
  }
  FormatPrint(std::cout, "GetFirmwareRegister( name=%s ) return value=%#016x \n", name.c_str(), value);
  return value;
}


uint64_t GetAlpideRegister(const std::string& name){  
  FormatPrint(std::cout, "GetAlpideRegister( name=%s )\n", name.c_str());
  static const std::string array_name("CHIP_REG_LIST");
  auto& json_array = s_json[array_name].array_items();
  if(json_array.empty()){
    std::cerr<<"ERROR  GetAlpideRegister:  unable to find array<"<<array_name<<">\n";
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
      SendFirmwareCommand("READ");
      //TODO: sleep
      value = GetFirmwareRegister("DATA_READ");
    }
    else if(json_addr.is_array()){
      auto& json_bytes = json_reg["bytes"];
      if(!json_bytes.is_string()){
	std::cerr<<"ERROR  GetAlpideRegister: bytes<"<< json_bytes.dump()<<"> is not a string\n";
	throw;
      }
      auto& json_words = json_reg["words"];
      if(!json_words.is_string()){
	std::cerr<<"ERROR  GetAlpideRegister: words<"<< json_words.dump()<<"> is not a string\n";
	throw;
      }
      uint64_t n_bytes = std::stoull(json_bytes.string_value());
      uint64_t n_words = std::stoull(json_words.string_value());
      if((!n_bytes)||(!n_words)||(n_bytes%n_words)){
	std::cerr<<"ERROR  GetAlpideRegister: incorrect bytes<"<<json_bytes.dump()<<"> or words<"<< json_words.dump()<<">\n";
	throw;
      }
      uint64_t n_bits_per_word = 8*n_words/n_bytes;
      
      auto& json_endian = json_reg["endian"];
      bool flag_is_little_endian;
      if(json_endian.string_value()=="LE"){
	flag_is_little_endian = true;
      }
      else if(json_endian.string_value()=="BE"){
	flag_is_little_endian = false;
      }
      else{
	std::cerr<<"ERROR  GetAlpideRegister: unknown endian<"<< json_endian.dump()<<">\n";
	throw;
      }

      auto& addr_array = json_addr.array_items();
      if(n_words != addr_array.size()){
	std::cerr<<"ERROR  GetAlpideRegister: address<"<< json_addr.dump()<<"> array's size does not match the word number which is "<<n_words<<"\n";
	throw;
      }
      uint64_t i=0;
      for(auto& name_in_array: addr_array ){
	if(!name_in_array.is_string()){
	  std::cerr<<"ERROR  GetAlpideRegister: name<"<< name_in_array.dump()<<"> is not a string value\n";
	  throw;
	}
	std::string name_in_array_str = name_in_array.string_value();
	uint64_t sub_value = GetAlpideRegister(name_in_array_str);
	if(flag_is_little_endian){
	  sub_value = (sub_value <<(n_bits_per_word*i) );
	}
	else{
	  sub_value = (sub_value <<(n_bits_per_word*(n_words-1-i)) );
	}
	value += sub_value;
	i++;
      }
    }
    else{
      std::cerr<<"ERROR  GetAlpideRegister: unknown address format<" << json_addr.dump() <<">\n";
      throw;
    }
    flag_found_reg = true;
    break;
  }
  if(!flag_found_reg){
    std::cerr<<"ERROR  GetAlpideRegister:  unable to find register<"<<name <<"> in array<"<<array_name<<">\n";
    throw;
  }  
  FormatPrint(std::cout, "GetAlpideRegister( name=%s ) return value=%#016x \n", name.c_str(), value);
  return value;  
}
