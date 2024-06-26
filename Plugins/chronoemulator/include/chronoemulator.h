#ifndef CHRONOKVS_CHRONOEMULATOR_H
#define CHRONOKVS_CHRONOEMULATOR_H

#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <unordered_map>
#include "../src/story.h"
#include "../src/chronicle.h"

namespace chronoemulator
{
class ChronoEmulator: public std::enable_shared_from_this <ChronoEmulator>
{
private:
    std::vector <std::shared_ptr <Chronicle>> chronicles;
    bool isConnected = false;

public:
    int Connect();

    int Disconnect();

    int CreateChronicle(const std::string &chronicle_name, const std::unordered_map <std::string, std::string> &attrs
                        , int &flags);

    int DestroyChronicle(const std::string &chronicle_name);

    std::pair <int, StoryHandle*> AcquireStory(const std::string &chronicle_name, const std::string &story_name
                                               , const std::unordered_map <std::string, std::string> &attrs
                                               , int &flags);

    int ReleaseStory(const std::string &chronicle_name, const std::string &story_name);

    int DestroyStory(const std::string &chronicle_name, const std::string &story_name);

};
}

#endif //CHRONOKVS_CHRONOEMULATOR_H
