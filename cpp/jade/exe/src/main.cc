0#include <string>
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

int main(int argc, char **argv){
  std::string file_context = LoadFileToString("reg.json");
  std::string json_err;
  json11::Json json = json11::Json::parse(file_context, json_err);
  if(!json_err.empty()){
    std::cerr<<"json11 report error: \n"<< json_err <<"\n";
    return 1;
  }
  
  auto json_chip_reg = json["CHIP_REG_LIST"];
  auto json_fw_reg = json["FIRMWARE_REG_LIST_V2"];

  std::multimap<uint64_t, json11::Json> addr_mmap;
  for(auto& e: json_chip_reg.array_items()){
    auto addr_obj = e["address"];
    if(!addr_obj.is_string()){
      continue;
    }
    uint64_t addr = std::stoull(addr_obj.string_value(),0,16);
    addr_mmap.insert(std::pair<uint64_t, json11::Json>(addr, e));
  }
  
  for(auto& e: addr_mmap){
    std::cout<< e.first<<" "<< e.second.dump()<<std::endl;
  }
  
  return 0;
}

static json11::Json s_json;

void SetFirmwareRegister(const std::string& name, uint64_t value){
  static const std::string array_name("FIRMWARE_REG_LIST_V2");
  auto& json_array = s_json[array_name].array_items();
  if(json_array.empty()){
    std::cerr<<"ERROR  SetFirmwareRegister:  unable to find array<"<<json_array<<">\n";
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
      if((!n_bytes)||(!n_words)||(n_words%n_bytes)){
	std::cerr<<"ERROR  SetFirmwareRegister: incorrect bytes<"<<json_bytes.dump()<<"> words<"<< json_words.dump()<<">\n";
	throw;
      }
      uint64_t n_bits_per_word = 8*n_words/n_bytes;
      
      auto& addr_array = json_addr.array_items();
      if(n_words != addr_array.size()){
	std::cerr<<"ERROR  SetFirmwareRegister: address<"<< json_addr.dump()<<"> array's size does not match the word number which is "<<n_words<<"\n";
	throw;
      }
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

      uint64_t i=0;
      for(auto& name_in_array: addr_array ){
	if(!name_in_array.is_string()){
	  std::cerr<<"ERROR  SetFirmwareRegister: name<"<< name_in_array.dump()<<"> is not a string value\n";
	  throw;
	}
	std::string name_in_array = name_in_array.string_value();
	uint64_t sub_value;
	if(flag_is_little_endian){
	  sub_value = (value <<(8*sizeof(value)-n_bits_per_word*(i+1)) ) >>(8*sizeof(value)-n_bits_per_word);//TODO: check by real run
	}
	else{
	  sub_value = (value <<(8*sizeof(value)-n_bits_per_word*(n_words-i)) ) >>(8*sizeof(value)-n_bits_per_word);
	}
	SetFirmwareRegister(name_in_array, sub_value);
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
  static const std::string array_name("CHIP_REG_LIST");
  auto& reg_array = s_json[array_name].array_items();
  if(reg_array.empty()){
    std::cerr<<"ERROR  SetAlpideRegister:  unable to find array<"<<reg_array<<">\n";
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
      if((!n_bytes)||(!n_words)||(n_words%n_bytes)){
	std::cerr<<"ERROR  SetAlpideRegister: incorrect bytes<"<<json_bytes.dump()<<"> words<"<< json_words.dump()<<">\n";
	throw;
      }
      uint64_t n_bits_per_word = 8*n_words/n_bytes;

      auto& addr_array = json_addr.array_items();
      if(n_words != addr_array.size()){
	std::cerr<<"ERROR  SetAlpideRegister: address<"<< json_addr.dump()<<"> array's size does not match the word number which is "<<n_words<<"\n";
	throw;
      }
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
      
      uint64_t i=0;
      for(auto& name_in_array: addr_array ){
	if(!name_in_array.is_string()){
	  std::cerr<<"ERROR  SetAlpideRegister: name<"<< name_in_array.dump()<<"> is not a string value\n";
	  throw;
	}
	std::string name_in_array = name_in_array.string_value();
	uint64_t sub_value;
	if(flag_is_little_endian){
	  sub_value = (value <<(8*sizeof(value)-n_bits_per_word*(i+1)) ) >>(8*sizeof(value)-n_bits_per_word);//TODO: check by real run
	}
	else{
	  sub_value = (value <<(8*sizeof(value)-n_bits_per_word*(n_words-i)) ) >>(8*sizeof(value)-n_bits_per_word);
	}
	SetAlpideRegister(name_in_array, sub_value);
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
  static const array_name("FIRMWARE_CMD_LIST_V2");
  auto& json_array = s_json[array_name].array_items();
  if(json_array.empty()){
    std::cerr<<"ERROR  SendFirmwareCommand:  unable to find array<"<<json_array<<">\n";
    throw;
  }
  bool flag_found_cmd = false;
  for(auto& json_cmd: json_array){
    if( json_cmd["name"].string_value() != name )
      continue;
    auto& json_value = json_cmd["value"];
    if(!json_value.is_string()){
      std::cerr<<"ERROR  SendFirmwareCommand:  commmand value<"<<json_value<<"> is not a string\n";
      throw;
    }
    uint64_t cmd_value = std::stoull(json_value.string_value(),0,16);
    SetFirmwareRegiester("FIRMWARE_CMD", cmd_value);
    flog_found_cmd = true;
  }
  if(!flag_found_cmd){
    std::cerr<<"ERROR  SetFirmwareCommand:  unable to find command<"<<name <<"> in array<"<<array_name<<">\n";
    throw;
  }  
}

void SendAlpideCommand(const std::string& name){
  static const array_name("CHIP_CMD_LIST");
  auto& json_array = s_json[array_name].array_items();
  if(json_array.empty()){
    std::cerr<<"ERROR  SendAlpideCommand:  unable to find array<"<<json_array<<">\n";
    throw;
  }
  bool flag_found_cmd = false;
  for(auto& json_cmd: json_array){
    if( json_cmd["name"].string_value() != name )
      continue;
    auto& json_value = json_cmd["value"];
    if(!json_value.is_string()){
      std::cerr<<"ERROR  SendAlpideCommand:  commmand value<"<<json_value<<"> is not a string\n";
      throw;
    }
    uint64_t cmd_value = std::stoull(json_value.string_value(),0,16);
    SetAlpideRegiester("CHIP_CMD", cmd_value);
    flog_found_cmd = true;
  }
  if(!flag_found_cmd){
    std::cerr<<"ERROR  SetAlpideCommand:  unable to find command<"<<name <<"> in array<"<<array_name<<">\n";
    throw;
  } 
}

void SendAlpideBroadcast(const std::string& name){
  static const array_name("CHIP_CMD_LIST");
  auto& json_array = s_json[array_name].array_items();
  if(json_array.empty()){
    std::cerr<<"ERROR  SendAlpideBroadcast:  unable to find array<"<<json_array<<">\n";
    throw;
  }
  bool flag_found_cmd = false;
  for(auto& json_cmd: json_array){
    if( json_cmd["name"].string_value() != name )
      continue;
    auto& json_value = json_cmd["value"];
    if(!json_value.is_string()){
      std::cerr<<"ERROR  SendAlpideBroadcast:  commmand value<"<<json_value<<"> is not a string\n";
      throw;
    }
    uint64_t cmd_value = std::stoull(json_value.string_value(),0,16);
    SetFirmwareRegister("BROADCAST_OPCODE", cmd_value);
    SendFirmwareCommand("BROADCAST");
    flog_found_cmd = true;
  }
  if(!flag_found_cmd){
    std::cerr<<"ERROR  SendAlpideBroadcast:  unable to find command<"<<name <<"> in array<"<<array_name<<">\n";
    throw;
  }
}


uint64_t GetAlpideRegister(const std::string& name){  
  

  
}

uint64_t GetFirmwareRegister(const std::string& name){  


  
}

