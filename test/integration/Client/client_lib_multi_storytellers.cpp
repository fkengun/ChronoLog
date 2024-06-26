#include <chronolog_client.h>
#include <cassert>
#include <common.h>
#include <thread>
#include <chrono>
#include <cmd_arg_parse.h>
#include "log.h"

#define STORY_NAME_LEN 5

struct thread_arg
{
    int tid;
    std::string client_id;
};

chronolog::Client*client;

void thread_body(struct thread_arg*t)
{
    // Local variable declarations
    int flags = 0;
    uint64_t offset;
    int ret;
    std::string chronicle_name;

    // Determine chronicle name based on thread ID (even or odd)
    if(t->tid % 2 == 0)
        chronicle_name = "CHRONICLE_2";
    else
        chronicle_name = "CHRONICLE_1";

    // Create attributes for the chronicle
    std::map <std::string, std::string> chronicle_attrs;
    chronicle_attrs.emplace("Priority", "High");
    flags = 1;

    // Create the chronicle
    ret = client->CreateChronicle(chronicle_name, chronicle_attrs, flags);
    LOG_DEBUG("[ClientLibMultiStorytellers] Chronicle created: tid={}, ChronicleName={}, Flags: {}", t->tid, chronicle_name
         , flags);

    // Create attributes for the story
    std::string story_name = gen_random(STORY_NAME_LEN);
    std::map <std::string, std::string> story_attrs;
    flags = 2;

    // Acquire the story
    auto acquire_ret = client->AcquireStory(chronicle_name, story_name, story_attrs, flags);
    LOG_DEBUG("[ClientLibMultiStorytellers] Story acquired: tid={}, ChronicleName={}, StoryName={}, Ret: {}", t->tid
         , chronicle_name, story_name, acquire_ret.first);

    // Assertion for successful story acquisition or expected errors
    assert(acquire_ret.first == chronolog::CL_SUCCESS || acquire_ret.first == chronolog::CL_ERR_NOT_EXIST ||
           acquire_ret.first == chronolog::CL_ERR_NO_KEEPERS);

    // If story acquisition is successful, log events to the story
    if(chronolog::CL_SUCCESS == acquire_ret.first)
    {
        auto story_handle = acquire_ret.second;
        for(int i = 0; i < 100; ++i)
        {
            // Log an event to the story
            story_handle->log_event("line " + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(i % 10));
        }

        // Release the story
        ret = client->ReleaseStory(chronicle_name, story_name);
        LOG_DEBUG("[ClientLibMultiStorytellers] Story released: tid={}, ChronicleName={}, StoryName={}, Ret: {}", t->tid
             , chronicle_name, story_name, ret);

        // Assertion for successful story release or expected errors
        assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NO_CONNECTION);
    }

    // Destroy the story
    ret = client->DestroyStory(chronicle_name, story_name);
    LOG_DEBUG("[ClientLibMultiStorytellers] Story destroyed: tid={}, ChronicleName={}, StoryName={}, Ret: {}", t->tid
         , chronicle_name, story_name, ret);

    // Assertion for successful story destruction or expected errors
    assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NOT_EXIST || ret == chronolog::CL_ERR_ACQUIRED ||
           ret == chronolog::CL_ERR_NO_CONNECTION);

    // Destroy the chronicle
    ret = client->DestroyChronicle(chronicle_name);
    LOG_DEBUG("[ClientLibMultiStorytellers] Chronicle destroyed: tid={}, ChronicleName={}", t->tid, chronicle_name);

    // Assertion for successful chronicle destruction or expected errors
    assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NOT_EXIST || ret == chronolog::CL_ERR_ACQUIRED ||
           ret == chronolog::CL_ERR_NO_CONNECTION);
}


int main(int argc, char**argv)
{
    std::string conf_file_path;
    conf_file_path = parse_conf_path_arg(argc, argv);
    if(conf_file_path.empty())
    {
        std::exit(EXIT_FAILURE);
    }

    int provided;
    std::string client_id = gen_random(8);

    int num_threads = 4;

    std::vector <struct thread_arg> t_args(num_threads);
    std::vector <std::thread> workers(num_threads);

    ChronoLogRPCImplementation protocol = CHRONOLOG_THALLIUM_SOCKETS;
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
    LOG_INFO("[ClientLibMultiStorytellers] Running test.");


    std::string server_ip = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.IP;
    int base_port = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.BASE_PORT;
    client = new chronolog::Client(confManager);//protocol, server_ip, base_port);

    std::string server_uri = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.PROTO_CONF;
    server_uri += "://" + server_ip + ":" + std::to_string(base_port);

    int flags = 0;
    uint64_t offset;
    int ret = client->Connect();

    if(chronolog::CL_SUCCESS != ret)
    {
        LOG_ERROR("[ClientLibMultiStorytellers] Failed to connect to ChronoVisor");
        delete client;
        return -1;
    }

    for(int i = 0; i < num_threads; i++)
    {
        t_args[i].tid = i;
        t_args[i].client_id = client_id;
        std::thread t{thread_body, &t_args[i]};
        workers[i] = std::move(t);
    }

    for(int i = 0; i < num_threads; i++)
        workers[i].join();

    ret = client->Disconnect();
    delete client;

    return 0;
}
