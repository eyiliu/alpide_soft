#define WIN32_LEAN_AND_MEAN

#include "JadeCore.hh"
#include "EudaqWriter.hh"

#include "eudaq/Producer.hh"

#include <list>
#include <iostream>
#include <chrono>
#include <thread>

class EudaqWriter_v2: public EudaqWriter {
public:
  using EudaqWriter::EudaqWriter;
  ~EudaqWriter_v2()override {};
  void Write(JadeDataFrameSP df) override;
  void Open() override;
  void Close() override;

  void SetProducerCallback(eudaq::Producer *producer) override;
private:
  eudaq::Producer *m_producer{nullptr};
  JadeDataFrameSP m_broken_df;
  uint32_t m_tg_expected;
  uint32_t m_tg_last_send;
  uint32_t m_flag_wait_first_event;
  uint32_t m_broken_continue_c;
  
};

//+++++++++++++++++++++++++++++++++++++++++
//TestWrite.cc
namespace{
  auto _test_index_ = JadeUtils::SetTypeIndex(std::type_index(typeid(EudaqWriter_v2)));
  auto _test_ = JadeFactory<JadeWriter>::Register<EudaqWriter_v2, const JadeOption&>(typeid(EudaqWriter_v2));
}

void EudaqWriter_v2::SetProducerCallback(eudaq::Producer *producer){
  m_producer = producer;
}


void EudaqWriter_v2::Open(){
  m_tg_last_send = 0;
  m_tg_expected = 0;
  m_broken_continue_c = 0;
  m_flag_wait_first_event = true;
  m_broken_df.reset();
}

void EudaqWriter_v2::Close(){
  
}

void EudaqWriter_v2::Write(JadeDataFrameSP df){
  if(!m_producer){
    std::cerr<<"EudaqWriter_v2: ERROR, eudaq producer is not avalible";
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
