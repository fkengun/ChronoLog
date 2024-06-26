#include "chronokvs_memorymanager.h"

namespace chronokvs
{

void KeyToTimestampMappingManager::store(const std::string &key, std::uint64_t timestamp)
{
    dataStore.insert({key, timestamp});
}

std::vector <std::uint64_t> KeyToTimestampMappingManager::retrieveByKey(const std::string &key)
{
    std::vector <std::uint64_t> timestamps;

    // Find the range of elements that match the given key
    auto range = dataStore.equal_range(key);

    // Iterate over the range and add all matching timestamps to the vector
    for(auto it = range.first; it != range.second; ++it)
    {
        timestamps.push_back(it->second); // it->second is the timestamp
    }

    return timestamps;
}
}



