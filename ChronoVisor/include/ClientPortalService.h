#ifndef CLIENT_PORTAL_SERVICE_H
#define CLIENT_PORTAL_SERVICE_H

#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/vector.hpp>
#include <thallium/serialization/stl/unordered_map.hpp>

#include "chronolog_types.h"
#include "KeeperIdCard.h"

#include "VisorClientPortal.h"
#include "AcquireStoryResponseMsg.h"


namespace tl = thallium; 

namespace chronolog
{


class ClientPortalService : public thallium::provider<ClientPortalService> 
{
    public:

    static ClientPortalService * CreateClientPortalService(thallium::engine& tl_engine, uint16_t service_provider_id
                , VisorClientPortal & visor_portal)
    {
        return new ClientPortalService(tl_engine, service_provider_id, visor_portal);
    }

    ~ClientPortalService() 
    {
        std::cout<<"ClientPortalService::destructor"<<std::endl;
        get_engine().pop_finalize_callback(this);
    }

    void Connect(tl::request const& request, std::string const& client_account, uint32_t client_host_ip)
    {
        int return_code = 0;
        // TODO : add ClientConnectResponseMsg = >tuple <int, ClientId, clock_offset>
        ClientId client_id;
        uint64_t clock_offset;
        return_code = theVisorClientPortal.ClientConnect(client_account, client_host_ip, client_id, clock_offset);

        request.respond(return_code);
    }

    void Disconnect(tl::request const& request, std::string const&  client_account) //old TODO : replace with client_id
    {
        int return_code = theVisorClientPortal.ClientDisconnect(client_account);
        request.respond(return_code);
    }

    void CreateChronicle( tl::request const& request //, ClientId const& client_id
            , std::string const& chronicle_name, const std::unordered_map<std::string, std::string> &attrs, int &flags)//old
    {
        ClientId client_id;
        int return_code = theVisorClientPortal.CreateChronicle(client_id, chronicle_name, attrs, flags);
        request.respond(return_code);
    }

    void DestroyChronicle( tl::request const& request //,  ClientId const& client_id
            , ChronicleName const& chronicle_name)
    { 
        ClientId client_id;
        int return_code = theVisorClientPortal.DestroyChronicle(client_id, chronicle_name);
        request.respond(return_code);
    }

    void AcquireStory( tl::request const& request
                              //ClientId const& client_id,
                              , std::string const& client_id,
                              std::string const& chronicle_name,
                              std::string const& story_name,
                              const std::unordered_map<std::string, std::string> &attrs,
                              int &flags)
    {

        AcquireStoryResponseMsg acquire_response = theVisorClientPortal.AcquireStory( client_id,chronicle_name,story_name,attrs,flags);
        request.respond(acquire_response);
    }

    void ReleaseStory( tl::request const& request
            , std::string const& client_id 
            , std::string const& chronicle_name, std::string const& story_name)
    {
        int return_code = theVisorClientPortal.ReleaseStory(client_id, chronicle_name,story_name);
        request.respond(return_code);
    }

    void DestroyStory( tl::request const& request
            , std::string const& chronicle_name, std::string const& story_name)
    {
        ClientId client_id{0};
        request.respond(theVisorClientPortal.DestroyStory(client_id, chronicle_name,story_name));
    }


    void GetChronicleAttr( tl::request const& request//, ClientId const& client_id
                , std::string const& chronicle_name, std::string const& key, std::string &value)
    {
        ClientId client_id{0};
        int return_code = theVisorClientPortal.GetChronicleAttr( client_id, chronicle_name, key, value);
        request.respond(return_code);
    }

    void EditChronicleAttr( tl::request const& request//, ClientId const& client_id
                , std::string const& chronicle_name, std::string const& key, std::string const &value)
    {

        ClientId client_id{0};
        int return_code =  theVisorClientPortal.EditChronicleAttr(client_id, chronicle_name, key, value);
        request.respond(return_code);
    }

    void ShowChronicles( tl::request const& request)//, ClientId const& client_id) 
    {
        //TODO: pass in client_id
        ClientId client_id{0};
        std::vector<std::string> chronicles;
        theVisorClientPortal.ShowChronicles( client_id, chronicles);
        request.respond(chronicles);
    }

    void ShowStories( tl::request const& request, const std::string &chronicle_name)//, std::vector<std::string> & stories)
    {
        ClientId client_id{0};
        std::vector<std::string> stories;
        theVisorClientPortal.ShowStories( client_id, chronicle_name,stories);
        
        request.respond(stories);
    }

private:
    
    ClientPortalService(tl::engine& tl_engine, uint16_t service_provider_id, VisorClientPortal & visorPortal)
        : thallium::provider<ClientPortalService>(tl_engine, service_provider_id)
        , theVisorClientPortal(visorPortal)
    {
        define("Connect", &ClientPortalService::Connect);
        define("Disconnect", &ClientPortalService::Disconnect);
        define("CreateChronicle", &ClientPortalService::CreateChronicle);
        define("DestroyChronicle", &ClientPortalService::DestroyChronicle);
        define("AcquireStory", &ClientPortalService::AcquireStory);
        define("ReleaseStory", &ClientPortalService::ReleaseStory);
        define("DestroyStory", &ClientPortalService::DestroyStory);
        define("GetChronicleAttr", &ClientPortalService::GetChronicleAttr);
        define("EditChronicleAttr", &ClientPortalService::EditChronicleAttr);
        define("ShowChronicles", &ClientPortalService::ShowChronicles);
        define("ShowStories", &ClientPortalService::ShowStories);
        //setup finalization callback in case this ser vice provider is still alive when the engine is finalized 
        std::cout<<"ClientPortalService::constructed at "<< get_engine().self()<<" provider_id {"<<service_provider_id<<"}"<<std::endl;        get_engine().push_finalize_callback(this, [p=this]() {delete p;});
    }

    ClientPortalService(ClientPortalService const&) =delete;
    ClientPortalService & operator= (ClientPortalService const&) =delete;


    VisorClientPortal &  theVisorClientPortal;
};


}// namespace chronolog

#endif
