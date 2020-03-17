#ifndef _FIRMWARE_PORTAL_HH_
#define _FIRMWARE_PORTAL_HH_

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
#include "myrapidjson.h"

class FirmwarePortal{
public:
  FirmwarePortal(const std::string &json_str, const std::string &ipaddr);
  FirmwarePortal(const std::string &json_str_options);

  void SetFirmwareRegister(const std::string& name, uint64_t value);
  void SetAlpideRegister(const std::string& name, uint64_t value);
  void SetRegionRegister(uint64_t region, const std::string& name, uint64_t value);  
  void SetPixelRegister(uint64_t x, uint64_t y, const std::string& name, uint64_t value); //page 70 manual 

  void BroadcastRegionRegister(const std::string& name, uint64_t value);
  void BroadcastPixelRegister(const std::string& name, uint64_t value);

  
  // uint64_t GetRegionRegister(uint64_t r, const std::string& name, uint64_t);
  void InjectPulse();// test only, to be removed

  
  void SendFirmwareCommand(const std::string& name);
  void SendAlpideCommand(const std::string& name);
  void SendAlpideBroadcast(const std::string& name);
  uint64_t GetFirmwareRegister(const std::string& name);
  uint64_t GetAlpideRegister(const std::string& name);  

  static std::string LoadFileToString(const std::string& path);

private:
  void  WriteByte(uint64_t address, uint64_t value);
  uint64_t ReadByte(uint64_t address);
  
  rapidjson::Document m_json;
  std::string m_alpide_ip_addr;

public:
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

  template<typename ... Args>
  static std::size_t DebugFormatPrint(std::ostream &os, const std::string& format, Args ... args ){
    return 0;
    std::size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1;
    std::unique_ptr<char[]> buf( new char[ size ] ); 
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    std::string formated_string( buf.get(), buf.get() + size - 1 );
    os<<formated_string<<std::flush;
    return formated_string.size();
  }


  
  template<typename T>
  static const std::string Stringify(const T& o){
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    o.Accept(writer);
    return sb.GetString();
  }

  template<typename T>
  static void PrintJson(const T& o){
    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> w(sb);
    o.Accept(w);
    std::fwrite(sb.GetString(), 1, sb.GetSize(), stdout);
  }

};


#endif

  
