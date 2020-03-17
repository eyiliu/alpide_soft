#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <iostream>

#include <signal.h>

#include "Telescope.hh"

#include "getopt.h"
#include "linenoise.h"



void fw_threshold(FirmwarePortal *m_fw, uint32_t thrshold){
  m_fw->SetAlpideRegister("ITHR", thrshold); //empty 0x32; 0x12 data, not full.
}


int main(int argc, char **argv){
  const std::string help_usage("\n\
Usage:\n\
-c json_file: path to json file\n\
-h : print usage information, and then quit\n\
");
  
  std::string c_opt;
  std::string w_opt;
  bool w_opt_enable = false;
  int c;
  while ( (c = getopt(argc, argv, "c:w:h")) != -1) {
    switch (c) {
    case 'c':
      c_opt = optarg;
      break;
    case 'w':
      w_opt_enable = true;
      w_opt = optarg;
      break;
    case 'h':
      fprintf(stdout, "%s", help_usage.c_str());
      return 0;
      break;
    default:
      fprintf(stderr, "%s", help_usage.c_str());
      return 1;
    }
  }
  
  if (optind < argc) {
    fprintf(stderr, "\ninvalid options: ");
    while (optind < argc)
      fprintf(stderr, "%s\n", argv[optind++]);;
    return 1;
  }

  ////////////////////////
  //test if all opts
  if(c_opt.empty() ){
    fprintf(stderr, "\ninsufficient options.\n%s\n\n\n",help_usage.c_str());
    return 1;
  }
  ///////////////////////
  
  std::string json_file_path = c_opt;

  ///////////////////////
  std::string file_context = FirmwarePortal::LoadFileToString(json_file_path);
  
  Telescope tel(file_context);
  
  const char* linenoise_history_path = "/tmp/.alpide_cmd_history";
  linenoiseHistoryLoad(linenoise_history_path);
  linenoiseSetCompletionCallback([](const char* prefix, linenoiseCompletions* lc)
                                 {
                                   static const char* examples[] =
                                     {"help", "start", "stop", "init", "threshold", "window", "info",
                                      "connect","quit", "sensor", "firmware", "set", "get", "region",
                                      "broadcast", "pixel", "ture", "reset","false", "ITHR",
                                      NULL};
                                   size_t i;
                                   for (i = 0;  examples[i] != NULL; ++i) {
                                     if (strncmp(prefix, examples[i], strlen(prefix)) == 0) {
                                       linenoiseAddCompletion(lc, examples[i]);
                                     }
                                   }
                                 } );
  
  const char* prompt = "\x1b[1;32malpide\x1b[0m> ";
  while (1) {
    char* result = linenoise(prompt);
    if (result == NULL) {
      break;
    }    
    if ( std::regex_match(result, std::regex("\\s*(quit)\\s*")) ){
      printf("quiting \n");
      linenoiseHistoryAdd(result);
      free(result);
      break;
    }
    else if (std::regex_match(result, std::regex("\\s*(start)\\s*"))){
      printf("starting \n");
      tel.Start();
    }
    else if (std::regex_match(result, std::regex("\\s*(stop)\\s*"))){
      printf("stop \n");
      tel.Stop();
    }
    else if (std::regex_match(result, std::regex("\\s*(init)\\s*"))){
      printf("init \n");
      for(auto& l: tel.m_vec_layer){
        l->fw_init();
      }
    }
    else if (std::regex_match(result, std::regex("\\s*(reset)\\s*"))){
      printf("rest \n");
      for(auto& l: tel.m_vec_layer){
        l->m_fw->SendFirmwareCommand("RESET");
      }
    }

    else if ( std::regex_match(result, std::regex("\\s*(sensor)\\s+(set)\\s+(\\w+)\\s+(?:(0[Xx])?([0-9]+))\\s*")) ){
      std::cmatch mt;
      std::regex_match(result, mt, std::regex("\\s*(sensor)\\s+(set)\\s+(\\w+)\\s+(?:(0[Xx])?([0-9]+))\\s*"));
      std::string name = mt[3].str();
      uint64_t value = std::stoull(mt[5].str(), 0, mt[4].str().empty()?10:16);
      for(auto& l: tel.m_vec_layer){
        auto &fw = l->m_fw;
        fw->SetAlpideRegister(name, value);
      }
    }
    else if ( std::regex_match(result, std::regex("\\s*(sensor)\\s+(get)\\s+(\\w+)\\s*")) ){
      std::cmatch mt;
      std::regex_match(result, mt, std::regex("\\s*(sensor)\\s+(get)\\s+(\\w+)\\s*"));
      std::string name = mt[3].str();
      for(auto& l: tel.m_vec_layer){
        auto &fw = l->m_fw;
        uint64_t value = fw->GetAlpideRegister(name);
        fprintf(stderr, "%s = %u, %#x\n", name.c_str(), value, value);
      }
    }    
    else if ( std::regex_match(result, std::regex("\\s*(firmware)\\s+(set)\\s+(\\w+)\\s+(?:(0[Xx])?([0-9]+))\\s*")) ){
      std::cmatch mt;
      std::regex_match(result, mt, std::regex("\\s*(firmware)\\s+(set)\\s+(\\w+)\\s+(?:(0[Xx])?([0-9]+))\\s*"));
      std::string name = mt[3].str();
      uint64_t value = std::stoull(mt[5].str(), 0, mt[4].str().empty()?10:16);
      for(auto& l: tel.m_vec_layer){
        auto &fw = l->m_fw;
        fw->SetFirmwareRegister(name, value);
      }
    }
    else if ( std::regex_match(result, std::regex("\\s*(firmware)\\s+(get)\\s+(\\w+)\\s*")) ){
      std::cmatch mt;
      std::regex_match(result, mt, std::regex("\\s*(firmware)\\s+(get)\\s+(\\w+)\\s*"));
      std::string name = mt[3].str();
      for(auto& l: tel.m_vec_layer){
        auto &fw = l->m_fw;
        uint64_t value = fw->GetFirmwareRegister(name);
        fprintf(stderr, "%s = %u, %#x\n", name.c_str(), value, value);
      }
    }
    else if ( std::regex_match(result, std::regex("\\s*(pixel)\\s+(mask)\\s+(?:(0[Xx])?([0-9]+))\\s+(?:(0[Xx])?([0-9]+))\\s+(true|false)\\s*")) ){
      std::cmatch mt;
      std::regex_match(result, mt, std::regex("\\s*(pixel)\\s+(mask)\\s+(?:(0[Xx])?([0-9]+))\\s+(?:(0[Xx])?([0-9]+))\\s+(true|false)\\s*"));
      uint64_t x = std::stoull(mt[4].str(), 0, mt[3].str().empty()?10:16);
      uint64_t y = std::stoull(mt[6].str(), 0, mt[5].str().empty()?10:16);
      bool value =  (mt[7].str()=="true")?true:false;
      for(auto& l: tel.m_vec_layer){
        auto &fw = l->m_fw;
        fw->SetPixelRegister(x, y, "MASK_EN", value);
      }
    }
    else if( std::regex_match(result, std::regex("\\s*(threshold)\\s+(?:(0[Xx])?([0-9]+))\\s*")) ){
      std::cmatch mt;
      std::regex_match(result, mt, std::regex("\\s*(threshold)\\s+(?:(0[Xx])?([0-9]+))\\s*"));
      uint16_t base = mt[2].str().empty()?10:16;
      uint64_t ithr = std::stoull(mt[3].str(), 0, base);
      for(auto& l: tel.m_vec_layer){
        auto &fw = l->m_fw;
        fw->SetAlpideRegister("ITHR", ithr);
      }
    }
    else if (!strncmp(result, "info", 5)){
      std::cout<<"layer number: "<< tel.m_vec_layer.size()<<std::endl;
      for(auto& l: tel.m_vec_layer){
        auto &fw = l->m_fw;
        uint32_t ip0 = fw->GetFirmwareRegister("IP0");
        uint32_t ip1 = fw->GetFirmwareRegister("IP1");
        uint32_t ip2 = fw->GetFirmwareRegister("IP2");
        uint32_t ip3 = fw->GetFirmwareRegister("IP3");
        std::cout<<"\n\ncurrent ip  " <<ip0<<":"<<ip1<<":"<<ip2<<":"<<ip3<<"\n\n"<<std::endl;
      }
    }
    linenoiseHistoryAdd(result);
    free(result);
  }

  linenoiseHistorySave(linenoise_history_path);
  linenoiseHistoryFree();
}
