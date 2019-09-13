#define WIN32_LEAN_AND_MEAN

#include "JadeCore.hh"
#include "EudaqWriter.hh"

#include "eudaq/Producer.hh"

#include <list>
#include <iostream>
#include <chrono>
#include <thread>


class EudaqWriter_v1: public EudaqWriter {
public:
  EudaqWriter_v1(const JadeOption &opt);
  ~EudaqWriter_v1() override;
  void Write(JadeDataFrameSP df) override;
  void Open() override;
  void Close() override;
  void SetProducerCallback(eudaq::Producer *producer) override;
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
  auto _test_index_ = JadeUtils::SetTypeIndex(std::type_index(typeid(EudaqWriter_v1)));
  auto _test_ = JadeFactory<JadeWriter>::Register<EudaqWriter_v1, const JadeOption&>(typeid(EudaqWriter_v1));
}

void EudaqWriter_v1::SetProducerCallback(eudaq::Producer *producer){
  m_producer = producer;
}

EudaqWriter_v1::EudaqWriter_v1(const JadeOption &opt)
  :EudaqWriter(opt), m_producer(nullptr){
}

EudaqWriter_v1::~EudaqWriter_v1(){
}

void EudaqWriter_v1::Open(){
  m_tg_last_send = 0;
  m_tg_expected = 0;
  m_flag_wait_first_event = true;
  m_list_cached_df.clear();

  m_broken_c = 0;
  m_stop_for_debug = 0;
}

void EudaqWriter_v1::Close(){
  
}

void EudaqWriter_v1::Write(JadeDataFrameSP df){
  if(m_stop_for_debug){
    return;
  }

  if(!m_producer){
    std::cerr<<"EudaqWriter_v1: ERROR, eudaq producer is not avalible";
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
