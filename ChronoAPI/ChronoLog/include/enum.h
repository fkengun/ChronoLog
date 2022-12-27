//
// Created by kfeng on 4/1/22.
//

#ifndef CHRONOLOG_ENUM_H
#define CHRONOLOG_ENUM_H

typedef enum ChronoLogRPCImplementation {
    CHRONOLOG_THALLIUM_SOCKETS = 0,
    CHRONOLOG_THALLIUM_TCP = 1,
    CHRONOLOG_THALLIUM_ROCE = 2
} ChronoLogRPCImplementation;


typedef enum ChonoLogClientRoles 
{
  CHRONOLOG_CLIENT_ADMIN = 0,
  CHRONOLOG_CLIENT_USER = 1
}ChronoLogClientRoles;

#endif //CHRONOLOG_ENUM_H
