#ifndef CHRONO_SERVICE_ID_H
#define CHRONO_SERVICE_ID_H

#include <arpa/inet.h>
#include <iostream>


namespace chronolog
{

typedef uint32_t RecordingGroupId;

class ServiceId
{
public:
    ServiceId(uint32_t addr, uint16_t a_port, uint16_t a_provider_id)
        : ip_addr(addr)
        , port(a_port)
        , provider_id(a_provider_id)
    {}

    ~ServiceId() = default;

    uint32_t ip_addr;    //32int IP representation in host notation
    uint16_t port;       //16int port representation in host notation
    uint16_t provider_id;//thalium provider id

    template <typename SerArchiveT>
    void serialize(SerArchiveT& serT)
    {
        serT& ip_addr;
        serT& port;
        serT& provider_id;
    }

    std::string& getIPasDottedString(std::string& a_string) const
    {

        char buffer[INET_ADDRSTRLEN];
        // convert ip from host to network byte order uint32_t
        uint32_t ip_net_order = htonl(ip_addr);
        // convert network byte order uint32_t to a dotted string
        if(NULL != inet_ntop(AF_INET, &ip_net_order, buffer, INET_ADDRSTRLEN)) { a_string += std::string(buffer); }
        return a_string;
    }
};

}//namespace chronolog


inline std::ostream& operator<<(std::ostream& out, chronolog::ServiceId const serviceId)
{
    std::string a_string;
    out << "ServiceId{" << serviceId.getIPasDottedString(a_string) << ":" << serviceId.port << ":" << serviceId.provider_id
        << "}";
    return out;
}

inline std::string& operator+= (std::string& a_string, chronolog::ServiceId const& serviceId)
{
    a_string += std::string("ServiceId{") + serviceId.getIPasDottedString(a_string) + ":" + std::to_string(serviceId.port) + ":" +
                std::to_string(serviceId.provider_id) + "}";
    return a_string;
}

#endif
