/*
 * AscEmu Framework based on ArcEmu MMORPG Server
 * Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
 * Copyright (C) 2008-2012 ArcEmu Team <http://www.ArcEmu.org/>
 * Copyright (C) 2005-2007 Ascent Team
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

#ifndef LOGON_COMM_CLIENT_H
#define LOGON_COMM_CLIENT_H

#include "CommonTypes.hpp"
#include "ByteBuffer.h"
#include "Network/Socket.h"
#include "LogonCommDefines.h"
#include "../shared/Log.hpp"
#include <RC4Engine.h>
#include "zlib.h"

#include <AENetwork/AENetworkClientInterface.hpp>
#include <AENetwork/AENetworkConnection.hpp>
#include <AENetwork/AENetworkPacket.hpp>
#include <AENetwork/AENetworkServerInterface.hpp>
#include <AENetwork/AENetworkCommon.hpp>
#include <AENetwork/AENetworkThreadsafeQueue.hpp>
#include <chrono>
#include <thread>

class LogonCommClientSocket : public Socket
{
    uint32 remaining;
    uint16 opcode;
    RC4Engine _sendCrypto;
    RC4Engine _recvCrypto;

    public:

        LogonCommClientSocket(SOCKET fd);
        ~LogonCommClientSocket();

        void OnRead();
        void SendPacket(WorldPacket* data, bool no_crypto);
        void HandlePacket(WorldPacket& recvData);
        void SendPing();
        void SendChallenge();
        void HandleAuthResponse(WorldPacket& recvData);

        void HandleRegister(WorldPacket& recvData);
        void HandlePong(WorldPacket& recvData);
        void HandleSessionInfo(WorldPacket& recvData);
        void HandleRequestAccountMapping(WorldPacket& recvData);
        void UpdateAccountCount(uint32 account_id, uint8 add);
        void HandleDisconnectAccount(WorldPacket& recvData);
        void HandleConsoleAuthResult(WorldPacket& recvData);
        void HandlePopulationRequest(WorldPacket& recvData);
        void HandleModifyDatabaseResult(WorldPacket& recvData);
        void HandleResultCheckAccount(WorldPacket& recvData);
        void HandleResultAllAccount(WorldPacket& recvData);

        void OnDisconnect();
        void CompressAndSend(ByteBuffer& uncompressed);
        uint32 last_ping;
        uint32 last_pong;

        uint32 pingtime;
        uint32 latency;
        uint32 _id;
        uint32 authenticated;
        bool use_crypto;
        std::set<uint32> realm_ids;
};

typedef void (LogonCommClientSocket::*logonpacket_handler)(WorldPacket&);

// MIT
namespace AENetwork
{
    class LogonCommClient : public ClientInterface<LogonCommTypes>
    {
    public:
        void tryToConnect()
        {
            isAuthenticated = false;

            LogNotice("NEW---LogonCommClient : Attempting to connect to logon server...");

            while (!isAuthenticated)
            {
                if (!isConnected())
                {
                    connectToServer("127.0.0.1", 8180);

                    std::this_thread::sleep_for(std::chrono::milliseconds(500));

                    if (!isConnected())
                    {
                        disconnect();

                        LogError("NEW---Connection failed. Will try again in 10 seconds.");

                        std::this_thread::sleep_for(std::chrono::seconds(10));
                    }
                }
                else
                {
                    LogNotice("NEW---LogonCommClient : Authenticating...");
                    sendAuth();

                    std::this_thread::sleep_for(std::chrono::milliseconds(500));

                    if (!incomingQueue().empty())
                    {
                        auto packet = incomingQueue().pop_front().packet;
                        onMessage(packet);

                        if (!isAuthenticated)
                        {
                            LogError("NEW---Authentication failed! Closing connection.");
                            disconnect();
                            break;
                        }

                        LogNotice("NEW---LogonCommClient : Successful authenticated");
                        break;
                    }

                    LogError("NEW---Authentication failed! Server not responding. Retry in 10 seconds.");

                    std::this_thread::sleep_for(std::chrono::seconds(10));
                }
            }
        }

        void update() override
        {
            if (isConnected())
            {
                if (!incomingQueue().empty())
                {
                    auto packet = incomingQueue().pop_front().packet;
                    onMessage(packet);
                }
            }
            else
            {
                tryToConnect();
            }
        }

        void sendAuth()
        {
            //todo: password encryption
            CmsgAuthRequest authRequest;
            authRequest.realmId = Config.MainConfig.getIntDefault("Realm1", "Id", 1);
            authRequest.password = worldConfig.logonServer.remotePassword;

            Packet<LogonCommTypes> packet;
            packet.header.id = LogonCommTypes::CMSG_AUTH_REQUEST;
            packet << authRequest.password;
            packet << authRequest.realmId;
            sendPacket(packet);
        }

    protected:

        void onMessage(Packet<LogonCommTypes>& packet) override
        {
            //TODO SMSG_REALM_REGISTER_RESULT, SMSG_ACC_SESSION_RESULT, SMSG_LOGON_PING_RESULT,
            // SMSG_ACC_CHAR_MAPPING_REQUEST, SMSG_SEND_ACCOUNT_DISCONNECT,
            // SMSG_LOGIN_CONSOLE_RESULT, SMSG_ACCOUNT_DB_MODIFY_RESULT, SMSG_REALM_POPULATION_REQUEST,
            // SMSG_ACCOUNT_RESULT, SMSG_ALL_ACCOUNT_RESULT

            switch (packet.header.id)
            {
                case LogonCommTypes::SMSG_AUTH_RESPONSE:
                    handleAuthResponse(packet);
                    break;
                default:
                    LogError("NEW---Unimplemented packet: %u in LogonCommClient::onMessage", packet.header.id);
                    break;
            }
        }

        void handleAuthResponse(Packet<LogonCommTypes>& packet)
        {
            SmsgAuthResponse authResponse;
            packet >> authResponse.result;

            std::cout << "NEW---Received CMSG_AUTH_REQUEST: Result: " << static_cast<uint32_t>(authResponse.result) << "\n";

            if (authResponse.result)
                isAuthenticated = true;
        }

    private:
        bool isAuthenticated = false;
    };
}

#endif // LOGON_COMM_CLIENT_H
