#include "eudaq/DataCollector.hh"

//#include <mutex>
//#include <deque>
//#include <map>
#include <set>

namespace eudaq {
  class AlpideDataCollector:public DataCollector{
    public:
      AlpideDataCollector(const std::string &name,
          const std::string &rc);
      void DoConnect(ConnectionSPC id) override;
      void DoDisconnect(ConnectionSPC id) override;
      void DoConfigure() override;
      void DoReset() override;
      void DoReceive(ConnectionSPC id, EventSP ev) override;
      static const uint32_t m_id_factory = cstr2hash("AlpideDataCollector");

    private:
      std::mutex m_mtx_map;
      std::map<ConnectionSPC, std::deque<EventSPC>> m_conn_evque;
      std::set<ConnectionSPC> m_conn_inactive;
      uint32_t m_noprint;
      uint32_t m_minimum_sub_event;
  };

  namespace{
    auto dummy0 = Factory<DataCollector>::
      Register<AlpideDataCollector, const std::string&, const std::string&>
      (AlpideDataCollector::m_id_factory);
  }

  AlpideDataCollector::AlpideDataCollector(const std::string &name,
      const std::string &rc):
    DataCollector(name, rc){
    std::cout<< " >>>>>>>>>>>>>>>>>>>>>> It is AlpideDataCollector"<<std::endl;
  }

  void AlpideDataCollector::DoConnect(ConnectionSPC idx){
    std::unique_lock<std::mutex> lk(m_mtx_map);
    m_conn_evque[idx].clear();
    m_conn_inactive.erase(idx);
  }

  void AlpideDataCollector::DoDisconnect(ConnectionSPC idx){
    std::unique_lock<std::mutex> lk(m_mtx_map);
    m_conn_inactive.insert(idx);
    if(m_conn_inactive.size() == m_conn_evque.size()){
      m_conn_inactive.clear();
      m_conn_evque.clear();
    }
  }

  void AlpideDataCollector::DoConfigure(){
    m_noprint = 0;
    auto conf = GetConfiguration();
    if(conf){
      conf->Print();
      m_noprint = conf->Get("DISABLE_PRINT", 0);
      m_minimum_sub_event = conf->Get("MINIMUM_SUB_EVENT", 0);
    }
  }

  void AlpideDataCollector::DoReset(){
    std::unique_lock<std::mutex> lk(m_mtx_map);
    m_noprint = 0;
    m_conn_evque.clear();
    m_conn_inactive.clear();
  }

  void AlpideDataCollector::DoReceive(ConnectionSPC idx, EventSP evsp){
    std::unique_lock<std::mutex> lk(m_mtx_map);
    if(!evsp->IsFlagTrigger()){
      EUDAQ_THROW("!evsp->IsFlagTrigger()");
    }

    //evsp->Print(std::cout);
    // std::cout<< evsp->GetDescription() << " : "<< evsp->GetTriggerN()<<std::endl;
    m_conn_evque[idx].push_back(evsp);

    uint32_t trigger_n = -1;
    for(auto &conn_evque: m_conn_evque){
      if(conn_evque.second.empty())
        return;
      else{
        uint32_t trigger_n_ev = conn_evque.second.front()->GetTriggerN();
        if(trigger_n_ev< trigger_n)
          trigger_n = trigger_n_ev;
      }
    }

    auto ev_sync = Event::MakeUnique("AlpideSyncByTriggerN");
    ev_sync->SetFlagPacket();
    ev_sync->SetTriggerN(trigger_n);
    for(auto &conn_evque: m_conn_evque){
      auto &ev_front = conn_evque.second.front(); 
      if(ev_front->GetTriggerN() == trigger_n){
        ev_sync->AddSubEvent(ev_front);
        conn_evque.second.pop_front();
      }
    }

    if(!m_conn_inactive.empty()){
      std::set<ConnectionSPC> conn_inactive_empty;
      for(auto &conn: m_conn_inactive){
        if(m_conn_evque.find(conn) != m_conn_evque.end() && 
            m_conn_evque[conn].empty()){
          m_conn_evque.erase(conn);
          conn_inactive_empty.insert(conn);	
        }
      }
      for(auto &conn: conn_inactive_empty){
        m_conn_inactive.erase(conn);
      }
    }
    if(!m_noprint)
      ev_sync->Print(std::cout);
    if(ev_sync->GetNumSubEvent() < m_minimum_sub_event){
      std::cout<< "dropped assambed event with subevent less than requried "<< m_minimum_sub_event <<" sub events" <<std::endl;
      std::string dev_numbers;
      for(int i=0; i< ev_sync->GetNumSubEvent(); i++){
	dev_numbers += std::to_string(ev_sync->GetSubEvent(i)->GetDeviceN());
	dev_numbers +=" ";
      }
      std::cout<< "  tluID="<<trigger_n<<" subevent= "<< dev_numbers <<std::endl;
      return;
    }
    WriteEvent(std::move(ev_sync));
  }
}
