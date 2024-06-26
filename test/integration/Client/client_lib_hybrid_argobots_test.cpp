#include <atomic>
#include <chronolog_client.h>
#include <common.h>
#include <thread>
#include <atomic>
#include <abt.h>
#include <mpi.h>
#include <cmd_arg_parse.h>
#include "log.h"

chronolog::Client*client;

struct thread_arg
{
    int tid;
};

void thread_function(void*t)
{
    LOG_INFO("[ClientLibHybridArgobotsTest] Starting thread function for thread ID: {}", ((thread_arg*)t)->tid);

    ChronoLog::ConfigurationManager confManager("./default_conf.json");
    std::string server_ip = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.IP;
    int base_port = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.BASE_PORT;
    std::string client_id = gen_random(8);
    std::string server_uri =
            confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.PROTO_CONF + "://" + server_ip + ":" +
            std::to_string(base_port);
    int flags = 0;

    int ret = client->Connect(); // Connect to server using client_id and flags
    if(ret == chronolog::CL_SUCCESS)
    {
        LOG_INFO("[ClientLibHybridArgobotsTest] Successfully connected to server for thread ID: {}", ((thread_arg*)t)->tid);
    }

    ret = client->Disconnect(); // Disconnect from server using client_id and flags
    if(ret == chronolog::CL_SUCCESS)
    {
        LOG_INFO("[ClientLibHybridArgobotsTest] Successfully disconnected from server for thread ID: {}"
             , ((thread_arg*)t)->tid);
    }
}

int main(int argc, char**argv)
{
    std::vector <std::string> client_ids;
    std::atomic <long> duration_connect{}, duration_disconnect{};
    std::vector <std::thread> thread_vec;
    uint64_t offset;
    int provided;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    std::string conf_file_path;
    conf_file_path = parse_conf_path_arg(argc, argv);
    if(conf_file_path.empty())
    {
        std::exit(EXIT_FAILURE);
    }
    ChronoLog::ConfigurationManager confManager(conf_file_path);
    int result = Logger::initialize(confManager.CLIENT_CONF.CLIENT_LOG_CONF.LOGTYPE
                                    , confManager.CLIENT_CONF.CLIENT_LOG_CONF.LOGFILE
                                    , confManager.CLIENT_CONF.CLIENT_LOG_CONF.LOGLEVEL
                                    , confManager.CLIENT_CONF.CLIENT_LOG_CONF.LOGNAME
                                    , confManager.CLIENT_CONF.CLIENT_LOG_CONF.LOGFILESIZE
                                    , confManager.CLIENT_CONF.CLIENT_LOG_CONF.LOGFILENUM
                                    , confManager.CLIENT_CONF.CLIENT_LOG_CONF.FLUSHLEVEL);
    if(result == 1)
    {
        exit(EXIT_FAILURE);
    }
    LOG_INFO("[ClientLibHybridArgobotsTest] Running test.");

    std::string server_ip = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.IP;
    int base_port = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.BASE_PORT;
    client = new chronolog::Client(confManager); // protocol, server_ip, base_port);

    int num_xstreams = 8;
    int num_threads = 8;

    ABT_xstream*xstreams = (ABT_xstream*)malloc(sizeof(ABT_xstream) * num_xstreams);
    ABT_pool*pools = (ABT_pool*)malloc(sizeof(ABT_pool) * num_xstreams);
    ABT_thread*threads = (ABT_thread*)malloc(sizeof(ABT_thread) * num_threads);
    struct thread_arg*t_args = (struct thread_arg*)malloc(num_threads * sizeof(struct thread_arg));


    ABT_init(argc, argv);

    ABT_xstream_self(&xstreams[0]);

    for(int i = 1; i < num_xstreams; i++)
    {
        ABT_xstream_create(ABT_SCHED_NULL, &xstreams[i]);
    }


    for(int i = 0; i < num_xstreams; i++)
    {
        ABT_xstream_get_main_pools(xstreams[i], 1, &pools[i]);
    }


    for(int i = 0; i < num_threads; i++)
    {
        ABT_thread_create(pools[i], thread_function, &t_args[i], ABT_THREAD_ATTR_NULL, &threads[i]);
    }

    for(int i = 0; i < num_threads; i++)
        ABT_thread_free(&threads[i]);

    for(int i = 1; i < num_xstreams; i++)
    {
        ABT_xstream_join(xstreams[i]);
        ABT_xstream_free(&xstreams[i]);
    }

    ABT_finalize();

    free(pools);
    free(xstreams);
    free(threads);
    free(t_args);

    LOG_INFO("[ClientLibHybridArgobotsTest] Cleaning up resources and finalizing MPI");
    delete client;
    MPI_Finalize();
    LOG_INFO("[ClientLibHybridArgobotsTest] Exiting main function");
    return 0;
}
