#ifndef CHRONOLOG_CHRONICLEMETADIRECTORY_H
#define CHRONOLOG_CHRONICLEMETADIRECTORY_H

#include <string>
#include <unordered_map>
#include <vector>
#include <Chronicle.h>
#include "chronolog_types.h" 

class ClientRegistryManager;

typedef uint64_t StoryId;

class ChronicleMetaDirectory {
public:
    ChronicleMetaDirectory();
    ~ChronicleMetaDirectory();

    void set_client_registry_manager(ClientRegistryManager *pClientRegistryManager) {
        clientRegistryManager_ = pClientRegistryManager;
    }

    std::unordered_map<uint64_t, Chronicle *> *getChronicleMap() { return chronicleMap_; }

    int create_chronicle(const std::string& name);
    int create_chronicle(const std::string& name, const std::unordered_map<std::string, std::string>& attrs);
    int destroy_chronicle(const std::string& name);

    int create_story(std::string const& chronicle_name, const std::string& story_name,
                     const std::unordered_map<std::string, std::string>& attrs);
    int destroy_story(std::string const& chronicle_name, const std::string& story_name);
    int acquire_story(chronolog::ClientId const & client_id, const std::string& chronicle_name,
                      const std::string& story_name, int& flags, StoryId &, bool&);
    int release_story(chronolog::ClientId const & client_id, const std::string& chronicle_name,
                      const std::string& story_name, StoryId &, bool&);

    int get_chronicle_attr(std::string const& name, const std::string& key, std::string& value);
    int edit_chronicle_attr(std::string const& name, const std::string& key, const std::string& value);

    int show_chronicles(std::vector<std::string>&);
    int show_stories( const std::string& chronicle_name, std::vector<std::string> &);

private:
    std::unordered_map<uint64_t , Chronicle *> *chronicleMap_;
    std::mutex g_chronicleMetaDirectoryMutex_;
    ClientRegistryManager *clientRegistryManager_ = nullptr;
//    std::unordered_map<std::string, uint64_t> *chronicleName2IdMap_;
//    std::unordered_map<uint64_t, std::string> *chronicleId2NameMap_;
};

#endif //CHRONOLOG_CHRONICLEMETADIRECTORY_H
