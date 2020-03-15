#include "Telescope.hh"

#include <regex>

using namespace std::chrono_literals;

void Layer::fw_start(){
  if(!m_fw) return;
  m_fw->SetAlpideRegister("CMU_DMU_CONF", 0x70);// token
  m_fw->SendAlpideBroadcast("RORST"); //Readout (RRU/TRU/DMU) reset, commit token
  m_fw->SetAlpideRegister("CHIP_MODE", 0x3d); //trigger MODE
  m_fw->SetFirmwareRegister("FIRMWARE_MODE", 1); //run ext trigger
}

void Layer::fw_stop(){
  if(!m_fw) return;
  m_fw->SetAlpideRegister("CHIP_MODE", 0x3c); // configure mode
  m_fw->SetFirmwareRegister("FIRMWARE_MODE", 0);
}

void Layer::fw_init(){
  if(!m_fw) return;
  m_fw->SetFirmwareRegister("TRIG_DELAY", 1); //25ns per dig (FrameDuration?)
  m_fw->SetFirmwareRegister("GAP_INT_TRIG", 20);

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
  m_fw->SetAlpideRegister("ITHR", 51);    //51  empty 0x32; 0x12 data, not full.  0x33 default, threshold
  // 3.8.1 Configuration of in-pixel logic
  m_fw->SendAlpideBroadcast("PRST");  //pixel matrix reset
  m_fw->BroadcastPixelRegister("MASK_EN", 0);
  m_fw->BroadcastPixelRegister("PULSE_EN", 0);
  m_fw->SendAlpideBroadcast("PRST");  //pixel matrix reset
  // 3.8.2 Configuration and start-up of the Data Transmission Unit, PLL
  m_fw->SetAlpideRegister("DTU_CONF", 0x008d); // default
  m_fw->SetAlpideRegister("DTU_DAC",  0x0088); // default
  m_fw->SetAlpideRegister("DTU_CONF", 0x0085); // clear pll disable bit
  m_fw->SetAlpideRegister("DTU_CONF", 0x0185); // set pll reset bit
  m_fw->SetAlpideRegister("DTU_CONF", 0x0085); // clear reset bit
  // 3.8.3 Setting up of readout
  // 3.8.3.1a (OB) Setting CMU and DMU Configuration Register
  m_fw->SetAlpideRegister("CMU_DMU_CONF", 0x70); //Token, disable MCH, enable DDR, no previous OB
  m_fw->SetAlpideRegister("TEST_CTRL", 0x400); //Disable Busy Line
  // 3.8.3.2 Setting FROMU Configuration Registers and enabling readout mode
  // FROMU Configuration Register 1,2
  m_fw->SetAlpideRegister("FROMU_CONF_1", 0x00); //Disable external busy
  m_fw->SetAlpideRegister("FROMU_CONF_2", 40); //STROBE duration
  // FROMU Pulsing Register 1,2
  // m_fw->SetAlpideRegister("FROMU_PULSING_2", 0xffff); //yiliu: test pulse duration, max  
  // Periphery Control Register (CHIP MODE)
  // m_fw->SetAlpideRegister("CHIP_MODE", 0x3d); //trigger MODE
  // RORST 
  // m_fw->SendAlpideBroadcast("RORST"); //Readout (RRU/TRU/DMU) reset, commit token
  //===========end of init part =====================
}

void Layer::rd_start(){
  if(m_is_async_reading){
    std::cout<< "old AsyncReading() has not been stopped "<<std::endl;
    return;
  }

  m_fut_async_rd = std::async(std::launch::async, &Layer::AsyncPushBack, this);

  if(!m_is_async_watching){
    m_fut_async_watch = std::async(std::launch::async, &Layer::AsyncWatchDog, this);
  }
}

void Layer::rd_stop(){
  m_is_async_reading = false;
  if(m_fut_async_rd.valid())
    m_fut_async_rd.get();

  m_is_async_watching = false;
  if(m_fut_async_watch.valid())
    m_fut_async_watch.get();
}

uint64_t Layer::AsyncPushBack(){ // IMPROVE IT AS A RING
  m_vec_ring_ev.clear();
  m_vec_ring_ev.resize(m_size_ring);
  m_count_ring_write = 0;
  m_count_ring_read = 0;
  m_hot_p_read = m_size_ring -1; // tail

  uint32_t tg_expected = 0;
  uint32_t flag_wait_first_event = true;

  m_rd->Open();
  m_is_async_reading = true;
  while (m_is_async_reading){
    auto df = m_rd? m_rd->Read(1000ms):nullptr; // TODO: read a vector
    if(!df){
      continue;
    }

    m_st_n_ev_input_now ++;
    uint64_t next_p_ring_write = m_count_ring_write % m_size_ring;
    if(next_p_ring_write == m_hot_p_read){
      // buffer full, permanent data lose
      m_st_n_ev_overflow_now ++;
      continue;
    }

    df->Decode(3); //level 1, header-only
    uint16_t tg_l15 = 0x7fff & df->GetCounter();
    //std::cout<< "id "<< tg_l15 <<"  ";
    if(flag_wait_first_event){
      flag_wait_first_event = false;
      m_extension = df->GetExtension() ;
      tg_expected = tg_l15;
    }
    if(tg_l15 != (tg_expected & 0x7fff)){
      // std::cout<<(tg_expected & 0x7fff)<< " " << tg_l15<<"\n";
      uint32_t tg_guess_0 = (tg_expected & 0xffff8000) + tg_l15;
      uint32_t tg_guess_1 = (tg_expected & 0xffff8000) + 0x8000 + tg_l15;
      if(tg_guess_0 > tg_expected && tg_guess_0 - tg_expected < 200){
        std::cout<< "missing trigger, expecting : provided "<< (tg_expected & 0x7fff) << " : "<< tg_l15<<" ("<< m_extension <<") \n";
        tg_expected =tg_guess_0;
      }
      else if (tg_guess_1 > tg_expected && tg_guess_1 - tg_expected < 200){
        std::cout<< "missing trigger, expecting : provided "<< (tg_expected & 0x7fff) << " : "<< tg_l15<<" ("<< m_extension <<") \n";
        tg_expected =tg_guess_1;
      }
      else{
        std::cout<< "broken trigger ID, expecting : provided "<< (tg_expected & 0x7fff) << " : "<< tg_l15<<" ("<<df->GetExtension() <<") \n";
        tg_expected ++;
        m_st_n_ev_bad_now ++;
        // permanent data lose
        continue;
      }
    }
    df->SetTrigger(tg_expected-1); //TODO: fix tlu firmware, mismatch between modes
    m_st_n_tg_ev_now = tg_expected-1;
    
    m_vec_ring_ev[next_p_ring_write] = df;
    m_count_ring_write ++;
    tg_expected ++;
  }
  m_rd->Close();
  std::cout<< "aysnc exit"<<std::endl;
  return m_count_ring_write;
}

uint64_t Layer::AsyncWatchDog(){
  m_tp_run_begin = std::chrono::system_clock::now();
  m_tp_old = m_tp_run_begin;
  m_is_async_watching = true;
  while(m_is_async_watching){
    std::this_thread::sleep_for(1s);
    
    uint64_t st_n_tg_ev_now = m_st_n_tg_ev_now;
    uint64_t st_n_ev_input_now = m_st_n_ev_input_now;
    //uint64_t st_n_ev_output_now = m_st_n_ev_output_now;
    uint64_t st_n_ev_bad_now = m_st_n_ev_bad_now;
    uint64_t st_n_ev_overflow_now = m_st_n_ev_overflow_now;

    // time
    auto tp_now = std::chrono::system_clock::now();
    std::chrono::duration<double> dur_period_sec = tp_now - m_tp_old;
    std::chrono::duration<double> dur_accu_sec = tp_now - m_tp_run_begin;
    double sec_period = dur_period_sec.count();
    double sec_accu = dur_accu_sec.count();

    //std::cout<< "sec "<< sec_period<< " : " << sec_accu<<std::endl;

    // period
    uint64_t st_n_tg_ev_period = st_n_tg_ev_now - m_st_n_tg_ev_old;
    uint64_t st_n_ev_input_period = st_n_ev_input_now - m_st_n_ev_input_old;
    uint64_t st_n_ev_bad_period = st_n_ev_bad_now - m_st_n_ev_bad_old;
    uint64_t st_n_ev_overflow_period = st_n_ev_overflow_now - m_st_n_ev_overflow_old;
    
    // ratio
    //double st_output_vs_input_accu = st_n_ev_input_now? st_ev_output_now / st_ev_input_now : 1;
    double st_bad_vs_input_accu = st_n_ev_input_now? 1.0 * st_n_ev_bad_now / st_n_ev_input_now : 0;
    double st_overflow_vs_input_accu = st_n_ev_input_now? 1.0 *  st_n_ev_overflow_now / st_n_ev_input_now : 0;
    double st_input_vs_trigger_accu = st_n_tg_ev_now? 1.0 * st_n_ev_input_now / st_n_tg_ev_now : 1;
    
    //double st_output_vs_input_period = st_ev_input_period? st_ev_output_period / st_ev_input_period : 1;
    double st_bad_vs_input_period = st_n_ev_input_period? 1.0 * st_n_ev_bad_period / st_n_ev_input_period : 0;
    double st_overflow_vs_input_period = st_n_ev_input_period? 1.0 *  st_n_ev_overflow_period / st_n_ev_input_period : 0;
    double st_input_vs_trigger_period = st_n_tg_ev_period? 1.0 *  st_n_ev_input_period / st_n_tg_ev_period : 1;
    
    // hz
    double st_hz_tg_accu = st_n_tg_ev_now / sec_accu ;
    double st_hz_input_accu = st_n_ev_input_now / sec_accu ; 

    double st_hz_tg_period = st_n_tg_ev_period / sec_period ;
    double st_hz_input_period = st_n_ev_input_period / sec_period ;

    std::string st_string_new =
      FirmwarePortal::FormatString("L<%u>: event(%d)/trigger(%d)=Ev/Tr(%.4f) dEv/dTr(%.4f) tr_accu(%.2f hz) ev_accu(%.2f hz) tr_period(%.2f hz) ev_period(%.2f hz)\n",
                                   m_extension, st_n_ev_input_now, st_n_tg_ev_now, st_input_vs_trigger_accu, st_input_vs_trigger_period,
                                   st_hz_tg_accu, st_hz_input_accu, st_hz_tg_period, st_hz_input_period
                                   );
    
    {
      std::unique_lock lk(m_mtx_st);
      m_st_string = std::move(st_string_new);
    }
    
    //write to old
    m_st_n_tg_ev_old = st_n_tg_ev_now;
    m_st_n_ev_input_old = st_n_ev_input_now;
    m_st_n_ev_bad_old = st_n_ev_bad_now;
    m_st_n_ev_overflow_old = st_n_ev_overflow_now;
    m_tp_old = tp_now;
  }
  return 0;
}

std::string  Layer::GetStatusString(){
  std::unique_lock lk(m_mtx_st);
  return m_st_string;
}

JadeDataFrameSP& Layer::Front(){
  if(m_count_ring_write > m_count_ring_read) {
    uint64_t next_p_ring_read = m_count_ring_read % m_size_ring;
    m_hot_p_read = next_p_ring_read;
    // keep hot read to prevent write-overlapping
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
    // keep hot read to prevent write-overlapping
    m_vec_ring_ev[next_p_ring_read].reset();
    m_count_ring_read ++;
  }
}

uint64_t Layer::Size(){
  return  m_count_ring_write - m_count_ring_read;
}

void Layer::ClearBuffer(){
  m_count_ring_write = m_count_ring_read;
  m_vec_ring_ev.clear();
}

Telescope::Telescope(const std::string& file_context){
  rapidjson::Document js_doc;
  js_doc.Parse(file_context.c_str());
  if(js_doc.HasParseError()){
    fprintf(stderr, "JSON parse error: %s (at string positon %u)", rapidjson::GetParseError_En(js_doc.GetParseError()), js_doc.GetErrorOffset());
    throw;
  }

  for (auto& js_layer : js_doc.GetArray()){
    if(js_layer.HasMember("disable") && js_layer["disable"].GetBool()){
      continue;
    }
    std::unique_ptr<FirmwarePortal> fw(new FirmwarePortal(FirmwarePortal::Stringify(js_layer["ctrl_link"])));
    std::unique_ptr<AltelReader> rd(new AltelReader(FirmwarePortal::Stringify(js_layer["data_link"])));
    std::unique_ptr<Layer> l(new Layer);
    l->m_fw.reset(new FirmwarePortal(FirmwarePortal::Stringify(js_layer["ctrl_link"])));
    l->m_rd.reset(new AltelReader(FirmwarePortal::Stringify(js_layer["data_link"])));
    m_vec_layer.push_back(std::move(l));
  }
}

Telescope::~Telescope(){
  Stop();
}

std::vector<JadeDataFrameSP> Telescope::ReadEvent(){
  std::vector<JadeDataFrameSP> ev_sync;
  if (!m_is_running) return ev_sync;  
  
  uint32_t trigger_n = -1;
  for(auto &l: m_vec_layer){
    if( l->Size() == 0){
      // TODO check cached size of all layers
      return ev_sync;
    }
    else{
      uint32_t trigger_n_ev = l->Front()->GetTrigger();
      if(trigger_n_ev< trigger_n)
        trigger_n = trigger_n_ev;
    }
  }

  for(auto &l: m_vec_layer){
    auto &ev_front = l->Front(); 
    if(ev_front->GetTrigger() == trigger_n){
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

void Telescope::Start(){
  for(auto & l: m_vec_layer){
    l->fw_init();
  }
  std::cout<< "fw_init"<<std::endl;
  for(auto & l: m_vec_layer){
    l->rd_start();
  }
  std::cout<< "rd_start"<<std::endl;
  for(auto & l: m_vec_layer){
    l->fw_start();
  }
  std::cout<< "fw_start"<<std::endl;

  if(!m_is_async_watching){
    m_fut_async_watch = std::async(std::launch::async, &Telescope::AsyncWatchDog, this);
  }
  
  m_fut_async_rd = std::async(std::launch::async, &Telescope::AsyncRead, this);
  m_is_running = true;
}

void Telescope::Start_no_tel_reading(){
  for(auto & l: m_vec_layer){
    l->fw_init();
  }
  for(auto & l: m_vec_layer){
    l->rd_start();
  }
  for(auto & l: m_vec_layer){
    l->fw_start();
  }
  if(!m_is_async_watching){
    m_fut_async_watch = std::async(std::launch::async, &Telescope::AsyncWatchDog, this);
  }
  //m_fut_async_rd = std::async(std::launch::async, &Telescope::AsyncRead, this);  
  m_is_running = true;

}

void Telescope::Stop(){
  m_is_async_reading = false;
  if(m_fut_async_rd.valid())
    m_fut_async_rd.get();

  m_is_async_watching = false;
  if(m_fut_async_watch.valid())
    m_fut_async_watch.get();
  
  for(auto & l: m_vec_layer){
    //l->fw_stop(); // commment out, unable to restart after fw_stop.
  }
  
  for(auto & l: m_vec_layer){
    l->rd_stop();
  }

  for(auto & l: m_vec_layer){
    l->ClearBuffer();
  }
  
  m_is_running = false;
}

uint64_t Telescope::AsyncRead(){
  auto now = std::chrono::system_clock::now();
  auto now_c = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss<<std::put_time(std::localtime(&now_c), "%y%m%d%H%M%S");
  std::string now_str = ss.str();
  std::string data_path = "data/alpide_"+now_str+".json";
  FILE* fd = fopen(data_path.c_str(), "wb");
  rapidjson::StringBuffer js_sb;
  rapidjson::Writer<rapidjson::StringBuffer> js_writer;
  
  uint64_t n_ev = 0;
  
  m_is_async_reading = true;
  while (m_is_async_reading){
    auto ev = ReadEvent();
    if(ev.empty()){
      std::this_thread::sleep_for(100us);
      continue;
    }
    n_ev ++;
    m_st_n_ev ++;
    //continue;
    js_sb.Clear();
    js_writer.Reset(js_sb);
    js_writer.StartArray();
    for(auto& e: ev){
      e->Serialize(js_writer);
      rapidjson::PutN(js_sb, '\n', 1);
    }
    js_writer.EndArray();
    rapidjson::PutN(js_sb, '\n', 1);
    auto p_ch = js_sb.GetString();
    auto n_ch = js_sb.GetSize();
    std::fwrite(reinterpret_cast<const char *>(p_ch), 1, n_ch, fd);
  }
  
  
  
  std::cout<< "aysnc exit, from Telescope::AsyncRead"<<std::endl;
  fclose(fd);
  return n_ev;
}

uint64_t Telescope::AsyncWatchDog(){
  m_is_async_watching = true;
  while(m_is_async_watching){
    std::this_thread::sleep_for(1s);
    for(auto &l: m_vec_layer){
      std::cout<< l->GetStatusString();
    }
    uint64_t st_n_ev = m_st_n_ev;
    printf("Tele: event(%u)\n\n", st_n_ev);
  }
  //sleep and watch running time status;
  return 0;
}
