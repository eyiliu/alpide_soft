#include <cstring>

#include <iostream>
#include <deque>
#include <queue>
#include <mutex>
#include <future>
#include <condition_variable>

#include <signal.h>

#include "libwebsockets.h"
#include "getopt.h"
#include "linenoise.h"

#include "Telescope.hh"

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
  Telescope* m_tel{nullptr}; 
  std::mutex m_mx_recv;
  std::atomic_uint64_t m_n_recv{0};
  std::deque<std::string> m_qu_recv;
  
  rapidjson::StringBuffer m_sb;
  rapidjson::Writer<rapidjson::StringBuffer> m_js_writer;
  
  bool m_flag_call_back_closed{false};
  uint64_t m_dummy_tr_n{0};
  
  void callback_add_recv(char* in,  size_t len){
    std::string str_recv(in, len); //TODO: split combined recv.
    std::cout<< "recv: "<<str_recv<<std::endl;
    std::unique_lock<std::mutex> lk_recv(m_mx_recv);
    m_qu_recv.push_back(std::move(str_recv));
    m_n_recv ++;
  }
  
  lws_threadpool_task_return
  task_exe(lws_threadpool_task_status s){
    //std::cout<< " .";
    if ( (s != LWS_TP_STATUS_RUNNING) || m_flag_call_back_closed){
      m_tel->Stop();
      return LWS_TP_RETURN_STOPPED;
    }
    //============== cmd execute  =================
    if(m_n_recv){
      std::string str_recv;
      {
        std::unique_lock<std::mutex> lk_recv(m_mx_recv);
        std::swap(str_recv, m_qu_recv.front());
        m_qu_recv.pop_front();
      }
      m_n_recv --;
      if(!str_recv.empty()){
        //std::cout<< "run recv: <"<<str_recv<<">"<<std::endl;
        if(str_recv=="start"){
          //m_tel->Start();
          m_tel->Start_no_tel_reading();
        };
      
        if(str_recv=="stop"){
          m_tel->Stop();       
        };
      
        if(str_recv=="teminate"){
          return LWS_TP_RETURN_FINISHED;        
        };
        return LWS_TP_RETURN_CHECKING_IN;
        // return LWS_TP_RETURN_SYNC;
      }
    }
    
    //============ data send ====================
    if(!m_tel){
      return LWS_TP_RETURN_CHECKING_IN;
    }
    if(0){
      m_sb.Clear();
      rapidjson::PutN(m_sb, '\t', LWS_PRE);
      m_js_writer.Reset(m_sb);
      m_js_writer.StartArray();
      {
        m_js_writer.StartObject();
        {
          m_js_writer.String("detector_type");
          m_js_writer.String("alpide");

          m_js_writer.String("geometry");
          m_js_writer.StartArray();
          {
            m_js_writer.Uint(1024);
            m_js_writer.Uint(512);
            m_js_writer.Uint(6);
          }
          m_js_writer.EndArray();
      
          m_js_writer.String("trigger");
          m_js_writer.Uint(m_dummy_tr_n);
          m_dummy_tr_n++;
          std::cout<< m_dummy_tr_n<<std::endl;
          m_js_writer.String("ext");
          m_js_writer.Uint(1);
      
          m_js_writer.String("data_type");
          m_js_writer.String("hit_xyz_array");
      
          m_js_writer.String("hit_xyz_array");
          m_js_writer.StartArray();
          {
            m_js_writer.StartArray();
            {
              m_js_writer.Uint(10);
              m_js_writer.Uint(10);
              m_js_writer.Uint(1);
            }
            m_js_writer.EndArray();
          }
          m_js_writer.EndArray();
        }
        m_js_writer.EndObject();
      }
      m_js_writer.EndArray();
      return LWS_TP_RETURN_SYNC;
    }
  
    // 
    auto ev = m_tel->ReadEvent();
    if(ev.empty()){ // no event
     return LWS_TP_RETURN_CHECKING_IN;
    }

    m_sb.Clear();
    rapidjson::PutN(m_sb, '\t', LWS_PRE);
    m_js_writer.Reset(m_sb);
    m_js_writer.StartArray();
    for(auto& e: ev){
      e->Serialize(m_js_writer);
    }
    m_js_writer.EndArray();
        
    //std::strcpy(m_send_buf, static_cast<const char*>(sb.GetString()));
    return LWS_TP_RETURN_SYNC; // LWS_CALLBACK_SERVER_WRITEABLE TP_STATUS_SYNC
    // return LWS_TP_RETURN_CHECKING_IN; // "check in" to see if it has been asked to stop.
  };
  
  Task_data(const std::string& json_path, const std::string& ip_addr){
    std::string file_context = FirmwarePortal::LoadFileToString(json_path);
    m_tel = new Telescope(file_context);;
  };
  
  ~Task_data(){
    if(m_tel){
      delete m_tel;
      m_tel = nullptr;
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
        
    Task_data *priv_new = new Task_data("jsonfile/telescope.json", "");
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
      
      auto p_Ch = task_data->m_sb.GetString();
      auto n_Ch = task_data->m_sb.GetSize();
      int m = lws_write(wsi, (unsigned char *)(p_Ch+LWS_PRE), n_Ch-LWS_PRE, LWS_WRITE_BINARY);
      ///// LWS_WRITE_BINARY LWS_WRITE_TEXT
      if (m < n_Ch-LWS_PRE ) {
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
  //info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;
  // see https://libwebsockets.org/lws-api-doc-master/html/group__context-and-vhost.html
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

