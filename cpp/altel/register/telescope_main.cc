
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <iostream>
#include <deque>
#include <queue>
#include <mutex>
#include <future>
#include <condition_variable>
#include <signal.h>

#include "FirmwarePortal.hh"
#include "AltelReader.hh"
#include "common/getopt/getopt.h"
#include "common/linenoiseng/linenoise.h"

std::vector<std::string> explode(const std::string& str, const char& ch) {
  std::string next;
  std::vector<std::string> result;

  // For each character in the string
  for (std::string::const_iterator it = str.begin(); it != str.end(); it++) {
    // If we've hit the terminal character
    if (*it == ch) {
      // If we have some characters accumulated
      if (!next.empty()) {
        // Add them to the result vector
        result.push_back(next);
        next.clear();
      }
    } else {
      // Accumulate the next character into the sequence
      next += *it;
    }
  }
  if (!next.empty())
    result.push_back(next);
  return result;
}


void fw_threshold(FirmwarePortal *m_fw, uint32_t thrshold){
  m_fw->SetAlpideRegister("ITHR", thrshold); //empty 0x32; 0x12 data, not full.
}

void fw_start(FirmwarePortal *m_fw){
  m_fw->SetAlpideRegister("CHIP_MODE", 0x3d); //trigger MODE
  m_fw->SendAlpideBroadcast("RORST"); //Readout (RRU/TRU/DMU) reset
  m_fw->SetFirmwareRegister("FIRMWARE_MODE", 1); //run ext trigger
}

void fw_stop(FirmwarePortal *m_fw){
  m_fw->SetAlpideRegister("CHIP_MODE", 0x3c); // configure mode
  m_fw->SetFirmwareRegister("FIRMWARE_MODE", 0);
}

void fw_init(FirmwarePortal *m_fw){
  //=========== init part ========================
  // 3.8 Chip initialization
  // GRST
  m_fw->SetFirmwareRegister("FIRMWARE_MODE", 0);
  m_fw->SetFirmwareRegister("ADDR_CHIP_ID", 0x10); //OB
  m_fw->SendAlpideBroadcast("GRST"); // chip global reset
  m_fw->SetAlpideRegister("CHIP_MODE", 0x3c); // configure mode
  // DAC setup
  m_fw->SetAlpideRegister("VRESETP", 0x75); //117
  m_fw->SetAlpideRegister("VRESETD", 0x93); //147
  m_fw->SetAlpideRegister("VCASP", 0x56);   //86
  m_fw->SetAlpideRegister("VCASN", 0x32);   //57 Y50
  m_fw->SetAlpideRegister("VPULSEH", 0xff); //255 
  m_fw->SetAlpideRegister("VPULSEL", 0x0);  //0
  m_fw->SetAlpideRegister("VCASN2", 0x39);  //62 Y63
  m_fw->SetAlpideRegister("VCLIP", 0x0);    //0
  m_fw->SetAlpideRegister("VTEMP", 0x0);
  m_fw->SetAlpideRegister("IAUX2", 0x0);
  m_fw->SetAlpideRegister("IRESET", 0x32);  //50
  m_fw->SetAlpideRegister("IDB", 0x40);     //64
  m_fw->SetAlpideRegister("IBIAS", 0x40);   //64
  m_fw->SetAlpideRegister("ITHR", 40);    //51  empty 0x32; 0x12 data, not full.  0x33 default, threshold
  // 3.8.1 Configuration of in-pixel logic
  m_fw->SendAlpideBroadcast("PRST");  //pixel matrix reset
  m_fw->BroadcastPixelRegister("MASK_EN", 0);
  m_fw->BroadcastPixelRegister("PULSE_EN", 0);
  // m_fw->SendAlpideBroadcast("PRST");  //pixel matrix reset
  // 3.8.2 Configuration and start-up of the Data Transmission Unit, PLL
  m_fw->SetAlpideRegister("DTU_CONF", 0x008d);
  m_fw->SetAlpideRegister("DTU_DAC",  0x0088);
  m_fw->SetAlpideRegister("DTU_CONF", 0x0085);
  m_fw->SetAlpideRegister("DTU_CONF", 0x0185);
  m_fw->SetAlpideRegister("DTU_CONF", 0x0085);
  // 3.8.3 Setting up of readout
  // 3.8.3.1a (OB) Setting CMU and DMU Configuration Register
  // Previous Chip
  m_fw->SetAlpideRegister("CMU_DMU_CONF", 0x70); //disable MCH encoding, enable DDR, no previous OB
  m_fw->SetAlpideRegister("TEST_CTRL", 0x400); //Disable Busy Line
  // Initial Token 1
  // 3.8.3.2 Setting FROMU Configuration Registers and enabling readout mode
  // FROMU Configuration Register 1,2
  m_fw->SetAlpideRegister("FROMU_CONF_1", 0x00); //Disable external busy
  m_fw->SetAlpideRegister("FROMU_CONF_2", 156); //STROBE duration
  // FROMU Pulsing Register 1,2
  m_fw->SetAlpideRegister("FROMU_PULSING_2", 0xffff); //yiliu: test pulse duration, max  
  // Periphery Control Register (CHIP MODE)
  m_fw->SetAlpideRegister("CHIP_MODE", 0x3d); //trigger MODE
  // RORST 
  m_fw->SendAlpideBroadcast("RORST"); //Readout (RRU/TRU/DMU) reset

  //===========end of init part =====================

  m_fw->SetFirmwareRegister("TRIG_DELAY", 100); //25ns per dig (FrameDuration?)
  m_fw->SetFirmwareRegister("GAP_INT_TRIG", 20);
}





static const char* examples[] =
  {
   "start", "stop", "init", "threshold", "window", "connect","quit", NULL
  };

void completionHook (char const* prefix, linenoiseCompletions* lc) {
  size_t i;
  for (i = 0;  examples[i] != NULL; ++i) {
    if (strncmp(prefix, examples[i], strlen(prefix)) == 0) {
      linenoiseAddCompletion(lc, examples[i]);
    }
  }
}


int main(int argc, char **argv){
  const std::string help_usage("\n\
Usage:\n\
-c json_file: path to json file\n\
-i ip_address: eg. 131.169.133.170 for alpide_0 \n\
-h : Print usage information to standard error and stop\n\
");
  
  int c;
  std::string c_opt;
  std::string i_opt;
  std::string w_opt;
  bool w_opt_enable = false;
  bool r_opt = false;
  bool e_opt = false;
  while ( (c = getopt(argc, argv, "c:i:w:reh")) != -1) {
    switch (c) {
    case 'c':
      c_opt = optarg;
      break;
    case 'i':
      i_opt = optarg;
      break;
    case 'w':
      w_opt_enable = true;
      w_opt = optarg;
      break;
    case 'r':
      r_opt = true;
      break;
    case 'e':
      e_opt = true;
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
    std::cerr<<"\ninvalid options: ";
    while (optind < argc)
      std::cerr<<argv[optind++]<<" \n";
    std::cerr<<"\n";
    return 1;
  }

  ////////////////////////
  //test if all opts
  if(c_opt.empty() || i_opt.empty()){
    std::cerr<<"\ninsufficient options.\n";
    std::cerr<<help_usage;
    std::cerr<<"\n\n\n";
    return 1;
  }
  ///////////////////////
  
  std::string json_file_path = c_opt;

  ///////////////////////
  
  std::string file_context = FirmwarePortal::LoadFileToString(json_file_path);
  FirmwarePortal fw(file_context, i_opt);
  
  linenoiseInstallWindowChangeHandler();

  const char* file = "./tmp/alpide_cmd_history";

  linenoiseHistoryLoad(file);
  linenoiseSetCompletionCallback(completionHook);

  char const* prompt = "\x1b[1;32malpide\x1b[0m> ";

  while (1) {
    char* result = linenoise(prompt);
    if (result == NULL) {
      break;
    }
    
    if (!strncmp(result, "/history", 8)) {
      /* Display the current history. */
      for (int index = 0; ; ++index) {
        char* hist = linenoiseHistoryLine(index);
        if (hist == NULL)
          break;
        printf("%4d: %s\n", index, hist);
        free(hist);
       }
    }
    
    if (*result == '\0') {
      free(result);
      break;
    }
    
       
    linenoiseHistoryAdd(result);
    free(result);
  }

  linenoiseHistorySave(file);
  linenoiseHistoryFree();
}
