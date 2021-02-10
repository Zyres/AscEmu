/*
Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

#pragma once

#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <deque>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <optional>

#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

namespace AENetwork
{
    enum class LogonCommTypes : uint16_t
    {
        CMSG_REALM_REGISTER_REQUEST = 0x001,  // try register our realm
        SMSG_REALM_REGISTER_RESULT = 0x002,  // register result from logonserver
        CMSG_ACC_SESSION_REQUEST = 0x003,
        SMSG_ACC_SESSION_RESULT = 0x004,
        CMSG_LOGON_PING_STATUS = 0x005,  // request logon online
        SMSG_LOGON_PING_RESULT = 0x006,  // send result if logon is online
        CMSG_FREE_01 = 0x007,  // unused
        SMSG_FREE_02 = 0x008,  // unused
        CMSG_AUTH_REQUEST = 0x009,  // try authenticate our realm
        SMSG_AUTH_RESPONSE = 0x00A,  // authentication result from logonserver
        SMSG_ACC_CHAR_MAPPING_REQUEST = 0x00B,
        CMSG_ACC_CHAR_MAPPING_RESULT = 0x00C,
        CMSG_ACC_CHAR_MAPPING_UPDATE = 0x00D,
        SMSG_SEND_ACCOUNT_DISCONNECT = 0x00E,  // send when account is disconnected
        CMSG_LOGIN_CONSOLE_REQUEST = 0x00F,
        SMSG_LOGIN_CONSOLE_RESULT = 0x010,
        CMSG_ACCOUNT_DB_MODIFY_REQUEST = 0x011,  // request logon db change
        SMSG_ACCOUNT_DB_MODIFY_RESULT = 0x012,
        SMSG_REALM_POPULATION_REQUEST = 0x013,
        CMSG_REALM_POPULATION_RESULT = 0x014,
        CMSG_ACCOUNT_REQUEST = 0x015,  // request account data
        SMSG_ACCOUNT_RESULT = 0x016,  // send account information to realm
        CMSG_ALL_ACCOUNT_REQUEST = 0x017,  // request all account data
        SMSG_ALL_ACCOUNT_RESULT = 0x018,  // send id, name, rank of all accounts to realm

        MSG_MAX_OPCODES                           // max opcodes
    };

    struct CmsgRegisterRequest
    {};

    struct SmsgRegisterResult
    {};

    struct CmsgSessionRequest
    {};

    struct SmsgSessionResult
    {};

    struct CmsgPingStatus
    {};

    struct SmsgPingResult
    {};

    struct CmsgAuthRequest
    {
        uint8_t* key;
        uint8_t realmId;
    };

    struct SmsgAuthResponse
    {};

    struct SmsgCharMappingRequest
    {};

    struct CmsgCharMappingResult
    {};

    struct SmsgAccountDisconnect
    {};

    struct CmsgConsoleRequest
    {};

    struct SmsgConsoleResult
    {};

    struct CmsgDBModifyRequest
    {};

    struct SmsgDBModifyResult
    {};

    struct SmsgRealmPopulationRequest
    {};

    struct CmsgRealmPolulationResult
    {};

    struct CmsgCheckAccountRequest
    {};

    struct SmsgCheckAccountResult
    {};

    struct CmsgAllAccountsRequest
    {};

    struct SmsgAllAccountsResult
    {};
}
