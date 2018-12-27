#include "JadeReader.hh"
#include "JadeUtils.hh"
#include "JadeFactory.hh"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include <chrono>

#define FRAME_SIZE (4+96*(4+16*2+4)+4)

#define HEADER_BYTE  (0x5a)
#define FOOTER_BYTE  (0xa5)


class AltelReader: public JadeReader{
public:
  AltelReader(const JadeOption &opt);
  ~AltelReader() override {};
  JadeOption Post(const std::string &url, const JadeOption &opt) override;
  JadeDataFrameSP Read(const std::chrono::milliseconds &timeout) override;
  void Open() override;
  void Close() override;
private:
  JadeOption m_opt;
  std::string m_buf;
  
  std::string  m_ip;
  ushort       m_tcp;
  sockaddr_in  m_tcpaddr;
  int          m_tcpfd;

private:
  int SelectSocket(int usec);
  
};

namespace{
  auto _test_index_ = JadeUtils::SetTypeIndex(std::type_index(typeid(AltelReader)));
  auto _test_ = JadeFactory<JadeReader>::Register<AltelReader, const JadeOption&>(typeid(AltelReader));
}

AltelReader::AltelReader(const JadeOption& opt)
  :JadeReader(opt), m_opt(opt){
}

void AltelReader::Open(){
  m_tcpfd = socket(AF_INET, SOCK_STREAM, 0);
  m_tcpaddr.sin_family = AF_INET;
  m_tcpaddr.sin_port = htons(m_tcp);
  m_tcpaddr.sin_addr.s_addr = inet_addr(m_ip.c_str());
  if(connect(m_tcpfd, (struct sockaddr *)&m_tcpaddr, sizeof(m_tcpaddr)) < 0){
    //when connect fails, go to follow lines
    if(errno != EINPROGRESS) perror("TCP connection");
    if(errno == 29)
      std::cerr<<"timeout";
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(m_tcpfd, &fds);
    timeval tv_timeout;
    tv_timeout.tv_sec = 0.1;
    tv_timeout.tv_usec = 0;
    int rc = select(m_tcpfd+1, &fds, NULL, NULL, &tv_timeout);
    if(rc<=0)
      perror("connect-select error");
  }  
}

void AltelReader::Close(){
  m_buf.clear();
  close(m_tcpfd);
}

JadeDataFrameSP AltelReader::Read(const std::chrono::milliseconds &timeout){
  size_t size_buf_min = 5;
  size_t size_buf = size_buf_min;
  std::string buf(size_buf, 0);
  size_t size_filled = 0;
  std::chrono::system_clock::time_point tp_timeout;
  bool can_time_out = false;
  uint32_t n = 0;
  uint32_t n_next_print = 4;
  while(size_filled < size_buf){
    
    //print
    if(n+1 == n_next_print){
      std::cout<<n<<"  "<<size_filled<<"  "<<size_buf<<"  "<<(unsigned int)(size_buf-size_filled)<<std::endl;
    }
    n++;
    if(n == n_next_print){
      n_next_print = n_next_print*2;
    }
    //
    
    fd_set fds;
    timeval tv_timeout;
    FD_ZERO(&fds);
    FD_SET(m_tcpfd, &fds);
    FD_SET(0, &fds);  
    tv_timeout.tv_sec = 0;
    tv_timeout.tv_usec = 10;
    if( !select(m_tcpfd+1, &fds, NULL, NULL, &tv_timeout) || !FD_ISSET(m_tcpfd, &fds) ){
      std::cout<<"timeout"<<std::endl;
      if(!can_time_out){
	can_time_out = true;
	tp_timeout = std::chrono::system_clock::now() + timeout;
      }
      else{
	if(std::chrono::system_clock::now() > tp_timeout){
	  std::cerr<<"JadeRead: reading timeout\n";
	  //TODO: keep remain data, nothrow
	  throw;
	}
      }
      continue;
    }
    
    int read_r = recv(m_tcpfd, &buf[size_filled], (unsigned int)(size_buf-size_filled), MSG_WAITALL);
    if(read_r <= 0){
      std::cerr<<"JadeRead: reading error\n";
      throw;
    }
    
    size_filled += read_r;
    can_time_out = false;

    if(size_buf == size_buf_min  && size_filled >= size_buf_min){
      uint8_t header_byte =  buf.front();
      uint32_t w1 = BE32TOH(*reinterpret_cast<const uint32_t*>(buf.data()));
      uint8_t rsv = (w1>>20) & 0xf;
      uint32_t size_payload = (w1 & 0xfffff)/2;
      if(header_byte != HEADER_BYTE){
	std::cerr<<"wrong header\n";
	//TODO: skip brocken data
	throw;
      }
      size_buf = size_buf_min + size_payload;
    }
  }
  uint8_t footer_byte =  buf.back();
  if(footer_byte != FOOTER_BYTE){
    std::cerr<<"wrong footer\n";
    //TODO: skip brocken data
    throw;
  }
  return std::make_shared<JadeDataFrame>(std::move(buf));
}


JadeOption AltelReader::Post(const std::string &url, const JadeOption &opt){
  // if(url == "/set_path"){
  //   m_path=opt.GetStringValue("PATH");
  //   return "{\"status\":true}";
  // }
  
  static const std::string url_base_class("/JadeReader/");
  if( ! url.compare(0, url_base_class.size(), url_base_class) ){
    return JadeReader::Post(url.substr(url_base_class.size()-1), opt);
  }
  return JadePost::Post(url, opt);
}
