#include "AidaTluController.hh"

#include <cstdio>
#include <csignal>

#include <string>
#include <fstream>
#include <sstream>
#include <memory>
#include <chrono>
#include <thread>

#include <unistd.h>
#include <getopt.h>

static const std::string help_usage = R"(
Usage:
  -d dutmask, The mask for enabling the DUT connections
  -b dutnobusy, Mask for disable the veto of the DUT busy singals
  -m dutmode, Mask for DUT Mode (2 bits each channel)
  -f dutmm, Mask for DUT Mode Modifier (2 bits each channel)
  -i 1/160MHz, The interval of internally generated triggers (0 = off)
  -v vetomask, The mask for vetoing external triggers
  -a andmask, The mask for coincidence of external triggers
  -u update interval in milliseconds
  -q quit after configuring TLU
  -p pause for user input before starting triggers
  -s save trigger numbers and timestamps
  -z Debugg level of IPBus
)";


static sig_atomic_t g_done = 0;
int main(int argc, char ** argv) {

  int do_quit = false;
  int do_pause = false;
  int do_help = false;
  int do_verbose = false;

  struct option longopts[] =
    {
     { "help",       no_argument,       &do_help,      1  },
     { "verbose",    no_argument,       &do_verbose,   1  },

     { "quit",       no_argument,       &do_quit,      1  },
     { "pause",      no_argument,       &do_pause,     1  },
     { "save",       optional_argument, NULL,         's' },
     { "update",     required_argument, NULL,         'u' },
   
     { "dutmask",    required_argument, NULL,         'd' },
     { "dutnobusy",  required_argument, NULL,         'b' },
     { "dutmode",    required_argument, NULL,         'm' },
     { "dutmm",      required_argument, NULL,         'f' },
     { "vetomask",   required_argument, NULL,         'v' },
     { "andmask",    required_argument, NULL,         'a' },
     { "dutnobusy",  required_argument, NULL,         'b' },
     { "hz",         required_argument, NULL,         'i' },
     { "threshold",  required_argument, NULL,         't' },
   
     { "thr0",       required_argument, NULL,         'A' },
     { "thr1",       required_argument, NULL,         'B' },
     { "thr3",       required_argument, NULL,         'C' },
     { "thr4",       required_argument, NULL,         'D' },
     { "thr5",       required_argument, NULL,         'E' },
   
     { "dut1",       required_argument, NULL,         'F' },
     { "dut2",       required_argument, NULL,         'G' },
     { "dut3",       required_argument, NULL,         'H' },
     { "dut4",       required_argument, NULL,         'I' },
     { 0, 0, 0, 0 }};

  
  uchar_t dmask = 1;
  uchar_t dutnobusy = 7;
  uchar_t dutmode = 60;
  uchar_t dutmm = 60;
  uint32_t clk = 0;
  uchar_t vmask = 0;
  uchar_t amask = 15;
  
  float vthresh_0 = 0;
  float vthresh_1 = 0;
  float vthresh_2 = 0;
  float vthresh_3 = 0;
  float vthresh_4 = 0;
  float vthresh_5 = 0;

  uint32_t wait = 1000;
  std::string sname;
  uchar_t  ipbusDebug = 2;  

  int c;
  while ((c = getopt_long_only(argc, argv, ":hqps::u:d:b:m:f:v:a:b:i:t:z:", longopts, NULL))!= -1) {
    switch (c) {
    case 'h':
      do_help = 1;
      break;
    case 'q':
      do_quit = 1;
      break;
    case 'p':
      do_pause = 1;
      break;
    case 's':
      if (optarg != NULL)
        sname = optarg;
      else
        sname = "data.txt";
      break;

    case 'd':
      dmask = static_cast<uchar_t>(std::stoull(optarg, 0, 16));
      break;
    case 'b':
      dutnobusy = static_cast<uchar_t>(std::stoull(optarg, 0, 16));
      break;
    case 'm':
      dutmode = static_cast<uchar_t>(std::stoull(optarg, 0, 16));
      break;
    case 'f':
      dutmm = static_cast<uchar_t>(std::stoull(optarg, 0, 16));
      break;
    case 'i':
      clk = static_cast<uint32_t>(std::stoull(optarg, 0, 10));
      break;
    case 'v':
      vmask = static_cast<uchar_t>(std::stoull(optarg, 0, 16));
      break;
    case 'a':
      amask = static_cast<uchar_t>(std::stoull(optarg, 0, 16));
      break;
    case 't':
      vthresh_0 = std::stof(optarg, 0);
      vthresh_1 = std::stof(optarg, 0);  
      vthresh_2 = std::stof(optarg, 0);
      vthresh_3 = std::stof(optarg, 0);
      vthresh_4 = std::stof(optarg, 0);
      vthresh_5 = std::stof(optarg, 0);
      break;
    case 'u':
      wait  = static_cast<uint32_t>(std::stoull(optarg, 0, 10));
      break;
    case 'z':
      ipbusDebug = static_cast<uchar_t>(std::stoull(optarg, 0, 10));
      break;
      ////////////////
    case 0:     /* getopt_long() set a variable, just keep going */
      break;
#if 0
    case 1:
      /*
       * Use this case if getopt_long() should go through all
       * arguments. If so, add a leading '-' character to optstring.
       * Actual code, if any, goes here.
       */
      break;
#endif
    case ':':   /* missing option argument */
      fprintf(stderr, "%s: option `-%c' requires an argument\n",
              argv[0], optopt);
      exit(1);
      break;
    case '?':
    default:    /* invalid option */
      fprintf(stderr, "%s: option `-%c' is invalid: ignored\n",
              argv[0], optopt);
      exit(1);
      break;
    }
  }

  if(do_help){
    std::cout<<help_usage<<std::endl;
    exit(0);
  }
  
  
  std::shared_ptr<std::ofstream> sfile;
  if (sname != "") {
    sfile = std::make_shared<std::ofstream>(sname.c_str());
    if (!sfile->is_open()) {
      std::cerr<<"Unable to open file: " << sname;
      return 1;
    }
  }
  signal(SIGINT, [](int){g_done+=1;});

  tlu::AidaTluController TLU("chtcp-2.0://localhost:10203?target=192.168.200.30:50001");
  uint8_t verbose = 2;
  
  // Define constants
  TLU.DefineConst(4, 6);

  // Import I2C addresses for hardware
  // Populate address list for I2C elements
  TLU.SetI2C_core_addr( 0x21);
  TLU.SetI2C_clockChip_addr(0x68);
  TLU.SetI2C_DAC1_addr(0x13);
  TLU.SetI2C_DAC2_addr(0x1f);
  TLU.SetI2C_EEPROM_addr(0x50);
  TLU.SetI2C_expander1_addr(0x74);
  TLU.SetI2C_expander2_addr(0x75);
  TLU.SetI2C_pwrmdl_addr( 0x1C,  0x76, 0x77, 0x51);
  TLU.SetI2C_disp_addr(0x3A);

  // Initialize TLU hardware
  TLU.InitializeI2C(verbose);
  TLU.InitializeIOexp(verbose);
  TLU.InitializeDAC(false, 1.3, verbose);
  // Initialize the Si5345 clock chip using pre-generated file
  TLU.InitializeClkChip( verbose  );

  TLU.ResetSerdes();
  TLU.ResetCounters();
  TLU.SetTriggerVeto(1, verbose);
  TLU.ResetFIFO();
  TLU.ResetEventsBuffer();
  TLU.ResetTimestamp();


  //conf
  
  TLU.SetThresholdValue(0, vthresh_0, verbose);
  TLU.SetThresholdValue(1, vthresh_1, verbose);
  TLU.SetThresholdValue(2, vthresh_2, verbose);
  TLU.SetThresholdValue(3, vthresh_3, verbose);
  TLU.SetThresholdValue(4, vthresh_4, verbose);
  TLU.SetThresholdValue(5, vthresh_5, verbose);

  TLU.SetDUTMask(dmask, verbose);
  TLU.SetDUTMaskMode(dutmode, verbose);
  TLU.SetDUTMaskModeModifier(dutmm, verbose);
  TLU.SetDUTIgnoreBusy(dutnobusy, verbose);
  TLU.SetDUTIgnoreShutterVeto(1, verbose);
  TLU.SetEnableRecordData(1);

  TLU.SetInternalTriggerInterval(clk);
  TLU.SetTriggerMask(amask, verbose);
  TLU.SetTriggerVeto(vmask, verbose);
  TLU.SetUhalLogLevel(ipbusDebug);

  uint32_t bid = TLU.GetBoardID();
  uint32_t fid = TLU.GetFirmwareVersion();
  std::printf("Board ID number  = %#012x\n ", bid);
  std::printf("Firmware version  = %d\n ", fid);
  
  if (do_quit) return 0;

  if (do_pause) {
    std::cerr << "Press enter to start triggers." << std::endl;
    std::getchar();
  }
  TLU.SetTriggerVeto(0, verbose);
  std::cout << "FMC TLU Started!" << std::endl;

  uint32_t total = 0;
  while (!g_done) {
    TLU.ReceiveEvents(verbose);
    int nev = 0;
    while (!TLU.IsBufferEmpty()){
      nev++;
      tlu::fmctludata * data = TLU.PopFrontEvent();
      uint32_t evn = data->eventnumber;
      uint64_t t = data->timestamp;

      if (sfile.get()) {
        *sfile << *data;
      }
    }
    total += nev;
    if (wait > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(wait));
    }
  }
  std::cout << "Quitting..." << std::endl;
  TLU.SetTriggerVeto(1, verbose);
  return 0;
}
