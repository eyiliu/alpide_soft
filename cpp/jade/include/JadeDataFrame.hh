#ifndef JADEPIX_JADEDATAFRAME
#define JADEPIX_JADEDATAFRAME

#include "JadeSystem.hh"
#include <algorithm>
#include <chrono>
#include <functional>
#include <map>
#include <string>
#include <vector>

class JadeDataFrame;
using JadeDataFrameSP = std::shared_ptr<JadeDataFrame>;

class DLLEXPORT JadeDataFrame {
  public:
  JadeDataFrame(std::string& data);
  JadeDataFrame(std::string&& data);
  // JadeDataFrame(size_t nraw);
  JadeDataFrame() = delete;
  // JadeDataFrame();
  virtual ~JadeDataFrame();
  virtual void Decode();

  //const version
  const std::chrono::system_clock::time_point& TimeStamp() const;
  const std::vector<int16_t>& Data() const;
  const std::string& RawData() const;
  const std::string& Description() const;

  //none const version
  std::chrono::system_clock::time_point& TimeStamp();
  std::vector<int16_t>& Data();
  std::string& RawData();
  std::string& Description();

  int16_t GetHitValue(size_t x, size_t y) const;
  uint32_t GetFrameCount() const;
  uint32_t GetMatrixSizeX() const; //x row, y column
  uint32_t GetMatrixSizeY() const;
  uint32_t GetTriggerN() const;
  uint32_t GetExtension() const;
  void Print(std::ostream& os, size_t ws = 0) const;

 private:
  bool m_is_decoded;
  std::string m_data_raw;
  std::string m_description;
  std::chrono::system_clock::time_point m_ts;
  uint32_t m_trigger_n;
  uint16_t m_extension;
  uint32_t m_frame_n;
  uint16_t m_n_x;
  uint16_t m_n_y;
  std::vector<int16_t> m_data;
};

#endif
