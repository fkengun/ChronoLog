#ifndef INGESTION_QUEUE_H
#define INGESTION_QUEUE_H


#include <iostream>
#include <deque>
#include <unordered_map>
#include <mutex>

#include "chronolog_types.h"
#include "StoryIngestionHandle.h"

//
// IngestionQueue is a funnel into the MemoryDataStore
// std::deque guarantees O(1) time for addidng elements and resizing 
// (vector of vectors implementation)

namespace chronolog
{

typedef std::deque<LogEvent> EventDeque;

class IngestionQueue
{
public:
    IngestionQueue()
    {}

    ~IngestionQueue()
    { shutDown(); }

void addStoryIngestionHandle( StoryId const& story_id, StoryIngestionHandle * ingestion_handle)
{
    std::lock_guard<std::mutex> lock(ingestionQueueMutex);
    storyIngestionHandles.emplace(std::pair<StoryId,StoryIngestionHandle*>(story_id,ingestion_handle));
    std::cout <<"IngestionQueue: added handle for story {"<<story_id<<"} {"<<ingestion_handle<<"}"<<" handlesMap.size="<<storyIngestionHandles.size()<<std::endl;

    std::cout <<"IngestionQueue: addHandle : storyIngestionHandles {"<< &storyIngestionHandles<<"} .size="<<storyIngestionHandles.size()<<std::endl;

}

void removeIngestionHandle(StoryId const & story_id)
{
    std::cout <<"IngestionQueue: removeHandle : storyIngestionHandles {"<< &storyIngestionHandles<<"} .size="<<storyIngestionHandles.size()<<std::endl;
    std::lock_guard<std::mutex> lock(ingestionQueueMutex);
    storyIngestionHandles.erase(story_id);
    std::cout <<"IngestionQueue: removed handle for story {"<<story_id<<"}"<<" handlesMap.size="<<storyIngestionHandles.size()<<std::endl;
}

void ingestLogEvent(LogEvent const & event)
{
	std::cout <<"IngestionQueue: ingestLogEvent : storyIngestionHandles {"<< &storyIngestionHandles<<"} .size="<<storyIngestionHandles.size()<<std::endl;
	std::cout<<"IngestionQueue: received event {"<<event<< "}"<<std::endl;

	auto ingestionHandle_iter = storyIngestionHandles.find(event.storyId);
	if( ingestionHandle_iter == storyIngestionHandles.end())
	{
		std::cout <<" orphan event {"<<event<<"}"<<std::endl;
		std::lock_guard<std::mutex> lock(ingestionQueueMutex);
		orphanEventQueue.push_back(event);
	}
	else
	{       //individual StoryIngestionHandle has its own mutex
		(*ingestionHandle_iter).second->ingestEvent(event);
	}	
}

void drainOrphanEvents()
{
    if (orphanEventQueue.empty())
    { return; }

    std::lock_guard<std::mutex> lock(ingestionQueueMutex);
    for (EventDeque::iterator iter = orphanEventQueue.begin(); iter != orphanEventQueue.end();)
    {
        auto ingestionHandle_iter = storyIngestionHandles.find((*iter).storyId);
        if (ingestionHandle_iter != storyIngestionHandles.end())
        {    //individual StoryIngestionHandle has its own mutex
            (*ingestionHandle_iter).second->ingestEvent(*iter);
            //remove the event from the orphan deque and get the iterator to the next element prior to removal
            iter = orphanEventQueue.erase(iter);
        }
        else
        { ++iter; }
    }
}

bool is_empty() const
{
    return (orphanEventQueue.empty() && storyIngestionHandles.empty());
}

void shutDown()
{
    std::cout << "IngestionQueue: shutdown : storyIngestionHandles {" << storyIngestionHandles.size()
              << "} orphanEventQueue {" << orphanEventQueue.size() << "}" << std::endl;
    // last attempt to drain orphanEventQueue into known ingestionHandles
    drainOrphanEvents();
    // disengage all handles
    std::lock_guard<std::mutex> lock(ingestionQueueMutex);
    storyIngestionHandles.clear();
}

private:

    IngestionQueue(IngestionQueue const &) = delete;
    IngestionQueue &operator=(IngestionQueue const &) = delete;

    std::mutex ingestionQueueMutex;
    std::unordered_map<StoryId, StoryIngestionHandle *> storyIngestionHandles;

    // events for unknown stories or late events for closed stories will end up
    // in orphanEventQueue that we'll periodically try to drain into the DataStore
    std::deque<LogEvent> orphanEventQueue;

    //Timer to triger periodic attempt to drain orphanEventQueue and collect/log statistics
};

}

#endif

