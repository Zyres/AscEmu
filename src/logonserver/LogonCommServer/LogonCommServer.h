/*
 * AscEmu Framework based on ArcEmu MMORPG Server
 * Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
 * Copyright (C) 2008-2012 ArcEmu Team <http://www.ArcEmu.org/>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LOGON_COMM_SERVER_H
#define __LOGON_COMM_SERVER_H

#include <RC4Engine.h>
#include "zlib.h"

class LogonCommServerSocket : public Socket
{
    uint32 remaining;
    uint16 opcode;
    uint32 seed;
    RC4Engine sendCrypto;
    RC4Engine recvCrypto;

    public:

        uint32 authenticated;
        bool use_crypto;

        LogonCommServerSocket(SOCKET fd);
        ~LogonCommServerSocket();

        void OnRead();
        void OnDisconnect();
        void OnConnect();
        void SendPacket(WorldPacket* data);
        void HandlePacket(WorldPacket & recvData);

        void HandleRegister(WorldPacket& recvData);
        void HandlePing(WorldPacket& recvData);
        void HandleSessionRequest(WorldPacket& recvData);
        void HandleSQLExecute(WorldPacket& recvData);
        void HandleReloadAccounts(WorldPacket& recvData);
        void HandleAuthChallenge(WorldPacket& recvData);
        void HandleMappingReply(WorldPacket& recvData);
        void HandleUpdateMapping(WorldPacket& recvData);
        void HandleTestConsoleLogin(WorldPacket& recvData);
        void HandleDatabaseModify(WorldPacket& recvData);
        void HandlePopulationRespond(WorldPacket& recvData);
        void HandleRequestCheckAccount(WorldPacket& recvData);
        void HandleRequestAllAccounts(WorldPacket& recvData);

        void RefreshRealmsPop();

        std::atomic<unsigned long> last_ping;
        bool removed;
        std::set<uint32> server_ids;
};

typedef void (LogonCommServerSocket::*logonpacket_handler)(WorldPacket&);

//MIT
namespace AENetwork
{
    class LogonCommServer : public ServerInterface<LogonCommTypes>
    {
    public:
        LogonCommServer(uint16_t port) : ServerInterface<LogonCommTypes>(port)
        {

        }

    protected:
        //called when client connects, we can refuse the connection
        bool onClientConnect(std::shared_ptr<Connection<LogonCommTypes>> client) override
        {
            // banned ip...
            return true;
        }

        // called when a client appears to have disconnected
        void onClientDisconnect(std::shared_ptr<Connection<LogonCommTypes>> client) override
        {
            std::cout << "Removing client [" << client->getId() << "]\n";
        }

        //called when a packet arrives
        void onMessage(std::shared_ptr<Connection<LogonCommTypes>> client, Packet<LogonCommTypes>& packet) override
        {
            if (!client->isClientAuthorized())
                std::cout << "[SERVER] client ["<< client->getId() <<"] is not authorized!\n";

            //TODO CMSG_REALM_REGISTER_REQUEST, CMSG_ACC_SESSION_REQUEST, CMSG_LOGON_PING_STATUS,
            // CMSG_AUTH_REQUEST, CMSG_ACC_CHAR_MAPPING_RESULT, CMSG_ACC_CHAR_MAPPING_UPDATE,
            // CMSG_ACCOUNT_DB_MODIFY_REQUEST, CMSG_REALM_POPULATION_RESULT, CMSG_ACCOUNT_REQUEST,
            // CMSG_ALL_ACCOUNT_REQUEST, CMSG_LOGIN_CONSOLE_REQUEST,

            switch (packet.header.id)
            {
                case LogonCommTypes::CMSG_AUTH_REQUEST:
                    handleAuthRequest(client, packet);
                    break;
                default:
                    LogError("Unimplemented packet: %u in LogonCommClient::onMessage", packet.header.id);
                    break;
            }
        }

        void handleAuthRequest(std::shared_ptr<Connection<LogonCommTypes>> client, Packet<LogonCommTypes>& packet)
        {
            std::cout << "[" << client->getId() << "]: Auth Request\n";

            //read packet <<
            std::string sql_passhash;
            uint8_t realmId = 0;

            packet >> sql_passhash;
            packet >> realmId;

            const auto realm = sRealmsMgr.getRealmById(realmId);
            if (realm == nullptr)
            {
                LogError("Realm %u is missing in table realms. Please add the server to your realms table.", static_cast<uint32_t>(realmId));
                return;
            }

            LogDefault("Password from packet %s; password in db %s", sql_passhash.c_str(), realm->password.c_str());

            // check if we have the correct password
            bool result = true;

            if (sql_passhash.compare(realm->password))
                result = false;

            LogDefault("Authentication request from %u, id %u - result %s.", client->getId(), static_cast<uint32_t>(realmId), result ? "OK" : "FAIL");


            client->setClientAuth(result);

            // ckeck security

            // create new packet
            Packet<LogonCommTypes> packetOut;
            packetOut.header.id = LogonCommTypes::SMSG_AUTH_RESPONSE;
            packetOut << result;
            client->sendPacket(packetOut);
        }
    };
}

#endif  // __LOGON_COMM_SERVER_H
