/*
Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

#pragma once

#include <AENetwork/AENetworkCommon.hpp>
#include <AENetwork/AENetworkPacket.hpp>

namespace AENetwork
{
    //allowing creation of a shared pointer within this object.
    template <typename T>
    class Connection : public std::enable_shared_from_this<Connection<T>>
    {
    public:

        enum class ConnectionOwner
        {
            Server = 0,
            Client = 1
        };

        Connection(ConnectionOwner ownerType, asio::io_context& asioContext, asio::ip::tcp::socket socket, ThreadsafeQueue<OwnedPacket<T>>& queueIn)
            : m_asioContext(asioContext), m_socket(std::move(socket)), m_queuePacketsIn(queueIn)
        {
            m_ConnectionOwnerType = ownerType;
        }

        virtual ~Connection() {}

        uint32_t getId() const { return id; }

        void connectToClient(uint32_t clientId = 0)
        {
            // only for server
            if (m_ConnectionOwnerType == ConnectionOwner::Server)
            {
                if (m_socket.is_open())
                {
                    id = clientId;
                    readHeaderFromPacket();
                }
            }
        }

        // can be called only for clients
        void connectToServer(const asio::ip::tcp::resolver::results_type& tcpEndpoint)
        {
            // only for clients
            if (m_ConnectionOwnerType == ConnectionOwner::Client)
            {
                // give asio something to do
                asio::async_connect(m_socket, tcpEndpoint,
                    [this](std::error_code errorCode, asio::ip::tcp::endpoint tcpEndpoint)
                    {
                        if (!errorCode)
                        {
                            readHeaderFromPacket();
                        }
                    });
            }
        }

        //can be called by server and clients
        void disconnect()
        {
            if (isConnected())
                asio::post(m_asioContext, [this]() { m_socket.close(); });
        }

        //can be called by server and clients
        bool isConnected() const { return m_socket.is_open(); }

        bool isClientAuthorized() const { return isAuthorized; }

        // can be called by server
        void setClientAuth(bool auth)
        {
            if (m_ConnectionOwnerType == ConnectionOwner::Server)
            {
                isAuthorized = auth;
            }
        }

        // send packet with the connection
        void sendPacket(const Packet<T>& packet)
        {
            asio::post(m_asioContext,
                [this, packet]()
                {
                    bool bWritinMessage = !m_queuePacketsOut.empty();
                    m_queuePacketsOut.push_back(packet);
                    if (!bWritinMessage)
                    {
                        writeHeaderToPacket();
                    }
                });
        }

    private:
        //async prime context ready to read a packet header
        void readHeaderFromPacket()
        {
            asio::async_read(m_socket, asio::buffer(&m_packetsTempIn.header, sizeof(PacketHeader<T>)),
                [this](std::error_code errorCode, std::size_t headerSize)
                {
                    if (!errorCode)
                    {
                        if (m_packetsTempIn.header.size > 0)
                        {
                            std::cout << "[" << id << "] " << "Received packet header with headerId: " << uint32_t(m_packetsTempIn.header.id) << " length: " << m_packetsTempIn.header.size << "\n";

                            // make enough space to store the packet body
                            m_packetsTempIn.body.resize(m_packetsTempIn.header.size);
                            readBodyFromPacket();
                        }
                        else
                        {
                            // if we have no body data
                            addToIncomingPacketQueue();
                        }
                    }
                    else
                    {
                        std::cout << "[" << id << "] readHeaderFromPacket Fail.\n";
                        m_socket.close();
                    }
                });
        }

        //async prime context ready to read a packet body
        void readBodyFromPacket()
        {
            std::cout << "[" << id << "] " << "readBodyFromPacket is called\n";
            std::cout << "[" << id << "] " << "Size: " << m_packetsTempIn.body.size() << " Data: " << m_packetsTempIn.body.data() << "\n";

            asio::async_read(m_socket, asio::buffer(m_packetsTempIn.body.data(), m_packetsTempIn.body.size()),
                [this](std::error_code errorCode, std::size_t sizeBody)
                {
                    if (!errorCode)
                    {
                        std::cout << "[" << id << "] " << "Body read " << m_packetsTempIn.body.size() << "\n";
                        // ...and they have! The message is now complete, so add
                        // the whole message to incoming queue
                        addToIncomingPacketQueue();
                    }
                    else
                    {
                        // As above!
                        std::cout << "[" << id << "] readBodyFromPacket Fail.\n";
                        m_socket.close();
                    }
                });
        }

        //async prime context ready to write a packet header
        void writeHeaderToPacket()
        {
            asio::async_write(m_socket, asio::buffer(&m_queuePacketsOut.front(), sizeof(PacketHeader<T>)),
                [this](std::error_code errorCode, std::size_t sizeHeader)
                {
                    if (!errorCode)
                    {
                        if (m_queuePacketsOut.front().body.size() > 0)
                        {
                            writeBodyToPacket();
                        }
                        else
                        {
                            m_queuePacketsOut.pop_front();

                            if (!m_queuePacketsOut.empty())
                            {
                                writeHeaderToPacket();
                            }
                        }
                    }
                    else
                    {
                        std::cout << "[" << id << "] writeHeaderToPacket Fail.\n";
                        m_socket.close();
                    }
                });
        }

        //async prime context ready to write a packet body
        void writeBodyToPacket()
        {
            asio::async_write(m_socket, asio::buffer(m_queuePacketsOut.front().body.data(), m_queuePacketsOut.front().body.size()),
                [this](std::error_code errorCode, std::size_t sizeBody)
                {
                    if (!errorCode)
                    {

                        m_queuePacketsOut.pop_front();

                        if (!m_queuePacketsOut.empty())
                        {
                            writeHeaderToPacket();
                        }
                    }
                    else
                    {
                        std::cout << "[" << id << "] writeBodyToPacket Fail.\n";
                        m_socket.close();
                    }
                });
        }

        void addToIncomingPacketQueue()
        {
            std::cout << "addToIncomingPacketQueue: new packet for: " << (m_ConnectionOwnerType == ConnectionOwner::Server ? "SERVER" : "CLIENT") << " Size: " << m_packetsTempIn.size() << "\n";
            if (m_ConnectionOwnerType == ConnectionOwner::Server)
                m_queuePacketsIn.push_back({ this->shared_from_this(), m_packetsTempIn });
            else
                m_queuePacketsIn.push_back({ nullptr, m_packetsTempIn });

            readHeaderFromPacket();
        }

    protected:
        // asio stuff here since it belongs to the connection

        // each connection has a unique socket to a remote
        asio::ip::tcp::socket m_socket;

        // a server can have multiple connections but we want to run the connections in one context
        // this context is shared with the whole asio instance provided for the client/server interface
        asio::io_context& m_asioContext;

        // this queue holds all packets to be sent to the server of this connection
        // it is threadsafe btw.
        ThreadsafeQueue<Packet<T>> m_queuePacketsOut;

        // this queue holds all packets for the server/client
        // It is a referende as the owner of this connection is expected to provide a queue
        ThreadsafeQueue<OwnedPacket<T>>& m_queuePacketsIn;

        //temp read buffer
        Packet<T> m_packetsTempIn;

        // ownership decides how some of the connection behaves
        ConnectionOwner m_ConnectionOwnerType = ConnectionOwner::Server;

        // Id for client
        uint32_t id = 0;

        // Is client validated?
        bool isAuthorized = false;

    };
}
