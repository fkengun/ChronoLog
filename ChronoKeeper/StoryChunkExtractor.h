#ifndef STORY_CHUNK_EXTRACTOR_H
#define STORY_CHUNK_EXTRACTOR_H


#include <iostream>
#include <deque>
#include <vector>
#include <mutex>
#include <thallium.hpp>

#include "chrono_common/chronolog_types.h"
#include "StoryChunkExtractionQueue.h"


namespace tl = thallium;

namespace chronolog
{


class StoryChunkExtractorBase
{

enum ExtractorState
{
    UNKNOWN = 0,
    RUNNING =1, //  active extraction threads 
    SHUTTING_DOWN=2 // Shutting down extraction threads
};


public:
    StoryChunkExtractorBase()
            : extractorState(UNKNOWN)
    {}

    ~StoryChunkExtractorBase();

    StoryChunkExtractionQueue & getExtractionQueue()
    {
        std::cout <<"ExtractionBase : chunkExtractionQueue.size="<<chunkExtractionQueue.size()<<std::endl;
        return chunkExtractionQueue;
    }

    bool is_running() const
    {  return (extractorState == RUNNING);  }

    bool is_shutting_down() const
    {   return (extractorState == SHUTTING_DOWN);   }

    void drainExtractionQueue(); 

    virtual void processStoryChunk( StoryChunk*)  //=0
    { }

    void startExtractionThreads(int);
    void shutdownExtractionThreads();

private:

    StoryChunkExtractorBase(StoryChunkExtractorBase const &) = delete;
    StoryChunkExtractorBase &operator=(StoryChunkExtractorBase const &) = delete;

    ExtractorState extractorState;
    std::mutex      extractorMutex;
    StoryChunkExtractionQueue chunkExtractionQueue;

    std::vector<tl::managed<tl::xstream>> extractionStreams; 
    std::vector<tl::managed<tl::thread>> extractionThreads; 
};

}

#endif

