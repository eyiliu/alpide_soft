#include "eudaq/OptionParser.hh"
#include "eudaq/FileReader.hh"
#include "eudaq/StdEventConverter.hh"

#include <iostream>

std::string validstr(uint32_t a){
  return ((a==-1)?std::string():std::to_string(a));
}

int main(int /*argc*/, const char **argv) {
  eudaq::OptionParser op("EUDAQ Command Line FileReader modified for TLU", "2.1", "EUDAQ FileReader (TLU)");
  eudaq::Option<std::string> file_input(op, "i", "input", "", "string", "input file");
  eudaq::Option<uint32_t> eventl(op, "e", "event", 0, "uint32_t", "event number low");
  eudaq::Option<uint32_t> eventh(op, "E", "eventhigh", 0, "uint32_t", "event number high");

  op.Parse(argv);

  std::string infile_path = file_input.Value();
  std::string type_in = infile_path.substr(infile_path.find_last_of(".")+1);
  if(type_in=="raw")
    type_in = "native";

  uint32_t eventl_v = eventl.Value();
  uint32_t eventh_v = eventh.Value();
  
  eudaq::FileReaderUP reader;
  reader = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in), infile_path);
  uint32_t event_c = 0;
  uint32_t event_c_valid_alpide = 0;
  uint32_t event_c_valid = 0;

  std::cout<<std::setw(10)<<"Event"<<std::setw(10)<<"TLU"
	   <<std::setw(22)<<"alpide[0]"<<std::setw(22)<<"alpide[1]"<<std::setw(22)<<"alpide[2]"
	   <<std::setw(22)<<"alpide[3]"<<std::setw(22)<<"alpide[4]"<<std::setw(22)<<"alpide[5]"<<std::endl;    
  
  while(1){
    auto ev = reader->GetNextEvent();
    if(!ev)
      break;
    uint32_t ev_n = ev->GetEventN();
    if (ev_n<eventl_v)
      continue;
    if (ev_n>=eventh_v)
      return 0;
    
    event_c ++;    
    auto sub_events = ev->GetSubEvents();


    uint32_t ev_n_tlu = -1;
    uint32_t tg_n_tlu = -1;
    std::vector<uint32_t> v_ev_n(6, -1);
    std::vector<uint32_t> v_tg_n(6, -1);
    for(auto& subev : sub_events){
      uint32_t dv_n = subev->GetDeviceN();
      //     std::cout<< dv_n<<std::endl;
      uint32_t tg_n = subev->GetTriggerN();
      uint32_t ev_n = subev->GetEventN();
      if(dv_n >= 50 && dv_n < 56){
	uint32_t alpide_n = dv_n -50;
	//std::cout<< alpide_n<<std::endl;
	v_ev_n[alpide_n]=ev_n;
	v_tg_n[alpide_n]=tg_n;
      }
      else if (subev->GetDescription() == "TluRawDataEvent"){
	ev_n_tlu = subev->GetEventN();
	tg_n_tlu = subev->GetTriggerN();
      }	   
    }

    std::cout<<std::setw(10)<<validstr(ev_n)<<std::setw(10)<<validstr(tg_n_tlu)<<std::setw(1)<<"|"
	     <<std::setw(11)<<validstr(v_ev_n[0])<<std::setw(10)<<validstr(v_tg_n[0])<<std::setw(1)<<"|"
	     <<std::setw(11)<<validstr(v_ev_n[1])<<std::setw(10)<<validstr(v_tg_n[1])<<std::setw(1)<<"|"
	     <<std::setw(11)<<validstr(v_ev_n[2])<<std::setw(10)<<validstr(v_tg_n[2])<<std::setw(1)<<"|"
	     <<std::setw(11)<<validstr(v_ev_n[3])<<std::setw(10)<<validstr(v_tg_n[3])<<std::setw(1)<<"|"
	     <<std::setw(11)<<validstr(v_ev_n[4])<<std::setw(10)<<validstr(v_tg_n[4])<<std::setw(1)<<"|"
	     <<std::setw(11)<<validstr(v_ev_n[5])<<std::setw(10)<<validstr(v_tg_n[5])<<std::setw(1)<<"|"
	     <<std::endl;

  }

  
  return 0;
}
