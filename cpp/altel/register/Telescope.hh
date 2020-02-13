#ifndef _TELESCOPE_HH_
#define _TELESCOPE_HH_

#include <mutex>
#include <future>

#include "FirmwarePortal.hh"
#include "AltelReader.hh"

class Layer{
public:
  std::unique_ptr<FirmwarePortal> m_fw;
  std::unique_ptr<AltelReader> m_rd;
  std::future<uint64_t> m_fut_async_rd;
  std::mutex m_mx_ev_to_wrt;
  std::vector<JadeDataFrameSP> m_vec_ring_ev;
  JadeDataFrameSP m_ring_end;
  
  uint64_t m_size_ring{1000000};
  std::atomic_uint64_t m_count_ring_write;
  std::atomic_uint64_t m_hot_p_read;
  uint64_t m_count_ring_read;
  bool m_is_async_reading{false};

public:
  void fw_start();
  void fw_stop();
  void fw_init();
  void rd_start();
  void rd_stop();

  uint64_t AsyncPushBack();
  JadeDataFrameSP GetNextCachedEvent();
  JadeDataFrameSP& Front();
  void PopFront();
  uint64_t Size();
  
};

class Telescope{
public:
  std::vector<std::unique_ptr<Layer>> m_vec_layer;
  std::future<uint64_t> m_fut_async_rd;
  std::future<uint64_t> m_fut_async_watch_dog;
  bool m_is_async_reading{false};
  bool m_is_async_watching{false};
  
  Telescope(const std::string& file_context);
  std::vector<JadeDataFrameSP> ReadEvent();
  
  void Start();
  void Stop();

  uint64_t AsyncRead();
  uint64_t AsyncWatchDog();
  
};


#endif
