#include <libwebsockets.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "FirmwarePortal.hh"
#include "common/getopt/getopt.h"
#include "common/linenoiseng/linenoise.h"

#include <iostream>

class Task_data;
class Task_data { // c++ style
public:
    char m_header[LWS_PRE]; // internally protocol header
    //this buffer MUST have LWS_PRE bytes valid BEFORE the pointer. This is so the protocol header data can be added in-situ.
    char m_send_buf[32768]; // must follow header LWS_PRE
    FirmwarePortal* m_fw{nullptr};
    uint64_t m_count{0};

    
    lws_threadpool_task_return
    task_exe(lws_threadpool_task_status s){
        m_count ++;        
        if(m_count > 100000000){
            return LWS_TP_RETURN_FINISHED;
        } else if(m_count % 10000000 == 0){
            lws_snprintf(m_send_buf, sizeof(m_send_buf), "pos++ %llu", (unsigned long long)m_count);
            return LWS_TP_RETURN_SYNC; // LWS_CALLBACK_SERVER_WRITEABLE TP_STATUS_SYNC
        }
        else{
            return LWS_TP_RETURN_CHECKING_IN; // "check in" to see if it has been asked to stop.
        }
    };

    Task_data(const std::string &ip){
        std::string file_context = FirmwarePortal::LoadFileToString("jsonfile/reg_cmd_list.json");
        m_fw = new FirmwarePortal(file_context, ip);
        m_count  = 0;
    };
    
    ~Task_data(){
        if(m_fw){
            delete m_fw;
            m_fw = nullptr;
        }
    };
    
    static void
    cleanup_data(struct lws *wsi, void *user){
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
        
        Task_data *priv_new = new Task_data("131.169.133.174");
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
        std::cout<< "CLOSED++"<<std::endl;
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
        
        std::string recv_str((char*)in, len);
        std::cout<< recv_str<<std::endl;
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
        case LWS_TP_STATUS_QUEUED:{
            std::cout<< "LWS_TP_STATUS_QUEUED"<<std::endl;
            return 0;
        }
        case LWS_TP_STATUS_RUNNING:{
            std::cout<< "LWS_TP_STATUS_RUNNING"<<std::endl;
            return 0;
        }
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

static struct lws_protocols protocols[] = {
    { "http", lws_callback_http_dummy, 0, 0 },
    {"lws-minimal", callback_minimal,  0, 128, 0, NULL, 0},
    { NULL, NULL, 0, 0 } /* terminator */
};


static const struct lws_http_mount mount = {
    /* .mount_next */		NULL,		/* linked-list "next" */
    /* .mountpoint */		"/",		/* mountpoint URL */
    /* .origin */			"./mount-origin", /* serve from dir */
    /* .def */			"index.html",	/* default filename */
    /* .protocol */			NULL,
    /* .cgienv */			NULL,
    /* .extra_mimetypes */		NULL,
    /* .interpret */		NULL,
    /* .cgi_timeout */		0,
    /* .cache_max_age */		0,
    /* .auth_mask */		0,
    /* .cache_reusable */		0,
    /* .cache_revalidate */		0,
    /* .cache_intermediaries */	0,
    /* .origin_protocol */		LWSMPRO_FILE,	/* files in a dir */
    /* .mountpoint_len */		1,		/* char count */
    /* .basic_auth_login_file */	NULL,
};

/*
 * This demonstrates how to pass a pointer into a specific protocol handler
 * running on a specific vhost.  In this case, it's our default vhost and
 * we pass the pvo named "config" with the value a const char * "myconfig".
 *
 * This is the preferred way to pass configuration into a specific vhost +
 * protocol instance.
 */

static const struct lws_protocol_vhost_options pvo_ops = {
    NULL,
    NULL,
    "config",           /* pvo name */
    "myconfig"          /* pvo value */
};

static const struct lws_protocol_vhost_options pvo = {
    NULL,		/* "next" pvo linked-list */
    &pvo_ops,	        /* "child" pvo linked-list */
    "lws-minimal",	/* protocol name we belong to on this vhost */
    ""                  /* ignored */
};

static int interrupted;
void sigint_handler(int sig)
{
    interrupted = 1;
}


int main(int argc, const char **argv)
{
    struct lws_context_creation_info info;
    struct lws_context *context;
    const char *p;
    int logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE
        /* for LLL_ verbosity above NOTICE to be built into lws,
         * lws must have been configured and built with
         * -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE */
        /* | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER */
        /* | LLL_EXT */ /* | LLL_CLIENT */ /* | LLL_LATENCY */
        /* | LLL_DEBUG */;

    signal(SIGINT, sigint_handler);

    if ((p = lws_cmdline_option(argc, argv, "-d")))
        logs = atoi(p);

    lws_set_log_level(logs, NULL);
    lwsl_user("LWS minimal ws server + threadpool | visit http://localhost:7681\n");

    memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */
    info.port = 7681;
    info.mounts = &mount;
    info.protocols = protocols;
    info.pvo = &pvo; /* per-vhost options */
    info.options =
        LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

    context = lws_create_context(&info);
    if (!context) {
        lwsl_err("lws init failed\n");
        return 1;
    }

    /* start the threads that create content */

    while (!interrupted)
        if (lws_service(context, 0))
            interrupted = 1;

    lws_context_destroy(context);

    return 0;
}
