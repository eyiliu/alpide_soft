/*
 * lws-minimal-ws-server=threadpool
 *
 * Written in 2010-2019 by Andy Green <andy@warmcat.com>
 *
 * This file is made available under the Creative Commons CC0 1.0
 * Universal Public Domain Dedication.
 *
 * This demonstrates a minimal ws server that can cooperate with
 * other threads cleanly.  Two other threads are started, which fill
 * a ringbuffer with strings at 10Hz.
 *
 * The actual work and thread spawning etc are done in the protocol
 * implementation in protocol_lws_minimal.c.
 *
 * To keep it simple, it serves stuff in the subdirectory "./mount-origin" of
 * the directory it was started in.
 * You can change that by changing mount.origin.
 */

#include <libwebsockets.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>


#include "FirmwarePortal.hh"
#include "common/getopt/getopt.h"
#include "common/linenoiseng/linenoise.h"


#include <iostream>
/*
 * ws protocol handler plugin for "lws-minimal" demonstrating lws threadpool
 *
 * Written in 2010-2019 by Andy Green <andy@warmcat.com>
 *
 * This file is made available under the Creative Commons CC0 1.0
 * Universal Public Domain Dedication.
 *
 * The main reason some things are as they are is that the task lifecycle may
 * be unrelated to the wsi lifecycle that queued that task.
 *
 * Consider the task may call an external library and run for 30s without
 * "checking in" to see if it should stop.  The wsi that started the task may
 * have closed at any time before the 30s are up, with the browser window
 * closing or whatever.
 *
 * So data shared between the asynchronous task and the wsi must have its
 * lifecycle determined by the task, not the wsi.  That means a separate struct
 * that can be freed by the task.
 *
 * In the case the wsi outlives the task, the tasks do not get destroyed until
 * the service thread has called lws_threadpool_task_status() on the completed
 * task.  So there is no danger of the shared task private data getting randomly
 * freed.
 */

struct per_vhost_data__minimal {
    struct lws_threadpool *tp;
    const char *config;
};

struct task_data {
    char result[64]; // include LWS_PRE header
    uint64_t pos;
    uint64_t end;
};

/*
 * Create the private data for the task
 *
 * Notice we hand over responsibility for the cleanup and freeing of the
 * allocated task_data to the threadpool, because the wsi it was originally
 * bound to may close while the thread is still running.  So we allocate
 * something discrete for the task private data that can be definitively owned
 * and freed by the threadpool, not the wsi... the pss won't do, as it only
 * exists for the lifecycle of the wsi connection.
 *
 * When the task is created, we also tell it how to destroy the private data
 * by giving it args.cleanup as cleanup_task_private_data() defined below.
 */

static struct task_data *
create_task_private_data(void)
{
    struct task_data *priv = static_cast<struct task_data*>(malloc(sizeof(*priv)));
    return priv;
}

/*
 * Destroy the private data for the task
 *
 * Notice the wsi the task was originally bound to may be long gone, in the
 * case we are destroying the lws context and the thread was doing something
 * for a long time without checking in.
 */
static void
cleanup_task_private_data(struct lws *wsi, void *user)
{
    struct task_data *priv = static_cast<struct task_data *>(user);
    free(priv);
}

/*
 * This runs in its own thread, from the threadpool.
 *
 * The implementation behind this in lws uses pthreads, but no pthreadisms are
 * required in the user code.
 *
 * The example counts to 10M, "checking in" to see if it should stop after every
 * 100K and pausing to sync with the service thread to send a ws message every
 * 1M.  It resumes after the service thread determines the wsi is writable and
 * the LWS_CALLBACK_SERVER_WRITEABLE indicates the task thread can continue by
 * calling lws_threadpool_task_sync().
 */

static enum lws_threadpool_task_return
task_function(void *user, enum lws_threadpool_task_status s)
{
//  std::cout<< "enter task"<<std::endl;
    struct task_data *priv = (struct task_data *)user;
    int budget = 100 * 1000;
        
    if (priv->pos == priv->end)
        return LWS_TP_RETURN_FINISHED;

    /*
     * Preferably replace this with ~100ms of your real task, so it
     * can "check in" at short intervals to see if it has been asked to
     * stop.
     *
     * You can just run tasks atomically here with the thread dedicated
     * to it, but it will cause odd delays while shutting down etc and
     * the task will run to completion even if the wsi that started it
     * has since closed.
     */

    while (budget--)
        priv->pos++;

    usleep(100000);

    if (!(priv->pos % (1000 * 1000))) {
        lws_snprintf(priv->result + LWS_PRE, // start after LWS_PRE header
                     sizeof(priv->result) - LWS_PRE,
                     "pos-- %llu", (unsigned long long)priv->pos);
//        std::cout<< "return sync "<<std::endl;
        return LWS_TP_RETURN_SYNC; // LWS_CALLBACK_SERVER_WRITEABLE TP_STATUS_SYNC
    }
//    std::cout<< "return checking in "<<std::endl;
    return LWS_TP_RETURN_CHECKING_IN; // "check in" to see if it has been asked to stop.
}

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
        cargs.threads = 3;
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
        
        struct task_data *priv_new = create_task_private_data();
        if (!priv_new)
            return 1;
        priv_new->pos = 0;
        priv_new->end = 10 * 1000 * 1000;
        

        struct lws_threadpool_task_args args;
        memset(&args, 0, sizeof(args));        
        args.user = priv_new; 

        /* queue the task... the task takes on responsibility for
         * destroying args.user.  pss->priv just has a copy of it */

        args.wsi = wsi;
        args.task = task_function;
        args.cleanup = cleanup_task_private_data;
        
        char name[32];
        lws_get_peer_simple(wsi, name, sizeof(name));
        std::cout<< "name = "<<name<<std::endl;
        if (!lws_threadpool_enqueue(vhd->tp, &args, "ws %s", name)) {
            lwsl_user("%s: Couldn't enqueue task\n", __func__);
            cleanup_task_private_data(wsi, priv_new);
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
        struct task_data *priv_task;
        int tp_status = lws_threadpool_task_status_wsi(wsi, &task, (void**)&priv_task);
        
        std::string recv_str((char*)in, len);
        std::cout<< recv_str<<std::endl;
        break;
    }
    case LWS_CALLBACK_SERVER_WRITEABLE:{
        
        /*
         * even completed tasks wait in a queue until we call the
         * below on them.  Then they may destroy themselves and their
         * args.user data (by calling the cleanup callback).
         *
         * If you need to get things from the still-valid private task
         * data, copy it here before calling
         * lws_threadpool_task_status() that may free the task and the
         * private task data.
         */
        struct lws_threadpool_task *task;
        struct task_data *priv_task;
        int tp_status = lws_threadpool_task_status_wsi(wsi, &task, (void**)&priv_task);
        lwsl_debug("%s: LWS_CALLBACK_SERVER_WRITEABLE: status %d\n",
                   __func__, tp_status);
        
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
            int n =  strlen(priv_task->result + LWS_PRE); //remove PRE header
            int m = lws_write(wsi, (unsigned char *)priv_task->result + LWS_PRE, n, LWS_WRITE_TEXT);
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
