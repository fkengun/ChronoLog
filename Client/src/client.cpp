//
// Created by kfeng on 5/17/22.
//

#include <sys/types.h>
#include <unistd.h>
#include "client.h"
#include "city.h"

bool ChronoLogClient::Connect(const std::string &server_uri, std::string &client_id, int &client_role) {
    if (client_id.empty()) {
        char ip[16];
        struct hostent *he = gethostbyname("localhost");
        in_addr **addr_list = (struct in_addr **) he->h_addr_list;
        strcpy(ip, inet_ntoa(*addr_list[0]));
        std::string addr_str = ip + std::string(",") + std::to_string(getpid());
        uint64_t client_id_hash = CityHash64(addr_str.c_str(), addr_str.size());
        client_id = std::to_string(client_id_hash);
    }
    client_id_ = client_id;
    if(client_role < 0 || client_role > 1) client_role = 1;
    return adminRpcProxy_->Connect(server_uri, client_id,client_role);
}

bool ChronoLogClient::Disconnect(const std::string &client_id, int &flags) {
    return adminRpcProxy_->Disconnect(client_id, flags);
}

bool ChronoLogClient::CreateChronicle(std::string &client_id, std::string &name, const std::unordered_map<std::string, std::string> &attrs) {
    return metadataRpcProxy_->CreateChronicle(client_id,name, attrs);
}

bool ChronoLogClient::DestroyChronicle(std::string &client_id, std::string &name, const int &flags) {
    return metadataRpcProxy_->DestroyChronicle(client_id, name, flags);
}

bool ChronoLogClient::AcquireChronicle(std::string &client_id, std::string &name, const int &flags) {
    return metadataRpcProxy_->AcquireChronicle(client_id, name, flags);
}

bool ChronoLogClient::ReleaseChronicle(std::string &client_id, std::string &name, const int &flags) {
    return metadataRpcProxy_->ReleaseChronicle(client_id, name, flags);
}

bool ChronoLogClient::CreateStory(std::string &client_id, std::string &chronicle_name, std::string &story_name, const std::unordered_map<std::string, std::string> &attrs) {
    return metadataRpcProxy_->CreateStory(client_id, chronicle_name, story_name, attrs);
}

bool ChronoLogClient::DestroyStory(std::string &client_id, std::string &chronicle_name, std::string &story_name, const int &flags) {
    return metadataRpcProxy_->DestroyStory(client_id, chronicle_name, story_name, flags);
}

bool ChronoLogClient::AcquireStory(std::string &client_id, std::string &chronicle_name, std::string &story_name, const int &flags) {
    return metadataRpcProxy_->AcquireStory(client_id, chronicle_name, story_name, flags);
}

bool ChronoLogClient::ReleaseStory(std::string &client_id, std::string &chronicle_name, std::string &story_name, const int &flags) {
    return metadataRpcProxy_->ReleaseStory(client_id, chronicle_name, story_name, flags);
}

std::string ChronoLogClient::GetChronicleAttr(std::string &client_id, std::string &chronicle_name, const std::string &key) {
    return metadataRpcProxy_->GetChronicleAttr(client_id, chronicle_name, key);
}

bool ChronoLogClient::EditChronicleAttr(std::string &client_id, std::string &chronicle_name, const std::string &key, const std::string &value) {
    return metadataRpcProxy_->EditChronicleAttr(client_id, chronicle_name, key, value);
}

bool ChronoLogClient::SetClientId(std::string &client_id)
{
	client_id_ = client_id;
	return true;
}

std::string& ChronoLogClient::GetClientId()
{
	return client_id_;
}
