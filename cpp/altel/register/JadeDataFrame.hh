#ifndef JADEPIX_JADEDATAFRAME_WS
#define JADEPIX_JADEDATAFRAME_WS

#include <string>
#include <vector>
#include <memory>

#include "common/mysystem.hh"

class JadeDataFrame;
using JadeDataFrameSP = std::shared_ptr<JadeDataFrame>;
  
class JadeDataFrame {
public:

  JadeDataFrame(std::string& data);
  JadeDataFrame(std::string&& data);
  JadeDataFrame() = delete;
  void Decode(uint32_t level);

  //const version
  const std::string& Raw() const;
  const std::string& Meta() const;

  //none const version
  std::string& Raw();
  std::string& Meta();

  uint32_t GetMatrixDepth() const;
  uint32_t GetMatrixSizeX() const; //x row, y column
  uint32_t GetMatrixSizeY() const;
  uint64_t GetCounter() const;
  uint64_t GetExtension() const;

  const std::string& Data_Flat() const {return m_data_flat; }
  const std::vector<uint16_t>& Data_X() const {return m_data_x; }
  const std::vector<uint16_t>& Data_Y() const {return m_data_y; }
  const std::vector<uint16_t>& Data_D() const {return m_data_d; }
  const std::vector<uint32_t>& Data_T() const {return m_data_t; }
  const std::vector<uint32_t>& Data_V() const {return m_data_v; }
  
  void Print(std::ostream& os, size_t ws = 0) const;

  template <typename W>
  void Serialize(W& w) const {
    w.StartObject();
    {
      w.String("level");
      w.Uint(m_level_decode);
      w.String("trigger");
      w.Uint(m_counter);
      w.String("ext");
      w.Uint(m_extension);

      w.String("geometry");
      w.StartObject();
      {
        w.String("nx");
        w.Uint(m_n_x);
        w.String("ny");
        w.Uint(m_n_y);
        w.String("nz");
        w.Uint(m_n_d);
      }
      w.EndObject();
      
      w.String("data");
      w.StartObject();
      {
        w.String("type");
        w.String("fired_pixels");
        
        w.String("fired_pixels");
        w.StartArray();
        {
          auto it_x = m_data_x.begin();
          auto it_y = m_data_y.begin();
          auto it_z = m_data_d.begin();
          while(it_x!=m_data_x.end()){
            w.StartObject();
            w.String("x");
            w.Uint(*it_x);
            w.String("y");
            w.Uint(*it_y);
            w.String("z");
            w.Uint(*it_z);
          }
        }
        w.EndArray();
      }
      w.EndObject();
    }
    w.EndObject();
  }
  
private:
  std::string m_data_raw;
  std::string m_meta;
  
  uint16_t m_level_decode;  

  uint64_t m_counter;
  uint64_t m_extension;
  
  uint16_t m_n_x;
  uint16_t m_n_y;
  uint16_t m_n_d; //Z
  std::string m_data_flat;

  std::vector<uint16_t> m_data_x;
  std::vector<uint16_t> m_data_y;
  std::vector<uint16_t> m_data_d;
  std::vector<uint32_t> m_data_t;
  std::vector<uint32_t> m_data_v;
  
};

#endif
