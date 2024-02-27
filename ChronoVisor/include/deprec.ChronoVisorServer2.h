#ifndef CHRONOLOG_CHRONOVISORSERVER2_H
#define CHRONOLOG_CHRONOVISORSERVER2_H

//#include <unordered_map>
#include <vector>
//#include <mutex>
//#include <atomic>
#include <log.h>
#include <thallium.hpp>
//#include <margo.h>
//#include <ClocksourceManager.h>
//#include <ChronicleMetaDirectory.h>
//#include <ClientRegistryManager.h>
#include <deprec.RPCVisor.h>

class ClocksourceManager;

namespace ChronoVisor
{
namespace tl = thallium;

class ChronoVisorServer2
{
public:
    explicit ChronoVisorServer2(const std::string &conf_file_path = "");

    explicit ChronoVisorServer2(const ChronoLog::ConfigurationManager &conf_manager);

    int start(chronolog::KeeperRegistry*);

private:
    void init();

    std::string protocol_;
    std::string baseIP_;
    int basePorts_{};
    int numPorts_{};
    std::vector <std::string> serverAddrVec_;
    int numStreams_{};
    std::vector <tl::engine> engineVec_;
    std::vector <margo_instance_id> midVec_;

    /**
     * @name ClientRegistry related variables
     */
    ///@{
    //std::shared_ptr<ClientRegistryManager> clientRegistryManager_;
    ///@}

    /**
     * @name Chronicle Meta Directory related variables
     */
    ///@{
    //std::shared_ptr<ChronicleMetaDirectory> chronicleMetaDirectory_;
    ///@}

    /**
     * @name RPC related variables
     */
    ///@{
    std::shared_ptr <RPCVisor> rpcVisor_;
    ///@}

    /**
     * @name Clock related variables
     */
    ///@{
    ClocksourceManager*pClocksourceManager_;
    ///@}
};
}


#endif //CHRONOLOG_CHRONOVISORSERVER2_H
