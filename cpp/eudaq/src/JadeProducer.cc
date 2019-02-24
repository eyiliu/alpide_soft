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
  uint32_t m_tg_h17;
  uint16_t m_last_tg_l15;
  
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
  :JadeWriter(opt), m_producer(nullptr), m_tg_h17(0), m_last_tg_l15(0){
  
}

EudaqWriter::~EudaqWriter(){
}

void EudaqWriter::Open(){
  m_tg_h17 = 0;
  m_last_tg_l15 = 0;
}

void EudaqWriter::Close(){
  
}

void EudaqWriter::Write(JadeDataFrameSP df){
  if(!m_producer){
    std::cerr<<"EudaqWriter: ERROR, eudaq producer is not avalible";
    throw;
  }
  
  auto evup_to_send = eudaq::Event::MakeUnique("JadeRaw");
  df->Decode(1); //level 1, header-only
  
  uint16_t tg_l15 = 0x7fff & df->GetCounter();
  if(tg_l15 < m_last_tg_l15 && m_last_tg_l15>0x6000 && tg_l15<0x2000){
    m_tg_h17++;
    EUDAQ_INFO("increase high 17bits of trigger number, last_tg_l15("+ 
	       std::to_string(m_last_tg_l15)+") tg_l15("+ 
	       std::to_string(tg_l15)+")" );
  }
  uint32_t tg_n = (m_tg_h17<<15) + tg_l15;
  m_last_tg_l15 = tg_l15;
  
  evup_to_send->SetTriggerN(tg_n);
  evup_to_send->AddBlock<char>( (uint32_t)0, df->Raw().data(), (size_t)(df->Raw().size()) );
  m_producer->SendEvent(std::move(evup_to_send));  
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
  std::string json_base64  = ini->Get("JSON_BASE64", "");
  std::string json_str;
  if(json_base64.empty()){
    std::string json_path  = ini->Get("JSON_PATH", "");
    json_str = JadeUtils::LoadFileToString(json_path);
  }
  else{
    json_str = JadeUtils::Base64_atob(json_base64);
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
