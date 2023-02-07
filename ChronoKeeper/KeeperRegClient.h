
#ifndef KEEPER_REG_CLIENT_H
#define KEEPER_REG_CLIENT_H

#include <iostream>
#include <thallium/serialization/stl/string.hpp>
#include <thallium.hpp>

#include "KeeperIdCard.h"
#include "KeeperStatsMsg.h"

namespace tl = thallium;


namespace chronolog
{


class KeeperRegistryClient
{

public:
    static KeeperRegistryClient * CreateKeeperRegistryClient( tl::engine & tl_engine,
		    std::string const & registry_service_addr, uint16_t registry_provider_id )
    	{
	   try{
	      return new KeeperRegistryClient( tl_engine,registry_service_addr, registry_provider_id);
	   } catch( tl::exception const&)
	   {
		std::cout<<"KeeperRegistryClient: failed construction"<<std::endl;
		return nullptr;
	   }
        }

    int send_register_msg( KeeperIdCard const& keeperIdCard)
    {
	 std::cout<< "KeeperRegisterClient::send_register_msg:"<<keeperIdCard<<std::endl;
	 return register_keeper.on(reg_service_ph)(keeperIdCard);
    }
    int send_unregister_msg( KeeperIdCard const& keeperIdCard)
    {
	 std::cout<< "KeeperRegisterClient::send_unregister_msg:"<<keeperIdCard<<std::endl;
	 return unregister_keeper.on(reg_service_ph)(keeperIdCard);
    }

    void send_stats_msg(KeeperStatsMsg const& keeperStatsMsg)
    {
	std::cout<< "KeeperRegisterClient::send_stats_msg:"<<keeperStatsMsg<<std::endl;
	handle_stats_msg.on(reg_service_ph)(keeperStatsMsg);
    }

    ~KeeperRegistryClient()
    {
	   register_keeper.deregister();
	   unregister_keeper.deregister();
           handle_stats_msg.deregister(); 
    }

    private:


    std::string reg_service_addr;     // na address of Keeper Registry Service 
    uint16_t 	reg_service_provider_id;          // KeeperRegistryService provider id
    tl::provider_handle  reg_service_ph;  //provider_handle for remote registry service
    tl::remote_procedure register_keeper;
    tl::remote_procedure unregister_keeper;
    tl::remote_procedure handle_stats_msg;

    // constructor is private to make sure thalium rpc objects are created on the heap, not stack
    KeeperRegistryClient( tl::engine & tl_engine, std::string const& registry_addr, uint16_t registry_provider_id)
	    : reg_service_addr(registry_addr), reg_service_provider_id(registry_provider_id)
	    , reg_service_ph(tl_engine.lookup( registry_addr),registry_provider_id)
	{
   	 register_keeper = tl_engine.define("register_keeper");
   	 unregister_keeper =tl_engine.define("unregister_keeper"); 
   	 handle_stats_msg =tl_engine.define("handle_stats_msg").disable_response();
       
	}	
};

}

#endif