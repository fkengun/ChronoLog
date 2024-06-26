#ifndef CHRONOKVS_CHRONOKVS_MEMORYMANAGER_H
#define CHRONOKVS_CHRONOKVS_MEMORYMANAGER_H

#include <string>
#include <vector>
#include <utility>
#include <unordered_map>

namespace chronokvs
{
class KeyToTimestampMappingManager
{
private:
    std::unordered_multimap <std::string, std::uint64_t> dataStore;
public:
    KeyToTimestampMappingManager() = default;

    ~KeyToTimestampMappingManager() = default;

    void store(const std::string &key, std::uint64_t timestamp);

    std::vector <std::uint64_t> retrieveByKey(const std::string &key);
};
}


#endif //CHRONOKVS_CHRONOKVS_MEMORYMANAGER_H
