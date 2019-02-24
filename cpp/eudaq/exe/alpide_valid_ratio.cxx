#include "eudaq/OptionParser.hh"
#include "eudaq/FileReader.hh"
#include "eudaq/StdEventConverter.hh"

#include <iostream>

int main(int /*argc*/, const char **argv) {
  eudaq::OptionParser op("EUDAQ Command Line FileReader modified for TLU", "2.1", "EUDAQ FileReader (TLU)");
  eudaq::Option<std::string> file_input(op, "i", "input", "", "string", "input file");

  op.Parse(argv);

  std::string infile_path = file_input.Value();
  std::string type_in = infile_path.substr(infile_path.find_last_of(".")+1);
  if(type_in=="raw")
    type_in = "native";


  eudaq::FileReaderUP reader;
  reader = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in), infile_path);
  uint32_t event_c = 0;
  uint32_t event_c_valid_alpide = 0;
  uint32_t event_c_valid = 0;
  
  while(1){
    auto ev = reader->GetNextEvent();
    if(!ev)
      break;
    
    event_c ++;    
    auto stdev = eudaq::StandardEvent::MakeShared();
    eudaq::StdEventConverter::Convert(ev, stdev, nullptr);

    eudaq::StandardPlane *p_alpide = 0;
    eudaq::StandardPlane *p_fei4 = 0;
    
    uint32_t num = stdev->NumPlanes();
    for (unsigned int i = 0; i < num;i++){      
      eudaq::StandardPlane *plane = &stdev->GetPlane(i);
      if(plane->Type() == std::string("USBPIXI4")){
	p_fei4 = plane;
      }
      else if(plane->Type() == std::string("alpide") && plane->ID() == 50){
	p_alpide = plane;
      }

    }

    if(p_alpide && p_fei4 && p_fei4->HitPixels()){
      event_c_valid ++;
      if(p_alpide->HitPixels()){
	event_c_valid_alpide ++;
      }
    }
    
  }

  std::cout<<"event_c:event_c_valid:event_c_valid_alpide "<<event_c<<":"<<event_c_valid<<":"<<event_c_valid_alpide<<std::endl;
  std::cout<<"event_c_valid_alpide/event_c_valid "<<double(event_c_valid_alpide)/double(event_c_valid)<<std::endl;
  
  return 0;
}
