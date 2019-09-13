#define WIN32_LEAN_AND_MEAN

#include "JadeCore.hh"

#include "eudaq/Producer.hh"

#include <list>
#include <iostream>
#include <chrono>
#include <thread>

namespace{
  auto _loading_print_
    = JadeUtils::FormatPrint(std::cout, "<INFO> %s  this eudaq module is loaded from %s\n\n",
                             JadeUtils::GetNowStr().c_str(), JadeUtils::GetBinaryPath().c_str())
    ;
}

class EudaqWriter: public JadeWriter {
public:
  EudaqWriter(const JadeOption &opt);
  ~EudaqWriter() override;
  void Write(JadeDataFrameSP df) override;
  void Open() override;
  void Close() override;

  uint32_t ExtendTriggerN(uint16_t tg_l16);
  void SetProducerCallback(eudaq::Producer *producer);
private:
  eudaq::Producer *m_producer;
  std::list<JadeDataFrameSP> m_list_cached_df;
  uint32_t m_tg_expected;
  
  uint32_t m_tg_last_send;
  uint32_t m_flag_wait_first_event;

  uint32_t m_broken_c{0};
  bool m_stop_for_debug{false};

};

//+++++++++++++++++++++++++++++++++++++++++
//TestWrite.cc
namespace{
  auto _test_index_ = JadeUtils::SetTypeIndex(std::type_index(typeid(EudaqWriter)));
  auto _test_ = JadeFactory<JadeWriter>::Register<EudaqWriter, const JadeOption&>(typeid(EudaqWriter));
}

void EudaqWriter::SetProducerCallback(eudaq::Producer *producer){
  m_producer = producer;
}

EudaqWriter::EudaqWriter(const JadeOption &opt)
  :JadeWriter(opt), m_producer(nullptr){
}

EudaqWriter::~EudaqWriter(){
}

void EudaqWriter::Open(){
  m_tg_last_send = 0;
  m_tg_expected = 0;
  m_flag_wait_first_event = true;
  m_list_cached_df.clear();

  m_broken_c = 0;
  m_stop_for_debug = 0;
}

void EudaqWriter::Close(){
  
}

void EudaqWriter::Write(JadeDataFrameSP df){
  if(m_stop_for_debug){
    return;
  }

  if(!m_producer){
    std::cerr<<"EudaqWriter: ERROR, eudaq producer is not avalible";
    throw;
  }
  
  df->Decode(1); //level 1, header-only
  std::cout<<"         >> "<< df->GetCounter()<< std::endl;
  
  m_list_cached_df.push_back(df);
  if(m_list_cached_df.size()<6){
    return;
  }
  else{
    //TODO: filter wrong trigger id
    for (std::list<JadeDataFrameSP>::iterator it= ++m_list_cached_df.begin(); it!= (--(--(--m_list_cached_df.end()))); ++it){
      std::list<JadeDataFrameSP>::iterator it_ealier = it;
      --it_ealier;
      std::list<JadeDataFrameSP>::iterator it_later = it;
      ++it_later;
      std::list<JadeDataFrameSP>::iterator it_later_2 = it_later;
      ++it_later_2;
      
      uint16_t tg_ealier = (*it_ealier)->GetCounter();
      uint16_t tg_that = (*it)->GetCounter();
      uint16_t tg_later = (*it_later)->GetCounter();
      uint16_t tg_later_2 = (*it_later_2)->GetCounter();
      
      if( (uint16_t(tg_that-tg_ealier)<1000) + (tg_that!=tg_ealier)+
	  (uint16_t(tg_later-tg_that)<1000) + (uint16_t(tg_later_2-tg_that)<1000) !=4){
	it = m_list_cached_df.erase(it);
	// std::cout<< "                                                                removed "<<tg_that<<std::endl;
	return;
      }
    }
    if(m_list_cached_df.size()<6)
      return;

    df = m_list_cached_df.front();
    m_list_cached_df.pop_front();
  }
  
  uint16_t tg_this = df->GetCounter();
  if(m_flag_wait_first_event){
    m_flag_wait_first_event = false;
    m_tg_expected = tg_this;
  }
  
  if(tg_this != (m_tg_expected & 0xffff)){
    // std::cout<< "expecting<"<<(m_tg_expected & 0xffff)<<">,  provided<"<< tg_this<<">"<<std::endl;
    m_broken_c++;

    if(m_broken_c == 100)
      m_stop_for_debug = true;
      
    uint32_t tg_guess_0 = (m_tg_expected & 0xffff0000) + tg_this;
    uint32_t tg_guess_1 = (m_tg_expected & 0xffff0000) + 0x10000 + tg_this;
    
    if(tg_guess_0 > m_tg_expected && tg_guess_0 - m_tg_expected < 1000){
      m_tg_expected =tg_guess_0;
    }
    else if(tg_guess_1 > m_tg_expected && tg_guess_1 - m_tg_expected < 1000){
      m_tg_expected =tg_guess_1;
    }
    else{
      m_tg_expected ++;
      return;
    }
    
  }

  // std::cout<< "                                                    <"<<(m_tg_expected & 0xffff)<<std::endl;
  m_broken_c = 0;
  
  auto evup_to_send = eudaq::Event::MakeUnique("JadeRaw");
  evup_to_send->SetTriggerN(m_tg_expected);
  evup_to_send->AddBlock<char>( (uint32_t)0, df->Raw().data(), (size_t)(df->Raw().size()) );
  m_producer->SendEvent(std::move(evup_to_send));
  m_tg_last_send = m_tg_expected;
  m_tg_expected++;
}

class JadeProducer : public eudaq::Producer {
public:
  using eudaq::Producer::Producer;
  ~JadeProducer() override {};
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("JadeProducer");
private:
  JadeManagerSP m_jade_man;
};

namespace{
  auto _reg_ = eudaq::Factory<eudaq::Producer>::
    Register<JadeProducer, const std::string&, const std::string&>(JadeProducer::m_id_factory);
}

void JadeProducer::DoInitialise(){
  auto ini = GetInitConfiguration();

  std::string json_str;
  std::string json_base64  = ini->Get("JSON_BASE64", "");
  std::string json_path  = ini->Get("JSON_PATH", "");
  std::string ip_addr= ini->Get("IP_ADDR", ""); 
  
  if(!json_base64.empty()){
    json_str = JadeUtils::Base64_atob(json_base64);
  }
  else if(!json_path.empty()){
    json_str = JadeUtils::LoadFileToString(json_path);
  }
  else if(!ip_addr.empty()){
    json_str +="{\"JadeManager\":{\"type\":\"JadeManager\",\"parameter\":{\"version\":3,\"JadeRegCtrl\":{\"type\":\"AltelRegCtrl\",\"parameter\":{\"IP_ADDRESS\": \"";
    json_str +=ip_addr;
    json_str +="\",\"IP_UDP_PORT\": 4660}},\"JadeReader\":{\"type\":\"AltelReader\",\"parameter\":{\"IS_DISK_FILE\":false,\"TERMINATE_AT_FILE_END\":true,\"FILE_PATH\":\"nothing\",\"IP_ADDRESS\": \"";
    json_str +=ip_addr;
    json_str +="\",\"IP_TCP_PORT\":24}},\"JadeWriter\":{\"type\":\"EudaqWriter\",	\"parameter\":{\"nothing\":\"nothing\"}}}}}";
  }
  else{
    std::cerr<<"JadeProducer: no ini section for "<<GetFullName()<<std::endl;
    std::cerr<<"terminating..."<<std::endl;
    std::terminate();
  }
  
  JadeOption opt_conf(json_str);
  JadeOption opt_man = opt_conf.GetSubOption("JadeManager");
  std::cout<<opt_man.DumpString()<<std::endl;
  std::string man_type = opt_man.GetStringValue("type");
  JadeOption opt_man_para = opt_man.GetSubOption("parameter");
  m_jade_man = JadeManager::Make(man_type, opt_man_para);

  m_jade_man->Init();
  auto writer = m_jade_man->GetWriter();
  auto eudaq_writer = std::dynamic_pointer_cast<EudaqWriter>(writer);
  if(!eudaq_writer){
    std::cerr<<"JadeProducer: there is no instance of EudaqWriter in "<<man_type<<std::endl;
  }
  else{
    eudaq_writer->SetProducerCallback(this);
  }
}

void JadeProducer::DoConfigure(){
  //do nothing here
}

void JadeProducer::DoStartRun(){
  m_jade_man->StartDataTaking();
}

void JadeProducer::DoStopRun(){
  m_jade_man->StopDataTaking();
}

void JadeProducer::DoReset(){
  m_jade_man.reset();
}

void JadeProducer::DoTerminate(){
  m_jade_man.reset();
  std::terminate();
}
