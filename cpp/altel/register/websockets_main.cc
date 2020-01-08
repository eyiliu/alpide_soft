#include <cstring>

#include <iostream>
#include <deque>
#include <queue>
#include <mutex>
#include <future>
#include <condition_variable>

#include <signal.h>

#include <libwebsockets.h>

#include "common/getopt/getopt.h"
#include "common/linenoiseng/linenoise.h"

#include "FirmwarePortal.hh"
#include "AltelReader.hh"

using namespace std::chrono_literals;


template<typename ... Args>
static std::size_t FormatPrint(std::ostream &os, const std::string& format, Args ... args ){
  std::size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1;
  std::unique_ptr<char[]> buf( new char[ size ] ); 
  std::snprintf( buf.get(), size, format.c_str(), args ... );
  std::string formated_string( buf.get(), buf.get() + size - 1 );
  os<<formated_string<<std::flush;
  return formated_string.size();
}


class Task_data;
class Task_data { // c++ style
public:
  char m_header[LWS_PRE]; // internally protocol header
  //this buffer MUST have LWS_PRE bytes valid BEFORE the pointer. This is so the protocol header data can be added in-situ.
  char m_send_buf[32768]; // must follow header LWS_PRE
  FirmwarePortal* m_fw{nullptr};
  
  std::mutex m_mx_recv;
  std::deque<std::string> m_qu_recv;  

  bool m_flag_call_back_closed{false};
  bool m_is_running_reading{false};
  std::unique_ptr<AltelReader> m_rd;
  std::future<uint64_t> m_fut_async_rd;

  std::mutex m_mx_ev_to_wrt;
  std::deque<JadeDataFrameSP> m_qu_ev_to_wrt;
  std::condition_variable m_cv_valid_ev_to_wrt;
  
  void callback_add_recv(char* in,  size_t len){
    std::string str_recv(in, len); //TODO: split combined recv.
    std::cout<< "recv: "<<str_recv<<std::endl;
    std::unique_lock<std::mutex> lk_recv(m_mx_recv);
    m_qu_recv.push_back(std::move(str_recv));
  } 
  
  lws_threadpool_task_return
  task_exe(lws_threadpool_task_status s){
    std::cout<< "."<<s <<std::endl;
    if ( (s != LWS_TP_STATUS_RUNNING) || m_flag_call_back_closed){
      std::cout<< "my stopping "<<std::endl;
      m_is_running_reading = false;
      if(m_fut_async_rd.valid())
        m_fut_async_rd.get();
      m_rd.reset();
      return LWS_TP_RETURN_STOPPED;
    }
    //============== cmd execute  =================
    std::string str_recv;
    {
      std::unique_lock<std::mutex> lk_recv(m_mx_recv);
      if(!m_qu_recv.empty()){
        std::swap(str_recv, m_qu_recv.front());
        m_qu_recv.pop_front();
      }
    }
    if(!str_recv.empty()){
      std::cout<< "run recv: <"<<str_recv<<">"<<std::endl;
      if(str_recv=="start"){
        std::string file_context = FirmwarePortal::LoadFileToString("jsonfile/reader.json");
        if(m_rd){
          m_rd->Close();
        }
        m_rd.reset();//delete old object
        m_rd.reset(new AltelReader(file_context) );
        m_rd->Open();
        m_is_running_reading = true;
        m_fut_async_rd = std::async(std::launch::async, &Task_data::async_reading, this);        
      };

      if(str_recv=="stop"){
        m_is_running_reading = false;
        if(m_fut_async_rd.valid())
          m_fut_async_rd.get();
        m_rd.reset();
      };

      if(str_recv=="teminate"){
        m_is_running_reading = false;
        if(m_fut_async_rd.valid())
          m_fut_async_rd.get();
        m_rd.reset();        
        return LWS_TP_RETURN_FINISHED;        
      };      
      return LWS_TP_RETURN_CHECKING_IN;
      // return LWS_TP_RETURN_SYNC;
    }

    //============ data send ====================

    std::unique_lock<std::mutex> lk_in(m_mx_ev_to_wrt);
    while(m_qu_ev_to_wrt.empty()){
      while(m_cv_valid_ev_to_wrt.wait_for(lk_in, 200ms) ==std::cv_status::timeout){
        return LWS_TP_RETURN_CHECKING_IN;
      }
    }

    auto df = m_qu_ev_to_wrt.front();
    m_qu_ev_to_wrt.pop_front();
    m_qu_ev_to_wrt.clear(); // only for test now, remove all events in queue
    lk_in.unlock();
    df->Decode(1); //level 1, header-only
    uint16_t tg =  df->GetCounter();
    std::snprintf(m_send_buf, sizeof(m_send_buf), "Trigger ID: %u", tg);
    // df goes to send out buffer
    return LWS_TP_RETURN_SYNC; // LWS_CALLBACK_SERVER_WRITEABLE TP_STATUS_SYNC
    //return LWS_TP_RETURN_CHECKING_IN; // "check in" to see if it has been asked to stop.
  };
  
  uint64_t async_reading(){
    std::cout<< "aysnc enter"<<std::endl;
    auto tp_start = std::chrono::system_clock::now();
    auto tp_print_prev = std::chrono::system_clock::now();
    uint64_t ndf_print_next = 10000;
    uint64_t ndf_print_prev = 0;
    uint64_t n_df = 0;
    m_is_running_reading = true;
    while (m_is_running_reading){
      auto df = m_rd? m_rd->Read(1000ms):nullptr;
      if(!df){
        continue;
      }
      std::unique_lock<std::mutex> lk_out(m_mx_ev_to_wrt);
      m_qu_ev_to_wrt.push_back(df);
      n_df ++;
      lk_out.unlock();
      m_cv_valid_ev_to_wrt.notify_all();
    }
    std::cout<< "aysnc exit"<<std::endl;
    return n_df;
  }
  
  Task_data(const std::string& json_path, const std::string& ip_addr){
    std::string file_context = FirmwarePortal::LoadFileToString(json_path);
    m_fw = new FirmwarePortal(file_context, ip_addr);
    
  };
    
  ~Task_data(){
    if(m_fw){
      delete m_fw;
      m_fw = nullptr;
    }
    std::cout<< "Task_data instance is distructed"<< std::endl;
  };

  
  static void
  cleanup_data(struct lws *wsi, void *user){
    std::cout<< "static clean up function"<<std::endl;
    Task_data *data = static_cast<Task_data*>(user);
    delete data;
  };

  static lws_threadpool_task_return
  task_function(void *user, enum lws_threadpool_task_status s){
    return (reinterpret_cast<Task_data*>(user))->task_exe(s);
  };
    
};

struct per_vhost_data__minimal { //c style, allocated internally
  struct lws_threadpool *tp;
  const char *config;
};

static int
callback_minimal(struct lws *wsi, enum lws_callback_reasons reason,
                 void *, void *in, size_t len)
{
  struct per_vhost_data__minimal *vhd =
    (struct per_vhost_data__minimal *)
    lws_protocol_vh_priv_get(lws_get_vhost(wsi),
                             lws_get_protocol(wsi));
    
  switch (reason) {
  case LWS_CALLBACK_PROTOCOL_INIT:{  //One-time call per protocol, per-vhost using it,
    lwsl_user("%s: INIT++\n", __func__);
    /* create our per-vhost struct */
    vhd = static_cast<per_vhost_data__minimal*>(lws_protocol_vh_priv_zalloc(lws_get_vhost(wsi), lws_get_protocol(wsi),
                                                                            sizeof(struct per_vhost_data__minimal)));
    if (!vhd)
      return 1;

    /* recover the pointer to the globals struct */
    const struct lws_protocol_vhost_options *pvo;
    pvo = lws_pvo_search(
                         (const struct lws_protocol_vhost_options *)in,
                         "config");
    if (!pvo || !pvo->value) {
      lwsl_err("%s: Can't find \"config\" pvo\n", __func__);
      return 1;
    }
    vhd->config = pvo->value;

    struct lws_threadpool_create_args cargs;
    memset(&cargs, 0, sizeof(cargs));

    cargs.max_queue_depth = 8;
    cargs.threads = 8;
    vhd->tp = lws_threadpool_create(lws_get_context(wsi), &cargs, "%s",
                                    lws_get_vhost_name(lws_get_vhost(wsi)));
    if (!vhd->tp)
      return 1;
    std::cout<< "vhost name "<< lws_get_vhost_name(lws_get_vhost(wsi))<<std::endl;
    lws_timed_callback_vh_protocol(lws_get_vhost(wsi),
                                   lws_get_protocol(wsi),
                                   LWS_CALLBACK_USER, 1);

    break;
  }
  case LWS_CALLBACK_PROTOCOL_DESTROY:{
    lws_threadpool_finish(vhd->tp);
    lws_threadpool_destroy(vhd->tp);
    break;
  }
  case LWS_CALLBACK_USER:{

    /*
     * in debug mode, dump the threadpool stat to the logs once
     * a second
     */
    lws_threadpool_dump(vhd->tp);
    lws_timed_callback_vh_protocol(lws_get_vhost(wsi),
                                   lws_get_protocol(wsi),
                                   LWS_CALLBACK_USER, 1);
    break;
  }
  case LWS_CALLBACK_ESTABLISHED:{ // (VH) after the server completes a handshake with an incoming client, per ws session
    std::cout<< "ESTABLISHED++"<<std::endl;
        
    Task_data *priv_new = new Task_data("jsonfile/reg_cmd_list.json", "131.169.133.174");
    if (!priv_new)
      return 1;

    struct lws_threadpool_task_args args;
    memset(&args, 0, sizeof(args));
    args.user = priv_new;
    args.wsi = wsi;
    args.task = Task_data::task_function;
    args.cleanup = Task_data::cleanup_data;
        
    char name[32];
    lws_get_peer_simple(wsi, name, sizeof(name));
    std::cout<< "name = "<<name<<std::endl;
    if (!lws_threadpool_enqueue(vhd->tp, &args, "ws %s", name)) {
      lwsl_user("%s: Couldn't enqueue task\n", __func__);
      delete priv_new;
      return 1;
    }

    lws_set_timeout(wsi, PENDING_TIMEOUT_THREADPOOL, 30);

    /*
     * so the asynchronous worker will let us know the next step
     * by causing LWS_CALLBACK_SERVER_WRITEABLE
     */

    break;
  }
  case LWS_CALLBACK_CLOSED:{ // when the websocket session ends 
    std::cout<< "=====================CLOSED++\n\n"<<std::endl;
    struct lws_threadpool_task *task;
    Task_data *priv_task;
    int tp_status = lws_threadpool_task_status_wsi(wsi, &task, (void**)&priv_task);
    priv_task->m_flag_call_back_closed = true;
    std::this_thread::sleep_for(2s);
    lws_threadpool_task_status_wsi(wsi, &task, (void**)&priv_task);
    break;
  }
  case LWS_CALLBACK_WS_SERVER_DROP_PROTOCOL:{
    lwsl_user("LWS_CALLBACK_WS_SERVER_DROP_PROTOCOL: %p\n", wsi);
    lws_threadpool_dequeue(wsi);
    break;
  }
  case LWS_CALLBACK_RECEIVE:{
    //TODO: handle user command
    //add received message to task data which then can be retrieved by task function
    struct lws_threadpool_task *task;
    Task_data *priv_task;
    int tp_status = lws_threadpool_task_status_wsi(wsi, &task, (void**)&priv_task);
    priv_task->callback_add_recv((char*)in, len);
    break;
  }
  case LWS_CALLBACK_SERVER_WRITEABLE:{                
    struct lws_threadpool_task *task;
    Task_data *task_data;
    // cleanup)function will be called in lws_threadpool_task_status_wsi in case task is completed/stop/finished
    int tp_status = lws_threadpool_task_status_wsi(wsi, &task, (void**)&task_data);
    lwsl_debug("%s: LWS_CALLBACK_SERVER_WRITEABLE: status %d\n", __func__, tp_status);
        
    switch(tp_status){
    case LWS_TP_STATUS_FINISHED:{
      std::cout<< "LWS_TP_STATUS_FINISHED"<<std::endl;
      return 0;
    }
    case LWS_TP_STATUS_STOPPED:{
      std::cout<< "LWS_TP_STATUS_STOPPED"<<std::endl;
      return 0;
    }
    case LWS_TP_STATUS_QUEUED:
    case LWS_TP_STATUS_RUNNING:
      return 0;
    case LWS_TP_STATUS_STOPPING:{
      std::cout<< "LWS_TP_STATUS_STOPPING"<<std::endl;
      return 0;
    }
    case LWS_TP_STATUS_SYNCING:{
      /* the task has paused (blocked) for us to do something */
      lws_set_timeout(wsi, PENDING_TIMEOUT_THREADPOOL_TASK, 5);
      int n = strlen(task_data->m_send_buf);
      int m = lws_write(wsi, (unsigned char *)task_data->m_send_buf, n, LWS_WRITE_TEXT);
      if (m < n) {
        lwsl_err("ERROR %d writing to ws socket\n", m);
        lws_threadpool_task_sync(task, 1); // task to be stopped
        return -1;
      }
      
      /*
       * service thread has done whatever it wanted to do with the
       * data the task produced: if it's waiting to do more it can
       * continue now.
       */
      lws_threadpool_task_sync(task, 0); // task thread can continue, unblocked

      //? Is OK do something here without interference to the task data? yes
      break;
    }
    default:
      return -1;
    }   
    break;
  }
  default:
    break;
  }
  return 0;
}

/////////////////////////////////////////


static int interrupted;
int main(int argc, const char **argv){
  signal(SIGINT, [](int signal){interrupted = 1;});

  int log_level = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE;
  lws_set_log_level(log_level, NULL);

  struct lws_protocols protocols[] =
    {
     { "http", lws_callback_http_dummy, 0, 0 },
     { "lws-minimal", callback_minimal,  0, 128, 0, NULL, 0},
     { NULL, NULL, 0, 0 } /* terminator */
    };

  const struct lws_http_mount mount =
    {/* .mount_next */          NULL,             /* linked-list "next" */
     /* .mountpoint */          "/",              /* mountpoint URL */
     /* .origin */              "./mount-origin", /* serve from dir */
     /* .def */			"index.html",	  /* default filename */
     /* .protocol */		NULL,
     /* .cgienv */		NULL,
     /* .extra_mimetypes */     NULL,
     /* .interpret */		NULL,
     /* .cgi_timeout */         0,
     /* .cache_max_age */       0,
     /* .auth_mask */           0,
     /* .cache_reusable */      0,
     /* .cache_revalidate */    0,
     /* .cache_intermediaries */0,
     /* .origin_protocol */     LWSMPRO_FILE,      /* files in a dir */
     /* .mountpoint_len */      1,                 /* char count */
     /* .basic_auth_login_file */NULL,
    };
  
  const struct lws_protocol_vhost_options pvo_ops =
    {NULL,
     NULL,
     "config",           /* pvo name */
     "myconfig"          /* pvo value */
    };

  const struct lws_protocol_vhost_options pvo =
    {NULL,               /* "next" pvo linked-list */
     &pvo_ops,           /* "child" pvo linked-list */
     "lws-minimal",      /* protocol name we belong to on this vhost */
     ""                  /* ignored */
    };

  uint16_t port = 7681;
  struct lws_context_creation_info info;
  memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */
  info.port = port;
  info.mounts = &mount;
  info.protocols = protocols;
  info.pvo = &pvo;               /* per-vhost options */
  info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;
  info.ws_ping_pong_interval = 5; // idle ping-pong sec
  
  struct lws_context *context =  lws_create_context(&info);
  if (!context) {
    lwsl_err("lws init failed\n");
    return 1;
  }
  
  lwsl_user("http://localhost:%u\n", port);  
  while (!interrupted){
    if (lws_service(context, 0))
      interrupted = 1;
  }
  
  lws_context_destroy(context);
  return 0;
}
