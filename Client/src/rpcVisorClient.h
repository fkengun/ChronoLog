
#ifndef RPC_VISOR_PORTAL_CLIENT_H
#define RPC_VISOR_PORTAL_CLIENT_H

#include <string>
#include <unordered_map>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <thallium.hpp>
#include <sys/types.h>
#include <unistd.h>
#include <thallium/serialization/serialize.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/vector.hpp>
#include <thallium/serialization/stl/unordered_map.hpp>  // remove after attrs are changed 

#include "log.h"
#include "error.h"
#include "chronolog_types.h"
#include "AcquireStoryResponseMsg.h"

namespace tl = thallium;


namespace chronolog
{


class RpcVisorClient
{

public:
    static RpcVisorClient * CreateRpcVisorClient( tl::engine & tl_engine,
                                                  std::string const & service_addr, uint16_t provider_id )
    {
        try
        {
            return new RpcVisorClient( tl_engine,service_addr, provider_id);
        }
        catch( tl::exception const&)
        {

        }
        return nullptr;
    }


    int Connect(std::string const& client_account, uint32_t client_host_ip, ClientId & , uint64_t &clock_offset)
    {
//        LOGD("%s in ChronoLogAdminRPCProxy at addresss %p called in PID=%d, with args: uri=%s, client_id=%s",
//             __FUNCTION__, this, getpid(), uri.c_str(), client_id.c_str());
//        return CHRONOLOG_RPC_CALL_WRAPPER("Connect", 0, int, uri, client_id, flags, clock_offset);
        try
        {
            return visor_connect.on(service_ph)( client_account, client_host_ip);
        }
        catch (tl::exception const&)
        {

        }
        return(CL_ERR_UNKNOWN);
    }

    int Disconnect(std::string const& client_id)
    {
//        LOGD("%s is called in PID=%d, with args: client_id=%s, flags=%d",
//             __FUNCTION__, getpid(), client_id.c_str(), flags);
        try
        {
            return visor_disconnect.on(service_ph)( client_id);
        }
        catch (tl::exception const&)
        {

        }
        return(CL_ERR_UNKNOWN);
    }

    int CreateChronicle(std::string const& name,
                        const std::unordered_map<std::string, std::string> &attrs,
                        int &flags) 
    {
        LOGD("%s is called in PID=%d, with args: name=%s, flags=%d, attrs=",
             __FUNCTION__, getpid(), name.c_str(), flags);
        try
        {
            return  create_chronicle.on(service_ph)(name, attrs, flags);
        }
        catch (tl::exception const&)
        {

        }
        return(CL_ERR_UNKNOWN);
        
    }

    int DestroyChronicle(std::string const& name)
    {
        LOGD("%s is called in PID=%d, with args: name=%s", __FUNCTION__, getpid(), name.c_str());
        try
        {
            return destroy_chronicle.on(service_ph)(name);
        }
        catch (tl::exception const&)
        {

        }
        return(CL_ERR_UNKNOWN);
    }

    chronolog::AcquireStoryResponseMsg AcquireStory(std::string const& client_id, std::string const& chronicle_name, std::string const& story_name,
                     const std::unordered_map<std::string, std::string> &attrs,  const int &flags) 
    {
        LOGD("%s is called in PID=%d, with args: client_id=%s, chronicle_name=%s, story_name=%s, flags=%d",
             __FUNCTION__, getpid(), client_id.c_str(), chronicle_name.c_str(), story_name.c_str(), flags);
        try
        {
            return  acquire_story.on(service_ph)(client_id, chronicle_name, story_name, attrs, flags);
        }
        catch (tl::exception const&)
        {

        }
        return(AcquireStoryResponseMsg(CL_ERR_UNKNOWN,0,std::vector<KeeperIdCard> {}));
    }

    int ReleaseStory(std::string &client_id, std::string const& chronicle_name, std::string const& story_name) 
    {
        LOGD("%s is called in PID=%d, with args: client_id=%s, chronicle_name=%s, story_name=%s",
             __FUNCTION__, getpid(), client_id.c_str(), chronicle_name.c_str(), story_name.c_str());
        try
        {
            return release_story.on(service_ph)(client_id, chronicle_name, story_name);
        }
        catch (tl::exception const&)
        {

        }
        return(CL_ERR_UNKNOWN);
    }

    int DestroyStory(std::string const& chronicle_name, std::string const& story_name)
    {
        LOGD("%s is called in PID=%d, with args: chronicle_name=%s, story_name=%s",
             __FUNCTION__, getpid(), chronicle_name.c_str(), story_name.c_str());
        try
        {
            return  destroy_story.on(service_ph)( chronicle_name, story_name);
        }
        catch (tl::exception const&)
        {

        }
        return(CL_ERR_UNKNOWN);
    }


    int GetChronicleAttr(std::string const& name, const std::string &key, std::string &value) 
    {
        LOGD("%s is called in PID=%d, with args: name=%s, key=%s", __FUNCTION__, getpid(), name.c_str(), key.c_str());
        try
        {
            return get_chronicle_attr.on(service_ph)(name, key, value);
        }
        catch (tl::exception const&)
        {

        }
        return(CL_ERR_UNKNOWN);
    }

    int EditChronicleAttr(std::string const& name, const std::string &key, const std::string &value) 
    {
        LOGD("%s is called in PID=%d, with args: name=%s, key=%s, value=%s",
             __FUNCTION__, getpid(), name.c_str(), key.c_str(), value.c_str());
        try
        {
            return  edit_chronicle_attr.on(service_ph)(name, key, value);
        }
        catch (tl::exception const&)
        {

        }
        return(CL_ERR_UNKNOWN);
    }

    std::vector<std::string>  ShowChronicles(std::string const& client_id) //, std::vector<std::string> & chronicles) 
    {
        LOGD("%s is called in PID=%d, with args: client_id=%s", __FUNCTION__, getpid(), client_id.c_str());
            
        return  show_chronicles.on(service_ph)(client_id); //INNA: change the function definitions , then add try-catch block
    }

    std::vector<std::string>  ShowStories(std::string const& client_id, std::string const&chronicle_name) //, std::vector<std::string> & stories ) 
    {
        LOGD("%s is called in PID=%d, with args: client_id=%s, chronicle_name=%s",
             __FUNCTION__, getpid(), client_id.c_str(), chronicle_name.c_str());
        return show_stories.on(service_ph)( client_id, chronicle_name); //INNA: change the function definitions , then add try-catch block
    }




    ~RpcVisorClient()
    {
        visor_connect.deregister();
        visor_disconnect.deregister();
        create_chronicle.deregister();
        destroy_chronicle.deregister();
        get_chronicle_attr.deregister();
        edit_chronicle_attr.deregister();
        acquire_story.deregister();
        release_story.deregister();
        destroy_story.deregister();
        show_chronicles.deregister();
        show_stories.deregister();
    }

    private:


    std::string service_addr;     // na address of ChronoVisor ClientService  
    uint16_t    service_provider_id;          // ChronoVisor ClientService provider_id id
    tl::provider_handle  service_ph;  //provider_handle for client registry service
    tl::remote_procedure visor_connect;
    tl::remote_procedure visor_disconnect;
    tl::remote_procedure create_chronicle;
    tl::remote_procedure destroy_chronicle;
    tl::remote_procedure get_chronicle_attr;
    tl::remote_procedure edit_chronicle_attr;
    tl::remote_procedure acquire_story;
    tl::remote_procedure release_story;
    tl::remote_procedure destroy_story;
    tl::remote_procedure show_chronicles;
    tl::remote_procedure show_stories;

    RpcVisorClient(RpcVisorClient const&) = delete;
    RpcVisorClient& operator= (RpcVisorClient const&) = delete;


    // constructor is private to make sure thalium rpc objects are created on the heap, not stack
    RpcVisorClient(tl::engine &tl_engine, std::string const &service_addr, uint16_t provider_id)
            : service_addr(service_addr), service_provider_id(provider_id),
              service_ph(tl_engine.lookup(service_addr), provider_id)
    {
        std::cout << " RpcVisorClient created for Visor Service at {" << service_addr << "} provider_id {"
                  << service_provider_id << "}" << std::endl;
        visor_connect = tl_engine.define("Connect");
        visor_disconnect = tl_engine.define("Disconnect");
        create_chronicle = tl_engine.define("CreateChronicle");
        destroy_chronicle = tl_engine.define("DestroyChronicle");
        get_chronicle_attr = tl_engine.define("GetChronicleAttr");
        edit_chronicle_attr = tl_engine.define("EditChronicleAttr");
        acquire_story = tl_engine.define("AcquireStory");
        release_story = tl_engine.define("ReleaseStory");
        destroy_story = tl_engine.define("DestroyStory");
        show_chronicles = tl_engine.define("ShowChronicles");
        show_stories = tl_engine.define("ShowStories");
    }
};

}

#endif
