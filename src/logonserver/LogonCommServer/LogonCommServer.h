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
        virtual bool onClientConnect(std::shared_ptr<Connection<LogonCommTypes>> client)
        {
            // banned ip...
            return true;
        }

        // called when a client appears to have disconnected
        virtual void onClientDisconnect(std::shared_ptr<Connection<LogonCommTypes>> client)
        {
            std::cout << "Removing client [" << client->getId() << "]\n";
        }

        //called when a packet arrives
        virtual void onMessage(std::shared_ptr<Connection<LogonCommTypes>> client, Packet<LogonCommTypes>& packet)
        {
            std::cout << "[SERVER] OnMessage called!\n";

            if (!client->isClientAuthorized())
                std::cout << "[SERVER] client ["<< client->getId() <<"] is not authorized!\n";

            switch (packet.header.id)
            {
                case LogonCommTypes::CMSG_REALM_REGISTER_REQUEST:
                {
                    std::cout << "[" << client->getId() << "]: Auth Register Request\n";
                } break;
                case LogonCommTypes::CMSG_AUTH_REQUEST:
                {
                    std::cout << "[" << client->getId() << "]: Auth Request\n";
                    //read packet <<

                    AuthRequest ar;
                    packet >> ar;

                    std::cout << "[" << client->getId() << "]: Received Request: Random: " << ar.random << " Key: " << ar.key << "\n";

                    // ckeck security

                    // create new packet
                    AENetwork::Packet<AENetwork::LogonCommTypes> packetOut;
                    packetOut.header.id = AENetwork::LogonCommTypes::SMSG_AUTH_RESPONSE;
                    packetOut << uint32_t(1);
                    client->sendPacket(packetOut);
                } break;
            default:
            {
                // unknown packet opcode/header.id, disconnect!
                client->disconnect();
            } break;
            }
        }
    };
}

#endif  // __LOGON_COMM_SERVER_H
