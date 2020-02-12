#include "Telescope.hh"

#include <regex>

using namespace std::chrono_literals;

void Layer::fw_start(){
  if(!m_fw) return;
  m_fw->SetAlpideRegister("CHIP_MODE", 0x3d); //trigger MODE
  m_fw->SendAlpideBroadcast("RORST"); //Readout (RRU/TRU/DMU) reset
  m_fw->SetFirmwareRegister("FIRMWARE_MODE", 1); //run ext trigger
}

void Layer::fw_stop(){
  if(!m_fw) return;
  m_fw->SetAlpideRegister("CHIP_MODE", 0x3c); // configure mode
  m_fw->SetFirmwareRegister("FIRMWARE_MODE", 0);
}
  
void Layer::fw_init(){
  if(!m_fw) return;
  //=========== init part ========================
  // 3.8 Chip initialization
  // GRST
  m_fw->SetFirmwareRegister("FIRMWARE_MODE", 0);
  m_fw->SetFirmwareRegister("ADDR_CHIP_ID", 0x10); //OB
  m_fw->SendAlpideBroadcast("GRST"); // chip global reset
  m_fw->SetAlpideRegister("CHIP_MODE", 0x3c); // configure mode
  // DAC setup
  m_fw->SetAlpideRegister("VRESETP", 0x75); //117
  m_fw->SetAlpideRegister("VRESETD", 0x93); //147
  m_fw->SetAlpideRegister("VCASP", 0x56);   //86
  m_fw->SetAlpideRegister("VCASN", 0x32);   //57 Y50
  m_fw->SetAlpideRegister("VPULSEH", 0xff); //255 
  m_fw->SetAlpideRegister("VPULSEL", 0x0);  //0
  m_fw->SetAlpideRegister("VCASN2", 0x39);  //62 Y63
  m_fw->SetAlpideRegister("VCLIP", 0x0);    //0
  m_fw->SetAlpideRegister("VTEMP", 0x0);
  m_fw->SetAlpideRegister("IAUX2", 0x0);
  m_fw->SetAlpideRegister("IRESET", 0x32);  //50
  m_fw->SetAlpideRegister("IDB", 0x40);     //64
  m_fw->SetAlpideRegister("IBIAS", 0x40);   //64
  m_fw->SetAlpideRegister("ITHR", 40);    //51  empty 0x32; 0x12 data, not full.  0x33 default, threshold
  // 3.8.1 Configuration of in-pixel logic
  m_fw->SendAlpideBroadcast("PRST");  //pixel matrix reset
  m_fw->BroadcastPixelRegister("MASK_EN", 0);
  m_fw->BroadcastPixelRegister("PULSE_EN", 0);
  // m_fw->SendAlpideBroadcast("PRST");  //pixel matrix reset
  // 3.8.2 Configuration and start-up of the Data Transmission Unit, PLL
  m_fw->SetAlpideRegister("DTU_CONF", 0x008d);
  m_fw->SetAlpideRegister("DTU_DAC",  0x0088);
  m_fw->SetAlpideRegister("DTU_CONF", 0x0085);
  m_fw->SetAlpideRegister("DTU_CONF", 0x0185);
  m_fw->SetAlpideRegister("DTU_CONF", 0x0085);
  // 3.8.3 Setting up of readout
  // 3.8.3.1a (OB) Setting CMU and DMU Configuration Register
  // Previous Chip
  m_fw->SetAlpideRegister("CMU_DMU_CONF", 0x70); //disable MCH encoding, enable DDR, no previous OB
  m_fw->SetAlpideRegister("TEST_CTRL", 0x400); //Disable Busy Line
  // Initial Token 1
  // 3.8.3.2 Setting FROMU Configuration Registers and enabling readout mode
  // FROMU Configuration Register 1,2
  m_fw->SetAlpideRegister("FROMU_CONF_1", 0x00); //Disable external busy
  m_fw->SetAlpideRegister("FROMU_CONF_2", 156); //STROBE duration
  // FROMU Pulsing Register 1,2
  m_fw->SetAlpideRegister("FROMU_PULSING_2", 0xffff); //yiliu: test pulse duration, max  
  // Periphery Control Register (CHIP MODE)
  m_fw->SetAlpideRegister("CHIP_MODE", 0x3d); //trigger MODE
  // RORST 
  m_fw->SendAlpideBroadcast("RORST"); //Readout (RRU/TRU/DMU) reset

  //===========end of init part =====================
  m_fw->SetFirmwareRegister("TRIG_DELAY", 100); //25ns per dig (FrameDuration?)
  m_fw->SetFirmwareRegister("GAP_INT_TRIG", 20);
}


uint64_t Layer::AsyncPushBack(){ // IMPROVE IT AS A RING
  if(m_is_async_reading){
    std::cout<< "old AsyncReading() has not been stopped "<<std::endl;
    return 0;
  }
    
  m_vec_ring_ev.clear();
  m_vec_ring_ev.resize(m_size_ring);
  m_count_ring_write = 0;
  m_count_ring_read = 0;
  m_hot_p_read = m_size_ring -1; // tail
    
  m_is_async_reading = true;
  while (m_is_async_reading){
    auto df = m_rd? m_rd->Read(1000ms):nullptr; // TODO: read a vector
    if(!df){
      continue;
    }
    uint64_t next_p_ring_write = m_count_ring_write % m_size_ring;
    if(next_p_ring_write == m_hot_p_read){
      // buffer full, permanent data lose
      continue;
    }
    m_vec_ring_ev[next_p_ring_write] = df;
    m_count_ring_write ++;
  }
  std::cout<< "aysnc exit"<<std::endl;
  return m_count_ring_write;
}
  
JadeDataFrameSP Layer::GetNextCachedEvent(){
  if(m_count_ring_write > m_count_ring_read) {
    uint64_t next_p_ring_read = m_count_ring_read % m_size_ring;
    m_hot_p_read = next_p_ring_read;
    auto ev = std::move(m_vec_ring_ev[next_p_ring_read]);
    // keep hot read to prevent write-overlapping
    m_count_ring_read ++;
    return ev;
  }
  else{
    return m_ring_end;
  }
}

JadeDataFrameSP& Layer::Front(){
  if(m_count_ring_write > m_count_ring_read) {
    uint64_t next_p_ring_read = m_count_ring_read % m_size_ring;
    m_hot_p_read = next_p_ring_read;
    return m_vec_ring_ev[next_p_ring_read];
  }
  else{
    return m_ring_end;
  }
}

void Layer::PopFront(){
  if(m_count_ring_write > m_count_ring_read) {
    uint64_t next_p_ring_read = m_count_ring_read % m_size_ring;
    m_hot_p_read = next_p_ring_read;
    m_vec_ring_ev[next_p_ring_read].reset();
    m_count_ring_read ++;
  }
}

uint64_t Layer::Size(){
  return  m_count_ring_write - m_count_ring_read;
}





Telescope::Telescope(const std::string& file_context){
    rapidjson::Document js_doc;
    js_doc.Parse(file_context.c_str());
    if(js_doc.HasParseError()){
      fprintf(stderr, "JSON parse error: %s (at string positon %u)", rapidjson::GetParseError_En(js_doc.GetParseError()), js_doc.GetErrorOffset());
      throw;
    }

    for (auto& js_layer : js_doc.GetArray()){
      if(!js_layer["disable"].GetBool()){
        std::unique_ptr<FirmwarePortal> fw(new FirmwarePortal(FirmwarePortal::Stringify(js_layer["ctrl_link"])));
        std::unique_ptr<AltelReader> rd(new AltelReader(FirmwarePortal::Stringify(js_layer["data_link"])));
        std::unique_ptr<Layer> l(new Layer);
        l->m_fw.reset(new FirmwarePortal(FirmwarePortal::Stringify(js_layer["ctrl_link"])));
        l->m_rd.reset(new AltelReader(FirmwarePortal::Stringify(js_layer["data_link"])));
        m_vec_layer.push_back(std::move(l));
      }
    }
  }
  
std::vector<JadeDataFrameSP> Telescope::ReadEvent(){
    std::vector<JadeDataFrameSP> ev_sync;
    uint32_t trigger_n = -1;
    for(auto &l: m_vec_layer){
      if( l->Size() == 0)
        return ev_sync;
      else{
        uint32_t trigger_n_ev = l->Front()->GetCounter();
        if(trigger_n_ev< trigger_n)
          trigger_n = trigger_n_ev;
      }
    }

    for(auto &l: m_vec_layer){
      auto &ev_front = l->Front(); 
      if(ev_front->GetCounter() == trigger_n){
        ev_sync.push_back(ev_front);
        l->PopFront();
      }
    }
    
    if(ev_sync.size() < m_vec_layer.size() ){
      std::cout<< "dropped assambed event with subevent less than requried "<< m_vec_layer.size() <<" sub events" <<std::endl;
      std::string dev_numbers;
      for(auto & ev : ev_sync){
	dev_numbers += std::to_string(ev->GetExtension());
	dev_numbers +=" ";
      }
      std::cout<< "  tluID="<<trigger_n<<" subevent= "<< dev_numbers <<std::endl;
      std::vector<JadeDataFrameSP> empty;
      return empty;
    }
    return ev_sync;
  }
