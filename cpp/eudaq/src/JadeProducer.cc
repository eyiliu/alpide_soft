#define WIN32_LEAN_AND_MEAN

#include "JadeCore.hh"

#include "eudaq/Producer.hh"

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

  uint32_t ExtendTriggerN(uint16_t tg_l15);
  void SetProducerCallback(eudaq::Producer *producer);
private:
  eudaq::Producer *m_producer;
  JadeDataFrameSP m_broken_df;
  uint32_t m_tg_expected;
  uint32_t m_tg_last_send;
  uint32_t m_flag_wait_first_event;
  uint32_t m_broken_continue_c;
  
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
  m_broken_continue_c = 0;
  m_flag_wait_first_event = true;
  m_broken_df.reset();
}

void EudaqWriter::Close(){
  
}

void EudaqWriter::Write(JadeDataFrameSP df){
  if(!m_producer){
    std::cerr<<"EudaqWriter: ERROR, eudaq producer is not avalible";
    throw;
  }
  
  df->Decode(1); //level 1, header-only
  
  uint16_t tg_l15 = 0x7fff & df->GetCounter();
  if(m_flag_wait_first_event){
    m_flag_wait_first_event = false;
    m_tg_expected = tg_l15;
    m_broken_df.reset();
  }
  if(tg_l15 != (m_tg_expected & 0x7fff)){
    if(tg_l15 <= 8){
      if(m_broken_df)
	m_broken_continue_c++;
      m_broken_df = df;
      m_tg_expected ++;
      return;
    }
    uint32_t tg_guess_0 = (m_tg_expected & 0xffff8000) + tg_l15;
    uint32_t tg_guess_1 = (m_tg_expected & 0xffff8000) + 0x8000 + tg_l15;
    if(tg_guess_0 > m_tg_expected && tg_guess_0 - m_tg_expected < 20){
      m_tg_expected =tg_guess_0;
    }
    else if (tg_guess_1 > m_tg_expected && tg_guess_1 - m_tg_expected < 20){
      m_tg_expected =tg_guess_1;
    }
    else{
      if(m_broken_df)
	m_broken_continue_c++;
      m_broken_df = df;
      std::cout<< "expecting : provided "<< (m_tg_expected & 0x7fff) << " : "<< tg_l15<<std::endl;
      m_tg_expected ++;
      return;
    }
  }
  
  if(m_broken_df ){
    if(m_tg_last_send+2 == m_tg_expected){
      auto evup_to_send = eudaq::Event::MakeUnique("JadeRaw");
      evup_to_send->SetTriggerN(m_tg_last_send+1);
      evup_to_send->AddBlock<char>( (uint32_t)0, m_broken_df->Raw().data(), (size_t)(m_broken_df->Raw().size()) );
      m_producer->SendEvent(std::move(evup_to_send));
      // std::cout<< "recover broken dataframe.  TLUID_l15 last:dropped:current <"
      // 	       <<(0x7fff&m_tg_last_send)<<":"<<(0x7fff&(m_tg_last_send+1))<<":"<<(0x7fff&m_tg_expected)
      // 	       <<">" <<std::endl;
    }
    else{
      std::cout<< "Unable to recover broken dataframe, dropped.  TLUID_l15 last:dropped:current <"
	       <<(0x7fff&m_tg_last_send)<<":"<<(0x7fff&m_broken_df->GetCounter())<<":"<<(0x7fff&m_tg_expected)
	       <<">" <<std::endl;
      if((0x7fff&m_broken_df->GetCounter())-(0x7fff&m_tg_last_send)==1){
	std::cout<<m_tg_last_send <<" "<<m_tg_expected<<std::endl;
      } 
    }
  }
  auto evup_to_send = eudaq::Event::MakeUnique("JadeRaw");
  evup_to_send->SetTriggerN(m_tg_expected);
  evup_to_send->AddBlock<char>( (uint32_t)0, df->Raw().data(), (size_t)(df->Raw().size()) );
  m_producer->SendEvent(std::move(evup_to_send));
  m_broken_df.reset();
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
  std::string ip_addr= ini->Get("IP_ADDR", "192.168.22.20");
  std::string json_str;
  json_str +="{\"JadeManager\":{\"type\":\"JadeManager\",\"parameter\":{\"version\":3,\"JadeRegCtrl\":{\"type\":\"AltelRegCtrl\",\"parameter\":{\"IP_ADDRESS\": \"";
  json_str +=ip_addr;
  json_str +="\",\"IP_UDP_PORT\": 4660}},\"JadeReader\":{\"type\":\"AltelReader\",\"parameter\":{\"IS_DISK_FILE\":false,\"TERMINATE_AT_FILE_END\":true,\"FILE_PATH\":\"nothing\",\"IP_ADDRESS\": \"";
  json_str +=ip_addr;
  json_str +="\",\"IP_TCP_PORT\":24}},\"JadeWriter\":{\"type\":\"EudaqWriter\",	\"parameter\":{\"nothing\":\"nothing\"}}}}}";
   
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
