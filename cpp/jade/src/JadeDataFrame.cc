#include "JadeDataFrame.hh"
#include "JadeFactory.hh"
#include "JadeUtils.hh"

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#endif


using _base_c_ = JadeDataFrame;
using _index_c_ = JadeDataFrame;

// template class DLLEXPORT JadeFactory<_base_c_>;
// template DLLEXPORT
// std::unordered_map<std::type_index, typename JadeFactory<_base_c_>::UP (*)(const JadeOption&)>&
// JadeFactory<_base_c_>::Instance<const JadeOption&>();

// namespace{
//   auto _loading_index_ = JadeUtils::SetTypeIndex(std::type_index(typeid(_index_c_)));
//   auto _loading_ = JadeFactory<_base_c_>::Register<_base_c_, const JadeOption&>(typeid(_index_c_));
// }

JadeDataFrame::JadeDataFrame(std::string&& data)
  : m_data_raw(std::move(data))
  , m_is_decoded(false)
  , m_n_x(0)
    , m_n_y(0)
{
}

JadeDataFrame::JadeDataFrame(std::string& data)
  : m_data_raw(data)
  , m_is_decoded(false)
  , m_n_x(0)
    , m_n_y(0)
{
}


JadeDataFrame::~JadeDataFrame()
{
}

const std::vector<int16_t>& JadeDataFrame::Data() const
{
  return m_data;
}

const std::string& JadeDataFrame::RawData() const
{
  return m_data_raw;
}

const std::string& JadeDataFrame::Description() const
{
  return m_description;
}

const std::chrono::system_clock::time_point&
JadeDataFrame::TimeStamp() const
{
  return m_ts;
}

std::vector<int16_t>& JadeDataFrame::Data()
{
  return m_data;
}

std::string& JadeDataFrame::RawData()
{
  return m_data_raw;
}

std::string& JadeDataFrame::Description()
{
  return m_description;
}

  std::chrono::system_clock::time_point&
JadeDataFrame::TimeStamp()
{
  return m_ts;
}

uint32_t JadeDataFrame::GetFrameCount() const
{
  return m_frame_n;
}

uint32_t JadeDataFrame::GetMatrixSizeX() const
{
  return m_n_x;
}

uint32_t JadeDataFrame::GetMatrixSizeY() const
{
  return m_n_y;
}

uint32_t JadeDataFrame::GetTriggerN() const
{
  return m_trigger_n;
}

uint32_t JadeDataFrame::GetExtension() const
{
  return m_extension;
} // 

void JadeDataFrame::Decode(){
  m_is_decoded = true;
  const uint8_t* p_raw_beg = reinterpret_cast<const uint8_t *>(m_data_raw.data());
  const uint8_t* p_raw_end = p_raw_beg + m_data_raw.size();
  const uint8_t* p_raw = p_raw_beg;
  if(m_data_raw.size()<7){
    std::cerr << "JadeDataFrame: raw data length is less than 7\n";
    throw;
  }
  if( *p_raw_beg!=0x5a || *(p_raw_end-1)!=0xa5){
    std::cerr << "JadeDataFrame: pkg header/trailer mismatch\n";
    std::cerr << JadeUtils::ToHexString(m_data_raw)<<std::endl;
    std::cerr <<uint16_t((*p_raw_beg))<<std::endl;
    std::cerr <<uint16_t((*(p_raw_end-1)))<<std::endl;
    throw;
  }
  uint32_t len_payload_data = BE32TOH(*reinterpret_cast<const uint32_t*>(p_raw)) & 0x000fffff;
  if (len_payload_data/2 + 7 != m_data_raw.size()) {
    std::cerr << "JadeDataFrame: raw data length does not match\n";
    std::cerr << len_payload_data<<std::endl;
    std::cerr << m_data_raw.size()<<std::endl;
    throw;
  }  
  p_raw += 4;
  m_trigger_n = BE16TOH(*reinterpret_cast<const uint16_t*>(p_raw));
  p_raw += 2;
  m_data.clear();
  m_data.resize(1024 * 512, 0);
  uint8_t l_frame_n = -1;
  uint8_t l_region_id = -1;
  while(p_raw < p_raw_end){
    char d = *p_raw;
    if(d & 0b10000000){
      //NOT DATA 1
      if(d & 0b01000000){
        //empty or region header or busy_on/off 11
        if(d & 0b00100000){
          //emtpy or busy_on/off 111
          if(d & 0b00010000){
            //busy_on/off
            p_raw++;
            continue;
          }
          // empty 1110
          uint8_t chip_id = d & 0b00001111;
          l_frame_n++;
          p_raw++;
          d = *p_raw;
          uint8_t bunch_counter_h = d;
          p_raw++;
          continue;
        }
        // region header 110
        l_region_id = d & 0b00011111;
        p_raw++;
        continue;
      }
      //CHIP_HEADER/TRAILER or undefined 10
      if(d & 0b00100000){
        //CHIP_HEADER/TRAILER 101
        if(d & 0b000100000){
          //TRAILER 1011
          uint8_t readout_flag= d & 0b00001111;
          p_raw++;
          continue;
        }
        //HEADER 1010
        uint8_t chip_id = d & 0b00001111;
        l_frame_n++;
        p_raw++;
        d = *p_raw;
        uint8_t bunch_counter_h = d;
        p_raw++;
        continue;
      }
      //undefined 100
      p_raw++;
      continue;
    }
    else{
      //DATA 0
      if(d & 0b01000000){
        //DATA SHORT 01
        uint8_t encoder_id = (d & 0b00111100)>> 2;
        uint16_t addr = (d & 0b00000011)<<8;
        p_raw++;
        d = *p_raw;
        addr += *p_raw;
        p_raw++;

        uint16_t y = addr>>1;
        uint16_t x = (l_region_id<<5)+(encoder_id<<1)+((addr&0b1)==(addr>>1&0b1));
        m_data[x+1024*y] |= (1<<l_frame_n);
        continue;
      }
      //DATA LONG 00
      uint8_t encoder_id = (d & 0b00111100)>> 2;
      uint16_t addr = (d & 0b00000011)<<8;
      p_raw++;
      d = *p_raw;
      addr += *p_raw;
      p_raw++;
      d = *p_raw;
      uint8_t hit_map = (d & 0b01111111);
      p_raw++;

      uint16_t y = addr>>1;
      uint16_t x = (l_region_id<<5)+(encoder_id<<1)+((addr&0b1)==(addr>>1&0b1));
      m_data[x+1024*y] |= (1<<l_frame_n);
      for(int i=1; i<=7; i++){
        if(hit_map & (1<(i-1))){
          addr+= i;
          uint16_t y = addr>>1;
          uint16_t x = (l_region_id<<5)+(encoder_id<<1)+((addr&0b1)==(addr>>1&0b1));
          m_data[x+1024*y] |= (1<<l_frame_n);
        }
      }
      continue;
    }
  }
  return;
}

int16_t JadeDataFrame::GetHitValue(size_t x, size_t y) const
{
  size_t pos = x + m_n_x * y;
  int16_t val = m_data.at(pos);
  return val;
}

void JadeDataFrame::Print(std::ostream& os, size_t ws) const
{
  os << std::string(ws, ' ') << "{ name:JadeDataFrame,\n";
  os << std::string(ws + 2, ' ') << "is_decoded:" << m_is_decoded << ",\n";
  if (m_is_decoded) {
    os << std::string(ws + 2, ' ') << "description:"
      << "TODO"
      << ",\n";
    os << std::string(ws + 2, ' ') << "ts:"
      << "TODO"
      << ",\n";
    os << std::string(ws + 2, ' ') << "frame_n:" << m_frame_n << ",\n";
    os << std::string(ws + 2, ' ') << "n_x:" << m_n_x << ",\n";
    os << std::string(ws + 2, ' ') << "n_y:" << m_n_y << ",\n";
    if (m_n_x != 0 && m_n_y != 0) {
      os << std::string(ws + 2, ' ') << "data:[\n";
      for (size_t iy = 0; iy < m_n_y; iy++) {
        os << std::string(ws + 4, ' ') << "{row_y:" << iy
          << ",value:[" << GetHitValue(0, 0);
        for (size_t ix = 1; ix < m_n_x; ix++) {
          os << "," << GetHitValue(ix, iy);
        }
        os << "]}\n";
      }
      os << std::string(ws + 2, ' ') << "]\n";
    }
  }
  os << std::string(ws, ' ') << "}\n";
}
