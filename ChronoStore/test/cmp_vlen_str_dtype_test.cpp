//
// Created by kfeng on 7/6/23.
//

#include <iostream>
#include <sstream>
#include <random>
#include <vector>
#include <unordered_map>
#include <map>
#include <filesystem>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <hdf5.h>
#include <log.h>
#include <chronolog_errcode.h>
//#include <story_chunk_test_utils.h>

#define STORY "S1"
#define CHRONICLE "C1.h5"
#define NUM_OF_TESTS 200
#define DATASET_RANK 1
#define CHRONICLE_ROOT_DIR "/home/kfeng/chronolog_store/"
#define STORY_ID 1234567890
#define CLIENT_ID 1
#define CHRONICLE_NAME "Ares_Monitoring"
#define STORY_NAME "CPU_Utilization"

typedef uint64_t StoryId;
typedef uint64_t ChronicleId;
typedef uint32_t ClientId;
typedef uint64_t chrono_time;
typedef uint32_t chrono_index;
//typedef std::tuple<chrono_time, ClientId, chrono_index> EventSequence;

typedef struct LogEvent
{
    StoryId storyId;
    uint64_t eventTime;
    ClientId clientId;
    uint32_t eventIndex;
    uint64_t logRecordSize;
    char*logRecord;
} LogEvent;

LogEvent g_events[3];

typedef struct StoryChunk
{
    StoryId storyId;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t revisionTime;
    uint64_t numLogEvents;
    LogEvent*logEvents;

    [[nodiscard]] uint64_t getStoryID() const
    { return storyId; }

    [[nodiscard]] uint64_t getStartTime() const
    { return startTime; }

    [[nodiscard]] uint64_t getEndTime() const
    { return endTime; }

    [[nodiscard]] uint64_t getNumEvents() const
    { return numLogEvents; }

    [[nodiscard]] uint64_t getRevisionTime() const
    { return revisionTime; }

    [[nodiscard]] LogEvent*getLogEvents() const
    { return logEvents; }

    [[nodiscard]] size_t getTotalPayloadSize() const
    {
        size_t total_size = 0;
        for(uint64_t i = 0; i < numLogEvents; i++)
        { total_size += strlen(logEvents[i].logRecord) + 1; }
        return total_size;
    }
} StoryChunk;

void freeLogEvent(LogEvent &event)
{
    delete[] event.logRecord;
}

void freeStoryChunk(StoryChunk &chunk)
{
    for(uint64_t i = 0; i < chunk.numLogEvents; i++)
    {
        freeLogEvent(chunk.logEvents[i]);
    }
    delete[] chunk.logEvents;
}

StoryChunk*generateStoryChunkArray(uint64_t story_id, uint64_t client_id, uint64_t start_time, uint64_t time_step
                                   , uint64_t end_time, uint64_t mean_event_size, uint64_t min_event_size
                                   , uint64_t max_event_size, double stddev, uint64_t num_events_per_story_chunk
                                   , uint64_t num_story_chunks)
{
    auto*story_chunk_array = new StoryChunk[num_story_chunks];
    std::random_device rd;
    std::mt19937_64 rng(rd());
    std::normal_distribution <double> event_size_dist((double)mean_event_size, stddev);

    for(uint64_t i = 0; i < num_story_chunks; i++)
    {
        story_chunk_array[i].storyId = story_id;
        story_chunk_array[i].startTime = start_time;
        story_chunk_array[i].endTime = end_time;
        story_chunk_array[i].revisionTime = end_time;
        story_chunk_array[i].numLogEvents = num_events_per_story_chunk;
        story_chunk_array[i].logEvents = new LogEvent[num_events_per_story_chunk];

        for(uint64_t j = 0; j < num_events_per_story_chunk; j++)
        {
            story_chunk_array[i].logEvents[j].storyId = story_id;
            story_chunk_array[i].logEvents[j].eventTime = start_time + j * time_step;
            story_chunk_array[i].logEvents[j].clientId = client_id;
            story_chunk_array[i].logEvents[j].eventIndex = j;
            auto log_record_size = (size_t)event_size_dist(rng);
            log_record_size = log_record_size < min_event_size ? min_event_size : log_record_size > max_event_size
                                                                                  ? max_event_size : log_record_size;
            story_chunk_array[i].logEvents[j].logRecordSize = log_record_size;
            story_chunk_array[i].logEvents[j].logRecord = new char[log_record_size + 1];
            for(uint64_t k = 0; k < log_record_size; k++)
            {
                story_chunk_array[i].logEvents[j].logRecord[k] = 'a' + ((k + j) % 26);
            }
            story_chunk_array[i].logEvents[j].logRecord[log_record_size] = '\0';
        }

        story_id++;
    }

    return story_chunk_array;
}

std::map <uint64_t, StoryChunk>
generateStoryChunkMap(uint64_t story_id, uint64_t client_id, uint64_t start_time, uint64_t time_step, uint64_t end_time
                      , uint64_t mean_event_size, uint64_t min_event_size, uint64_t max_event_size, double stddev
                      , uint64_t num_events_per_story_chunk, uint64_t num_story_chunks)
{
    std::map <uint64_t, StoryChunk> story_chunk_map;
    std::random_device rd;
    std::mt19937_64 rng(rd());
    std::normal_distribution <double> event_size_dist((double)mean_event_size, stddev);

    for(uint64_t i = 0; i < num_story_chunks; i++)
    {
        StoryChunk story_chunk;
        story_chunk.storyId = story_id;
        story_chunk.startTime = start_time;
        story_chunk.endTime = end_time;
        story_chunk.revisionTime = end_time;
        story_chunk.numLogEvents = num_events_per_story_chunk;
        story_chunk.logEvents = new LogEvent[num_events_per_story_chunk];

        for(uint64_t j = 0; j < num_events_per_story_chunk; j++)
        {
            story_chunk.logEvents[j].storyId = story_id;
            story_chunk.logEvents[j].eventTime = start_time + j * time_step;
            story_chunk.logEvents[j].clientId = client_id;
            story_chunk.logEvents[j].eventIndex = j;
            auto log_record_size = (size_t)event_size_dist(rng);
            log_record_size = log_record_size < min_event_size ? min_event_size : log_record_size > max_event_size
                                                                                  ? max_event_size : log_record_size;
            story_chunk.logEvents[j].logRecordSize = log_record_size;
            story_chunk.logEvents[j].logRecord = new char[log_record_size + 1];
            for(uint64_t k = 0; k < log_record_size; k++)
            {
                story_chunk.logEvents[j].logRecord[k] = 'a' + ((k + j) % 26);
            }
            story_chunk.logEvents[j].logRecord[log_record_size] = '\0';
        }
        story_chunk_map.emplace(story_id, story_chunk);

        story_id++;
    }

    return story_chunk_map;
}

int main(int argc, char*argv[])
{
    if(argc < 8)
    {
        std::cout << "Usage: " << argv[0]
                  << " #StoryChunks #EventsInEachChunk meanEventSize minEventSize maxEventSize stddev"
                  << " startTime endTime" << std::endl;
        return 1;
    }
    int i, nCount = 3;
    size_t len;
    char*bptr;
    char const*str[] = {"Wake Me up When", "September", "Ends"};
    std::random_device rd;
    std::mt19937_64 rng(rd());
    std::uniform_int_distribution <uint64_t> dist(0, UINT64_MAX);
    uint64_t num_story_chunks = strtol(argv[1], nullptr, 10);
    uint64_t num_events_per_story_chunk = strtol(argv[2], nullptr, 10);
    uint64_t mean_event_size = strtol(argv[3], nullptr, 10);
    uint64_t min_event_size = strtol(argv[4], nullptr, 10);
    uint64_t max_event_size = strtol(argv[5], nullptr, 10);
    double stddev = strtod(argv[6], nullptr);
    uint64_t start_time = strtol(argv[7], nullptr, 10);
    uint64_t end_time = strtol(argv[8], nullptr, 10);
    std::string chronicle_name = CHRONICLE_NAME;
    std::string story_name = STORY_NAME;
    uint64_t story_id = STORY_ID; //dist(rng);
    uint64_t client_id = CLIENT_ID;

    std::cout << "StoryID: " << story_id << std::endl << "#StoryChunks: " << num_story_chunks << std::endl
              << "#EventsInEachChunk: " << num_events_per_story_chunk << std::endl << "meanEventSize: "
              << mean_event_size << std::endl << "minEventSize: " << min_event_size << std::endl << "maxEventSize: "
              << max_event_size << std::endl << "stddev: " << stddev << std::endl << "startTime: " << start_time
              << std::endl << "endTime: " << end_time << std::endl;

    for(i = 0; i < nCount; i++)
    {
        g_events[i].storyId = i + 10;
        g_events[i].eventTime = (i + 1) * 100;
        g_events[i].clientId = (i + 1) * 10;
        g_events[i].eventIndex = i;
        len = strlen(str[i]);
        g_events[i].logRecordSize = len;
        len = strlen(str[i]);
        if((bptr = (char*)malloc(++len * sizeof(char))))
        {

            strcpy(bptr, str[i]);

            g_events[i].logRecord = (char*)bptr;
        }
    }

    // Generate StoryChunks
    std::cout << "Generating StoryChunks..." << std::endl;
    uint64_t time_step = (end_time - start_time) / num_events_per_story_chunk;
//    StoryChunk*story_chunk_array = generateStoryChunkArray(story_id, client_id, start_time, time_step, end_time
//                                                           , mean_event_size, min_event_size, max_event_size, stddev
//                                                           , num_events_per_story_chunk, num_story_chunks);
    std::map <uint64_t, StoryChunk> story_chunk_map = generateStoryChunkMap(story_id, client_id, start_time, time_step
                                                                            , end_time, mean_event_size, min_event_size
                                                                            , max_event_size, stddev
                                                                            , num_events_per_story_chunk
                                                                            , num_story_chunks);

    hsize_t total_num_events = 0;
    std::vector <hsize_t> num_events;
    for(uint64_t j = 0; j < num_story_chunks; j++)
    {
//        hsize_t num_events_per_chunk = story_chunk_array[j].getNumEvents();
        hsize_t num_events_per_chunk = story_chunk_map[j].getNumEvents();
        num_events.push_back(num_events_per_chunk);
        total_num_events += num_events_per_chunk;
    }

    // Check if the directory for the Chronicle already exists
    struct stat st{};
    std::string chronicle_dir = CHRONICLE_ROOT_DIR + chronicle_name;
//    if (stat(chronicle_dir.c_str(), &st) != 0 || ! S_ISDIR(st.st_mode))
//    {
//        // Create the directory for the Chronicle
//        if (!std::filesystem::create_directory(chronicle_dir.c_str()))
//        {
//            LOGE("Failed to create chronicle directory: %s, errno: %d", chronicle_dir.c_str(), errno);
//            return CL_ERR_UNKNOWN;
//        }
//    }

    herr_t status;
    auto story_chunk_map_it = story_chunk_map.begin();
    std::unordered_map <uint64_t, hid_t> story_chunk_fd_map;
    for(; story_chunk_map_it != story_chunk_map.end(); story_chunk_map_it++)
//    for (uint64_t i = 0; i < 1; i++)
    {
        StoryChunk story_chunk = story_chunk_map_it->second;
//        StoryChunk story_chunk = story_chunk_array[i];
        // Check if the Story file has been opened/created already
        hid_t story_file;
        story_id = story_chunk.getStoryID();
        std::string story_file_name = chronicle_dir + "/" + std::to_string(story_id);
        story_file = H5Fcreate(story_file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
        LOGD("H5Fcreate returns: %li", story_file);
//        if (story_chunk_fd_map.find(story_id) != story_chunk_fd_map.end())
//        {
//            story_file = story_chunk_fd_map.find(story_id)->second;
//            LOGD("Story file already opened: %s", story_file_name.c_str());
//        }
        // Story file does not exist, create the Story file if it does not exist
//        else if (stat(story_file_name.c_str(), &st) != 0)
//        {
//            story_file = H5Fcreate(story_file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
//            if (story_file == H5I_INVALID_HID)
//            {
//                LOGE("Failed to create story file: %s", story_file_name.c_str());
//                return CL_ERR_UNKNOWN;
//            }
//            story_chunk_fd_map.emplace(story_id, story_file);
//        }
//        else
        {
            // Open the Story file if it exists
            story_file = H5Fopen(story_file_name.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
            if(story_file < 0)
            {
                LOGE("Failed to open story file: %s", story_file_name.c_str());
                return chronolog::CL_ERR_UNKNOWN;
            }
//            story_chunk_fd_map.emplace(story_id, story_file);
        }

        // Check if the dataset for the Story Chunk exists
        hid_t story_chunk_dset, story_chunk_dspace;
        std::string story_chunk_dset_name =
                std::to_string(story_chunk.getStartTime()) + "_" + std::to_string(story_chunk.getEndTime());
        if(H5Lexists(story_file, story_chunk_dset_name.c_str(), H5P_DEFAULT) > 0)
        {
            LOGE("Story chunk already exists: %s", story_chunk_dset_name.c_str());
            return chronolog::CL_ERR_STORY_CHUNK_EXISTS;
        }

        /**
         * H5T_C_S1 type based implementation
         */
        // Define variable-length payload data type
        hid_t event_payload_type = H5Tcopy(H5T_C_S1);
        LOGD("H5Tcopy(H5T_C_S1) returns: %li", event_payload_type);
        status = H5Tset_size(event_payload_type, H5T_VARIABLE);
        LOGD("H5Tset_size(event_payload_type, H5T_VARIABLE) returns: %i", status);
        hid_t vl_datatype_id = H5Tcopy(event_payload_type);
        LOGD("H5Tcopy(event_payload_type) returns: %li", vl_datatype_id);
        // Define LogEvent data type
        hid_t log_event_type = H5Tcreate(H5T_COMPOUND, sizeof(LogEvent));
        LOGD("H5Tcreate(H5T_COMPOUND, sizeof(LogEvent)) returns: %li", log_event_type);
        status = H5Tinsert(log_event_type, "storyId", HOFFSET(LogEvent, storyId), H5T_NATIVE_UINT64);
        LOGD("H5Tinsert returns: %i", status);
        status = H5Tinsert(log_event_type, "eventTime", HOFFSET(LogEvent, eventTime), H5T_NATIVE_UINT64);
        LOGD("H5Tinsert returns: %i", status);
        status = H5Tinsert(log_event_type, "clientId", HOFFSET(LogEvent, clientId), H5T_NATIVE_UINT32);
        LOGD("H5Tinsert returns: %i", status);
        status = H5Tinsert(log_event_type, "eventIndex", HOFFSET(LogEvent, eventIndex), H5T_NATIVE_UINT32);
        LOGD("H5Tinsert returns: %i", status);
        status = H5Tinsert(log_event_type, "logRecordSize", HOFFSET(LogEvent, logRecordSize), H5T_NATIVE_UINT64);
        LOGD("H5Tinsert returns: %i", status);
        status = H5Tinsert(log_event_type, "logRecord", HOFFSET(LogEvent, logRecord), vl_datatype_id);
        LOGD("H5Tinsert returns: %i", status);
        if(status < 0)
        {
            LOGE("Failed to create data type for log event, status: %i", status);
            return chronolog::CL_ERR_UNKNOWN;
        }

        // Create the dataspace for the dataset for the Story Chunk
        const hsize_t story_chunk_dset_dims[1] = {story_chunk.getNumEvents()};
        story_chunk_dspace = H5Screate_simple(DATASET_RANK, story_chunk_dset_dims, nullptr);
        LOGD("H5Screate_simple returns: %li", story_chunk_dspace);
        if(story_chunk_dspace < 0)
        {
            LOGE("Failed to create dataspace for story chunk: %s", story_chunk_dset_name.c_str());
            return chronolog::CL_ERR_UNKNOWN;
        }

        // Create the property list
        hid_t story_chunk_dset_creation_plist = H5Pcreate(H5P_DATASET_CREATE);
        LOGD("H5Pcreate returns: %li", story_chunk_dset_creation_plist);

        // Create the dataset for the Story Chunk
        // Set #chunks to number of Events in the Story Chunk
        hsize_t story_chunk_dset_chunk_dims[1] = {1}; //{ story_chunk.getNumEvents() };
        status = H5Pset_chunk(story_chunk_dset_creation_plist, DATASET_RANK, story_chunk_dset_chunk_dims);
        LOGD("H5Pset_chunk returns: %i", status);
//        story_chunk_dset = H5Dcreate2(story_file, story_chunk_dset_name.c_str(), story_chunk_type, story_chunk_dspace,
//                                      H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        story_chunk_dset = H5Dcreate(story_file, ("/" + story_chunk_dset_name).c_str(), log_event_type
                                     , story_chunk_dspace, H5P_DEFAULT, story_chunk_dset_creation_plist, H5P_DEFAULT);
        status = H5Pclose(story_chunk_dset_creation_plist);
        LOGD("H5Pclose returns: %i", status);
        if(story_chunk_dset < 0)
        {
            LOGE("Failed to create dataset for story chunk: %s", story_chunk_dset_name.c_str());
            return chronolog::CL_ERR_UNKNOWN;
        }

        // Create a contiguous memory space for the variable-length data
        // A end-of-string character is appended to each string
//        char *event_payload_data = new char[story_chunk.getTotalPayloadSize()];
//        size_t offset = 0;
//        LogEvent *log_events = story_chunk.getLogEvents();
//        for (int i = 0; i < story_chunk.getNumEvents(); ++i)
//        {
//            memcpy(event_payload_data + offset,
//                   log_events[i].logRecord,
//                   log_events[i].getPayloadSize());
//            offset += log_events[i].getPayloadSize();
//            event_payload_data[offset] = '\0';
//            offset += 1;
//        }

        // Write the Story Chunk to the dataset
        LOGD("%c", story_chunk.logEvents[0].logRecord[0]);
        LOGD("%c", story_chunk.logEvents[0].logRecord[7]);
        LOGD("%c", story_chunk.logEvents[9].logRecord[0]);
        LOGD("%c", story_chunk.logEvents[9].logRecord[7]);
//        status = H5Dwrite(story_chunk_dset, H5T_NATIVE_B8, H5S_ALL, H5S_ALL, H5P_DEFAULT,
//                          &story_chunk_map_it->second);
        status = H5Dwrite(story_chunk_dset, log_event_type, H5S_ALL, H5S_ALL, H5P_DEFAULT, &story_chunk.logEvents);
        LOGD("H5Dwrite returns: %i", status);
//        free(event_payload_data);
        if(status < 0)
        {
            LOGE("Failed to write story chunk to dataset: %s", story_chunk_dset_name.c_str());
            // Print the detailed error message
            H5Eprint(H5E_DEFAULT, stderr);

            // Get the error stack and retrieve the error message
            H5Ewalk2(H5E_DEFAULT, H5E_WALK_DOWNWARD
                     , [](unsigned n, const H5E_error2_t*err_desc, void*client_data) -> herr_t
                    {
                        printf("Error #%u: %s\n", n, err_desc->desc);
                        return 0;
                    }, nullptr);
            return chronolog::CL_ERR_UNKNOWN;
        }
        status = H5Dvlen_reclaim(log_event_type, story_chunk_dspace, H5P_DEFAULT, &g_events);
        LOGD("H5Dvlen_reclaim returns: %i", status);

        // Close file, dataset and data space
        status = H5Fclose(story_file);
        LOGD("H5Fclose returns: %i", status);
        status = H5Dclose(story_chunk_dset);
        LOGD("H5Dclose returns: %i", status);
        status = H5Sclose(story_chunk_dspace);
        LOGD("H5Sclose returns: %i", status);
        status = H5Tclose(log_event_type);
        LOGD("H5Tclose returns: %i", status);
        status = H5Tclose(event_payload_type);
        LOGD("H5Tclose returns: %i", status);
        status = H5Tclose(vl_datatype_id);
        LOGD("H5Tclose returns: %i", status);
        if(status < 0)
        {
            LOGE("Failed to close dataset or dataspace or file for story chunk: %s", story_chunk_dset_name.c_str());
            return chronolog::CL_ERR_UNKNOWN;
        }
    }

    // Close all files at the end
//    for (auto &story_chunk_fd_map_it : story_chunk_fd_map)
//    {
//        status = H5Fclose(story_chunk_fd_map_it.second);
//        if (status < 0)
//        {
//            char *story_file_name = new char[128];
//            size_t file_name_len = 0;
//            H5Fget_name(story_chunk_fd_map_it.second, story_file_name, file_name_len);
//            LOGE("Failed to close file for story chunk: %s", story_file_name);
//            free(story_file_name);
//            return CL_ERR_UNKNOWN;
//        }
//    }

    // Cleanup
    for(auto &story_chunk_map_it : story_chunk_map)
    { freeStoryChunk(story_chunk_map_it.second); }
}
