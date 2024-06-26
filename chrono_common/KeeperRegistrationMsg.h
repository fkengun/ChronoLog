#ifndef KEEPER_REGISTRATION_MSG_H
#define KEEPER_REGISTRATION_MSG_H

#include <arpa/inet.h>
#include <iostream>

#include "KeeperIdCard.h"
#include "ServiceId.h"

namespace chronolog
{

class KeeperRegistrationMsg
{

    KeeperIdCard keeperIdCard;
    ServiceId adminServiceId;

public:
    KeeperRegistrationMsg(KeeperIdCard const& keeper_card = KeeperIdCard{0, 0, 0},
                          ServiceId const& admin_service_id = ServiceId{0, 0, 0})
        : keeperIdCard(keeper_card)
        , adminServiceId(admin_service_id)
    {}

    ~KeeperRegistrationMsg() = default;

    KeeperIdCard const &getKeeperIdCard() const
    { return keeperIdCard; }

    ServiceId const &getAdminServiceId() const
    { return adminServiceId; }

    template <typename SerArchiveT>
    void serialize(SerArchiveT &serT)
    {
        serT&keeperIdCard;
        serT&adminServiceId;
    }

};

}//namespace

inline std::ostream &operator<<(std::ostream &out, chronolog::KeeperRegistrationMsg const &msg)
{
    out << "KeeperRegistrationMsg{" << msg.getKeeperIdCard() << "}{admin:" << msg.getAdminServiceId() << "}";
    return out;
}

#endif
