#include "eudaq/OptionParser.hh"
#include "eudaq/DataConverter.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/FileReader.hh"
#include <iostream>

int main(int /*argc*/, const char **argv) {
  eudaq::OptionParser op("EUDAQ Command Line FileReader modified for TLU", "2.1", "EUDAQ FileReader (TLU)");
  eudaq::Option<std::string> file_input(op, "i", "input", "", "string",
					"input file");
  eudaq::Option<std::string> file_output(op, "o", "output", "", "string",
					 "output file");
  eudaq::OptionFlag iprint(op, "ip", "iprint", "enable print of input Event");

  try{
    op.Parse(argv);
  }
  catch (...) {
    return op.HandleMainException();
  }
  
  std::string infile_path = file_input.Value();
  if(infile_path.empty()){
    std::cout<<"option --help to get help"<<std::endl;
    return 1;
  }
  
  std::string outfile_path = file_output.Value();
  std::string type_in = infile_path.substr(infile_path.find_last_of(".")+1);
  std::string type_out = outfile_path.substr(outfile_path.find_last_of(".")+1);
  bool print_ev_in = iprint.Value();
  
  if(type_in=="raw")
    type_in = "native";
  if(type_out=="raw")
    type_out = "native";
  

  eudaq::FileReaderUP reader_pre;
  reader_pre = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in), infile_path);

  uint32_t max_subev = 0;
  for(uint32_t i=0; i< 10000; i++){
    auto ev = reader_pre->GetNextEvent();
    if(!ev)
      break;
    if(ev->GetNumSubEvent()>max_subev){
      max_subev = ev->GetNumSubEvent();
    }
  }

  
  eudaq::FileReaderUP reader;
  eudaq::FileWriterUP writer;

  reader = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in), infile_path);

  if(!type_out.empty())
    writer = eudaq::Factory<eudaq::FileWriter>::MakeUnique(eudaq::str2hash(type_out), outfile_path);
  uint32_t ev_n_dropped = 0;
  uint32_t ev_n_picked = 0;
  uint32_t ev_n_total = 0;
  while(1){
    auto ev = reader->GetNextEvent();
    if(!ev)
      break;

    ev_n_total++;
    
    if(ev->GetNumSubEvent() < max_subev){
      ev_n_dropped ++;
      continue;
    }
    
    if(print_ev_in)
      ev->Print(std::cout);
    if(writer){
      auto ev_nc = std::const_pointer_cast<eudaq::Event>(ev);
      ev_nc->SetEventN(ev_n_picked);
      writer->WriteEvent(ev);
    }
    ev_n_picked ++;
  }
  
  std::cout<< "sub events number: "<<max_subev<<std::endl;
  std::cout<< "dropped events: "<< ev_n_dropped<<" "<<double(ev_n_dropped)/double(ev_n_total)*100<<"%"<<std::endl;
  std::cout<< "picked  events: "<< ev_n_picked<<" "<<double(ev_n_picked)/double(ev_n_total)*100<<"%"<<std::endl;
  
  return 0;
}
