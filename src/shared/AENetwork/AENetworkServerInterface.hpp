/*
Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

#pragma once

#include <AENetwork/AENetworkCommon.hpp>
#include <AENetwork/AENetworkPacket.hpp>
#include <AENetwork/AENetworkThreadsafeQueue.hpp>

namespace AENetwork
{
    template <typename T>
    class ServerInterface
    {
    public:
        ServerInterface(uint16_t port)
            // listen socket of the server
            : m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
        {

        }

        virtual ~ServerInterface()
        {
            stopServer();
        }

        bool startServer()
        {
            try
            {
                // set some work for the asio context
                waitForClients();

                // set up the thread
                m_contextThread = std::thread([this]() {m_asioContext.run(); });
            }
            catch (std::exception& exception)
            {
                std::cerr << "[SERVER] Exception: " << exception.what() << "\n";
                return false;
            }

            std::cout << "[SERVER] Started!\n";
            return true;
        }

        void stopServer()
        {

            // stop the context
            m_asioContext.stop();

            // stop the thread
            if (m_contextThread.joinable())
                m_contextThread.join();

            std::cout << "[SERVER] Stopped!\n";
        }

        //async instruct asio to wait for connections
        void waitForClients()
        {
            //issue a task to the asio contect to sit and wait for connections
            m_asioAcceptor.async_accept(
                [this](std::error_code errorCode, asio::ip::tcp::socket socket)
                {
                    if (!errorCode)
                    {
                        //connection was successfull, print out ip address
                        std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << "\n";

                        //create a new connection object
                        std::shared_ptr<Connection<T>> newConnection = std::make_shared<Connection<T>>(Connection<T>::ConnectionOwner::Server,
                            m_asioContext, std::move(socket), m_queuePacketsIn);

                        //user can deny the connection. Must be provided by an override, otherwise it will be false!
                        if (onClientConnect(newConnection))
                        {
                            // if connection is allowed, add the connection to the container
                            m_dequeActiveConnections.push_back(std::move(newConnection));

                            // add the id to the connection so we can identify it
                            m_dequeActiveConnections.back()->connectToClient(nIDCounter++);

                            //display on console the id of the connection
                            std::cout << "[" << m_dequeActiveConnections.back()->getId() << "] Connection Approved\n";

                        }
                        else
                        {
                            std::cout << "[----] Connection Denied\n";
                        }
                    }
                    else
                    {
                        std::cout << "[SERVER] New Connection Error: " << errorCode.message() << "\n";
                    }

                    // whatever happen reprime the asio context with more work to wait for another connetion
                    waitForClients();
                });
        }

        // send packet to a specific client
        void sendPacketToClient(std::shared_ptr<Connection<T>> client, const Packet<T>& packet)
        {
            // make sure the shared_ptr to the client is "valid" we do not know if we can communicate with the client, even with this check!
            if (client && client->IsConnected())
            {
                client->sendPacket(packet);
            }
            else
            {
                // a tcp connection has its downfall, we do not know if a client has disconnected
                // we assume the client is disconnected
                onClientDisconnect(client);
                client.reset(std::remove(m_dequeActiveConnections.begin(), m_dequeActiveConnections.end(), client), m_dequeActiveConnections.end());
            }
        }

        void sendPacketToAllClients(const Packet<T>& packet, std::shared_ptr<Connection<T>> ignoreClient = nullptr)
        {
            bool bInvalidClientExists = false;

            for (auto& client : m_dequeActiveConnections)
            {
                //check client is connected
                if (client && client->IsConnected())
                {
                    //it is
                    if (client != ignoreClient)
                        client->sendPacket(packet);
                }
                else
                {
                    onClientDisconnect(client);
                    client.reset();
                    bInvalidClientExists = true;
                }
            }

            // remove invalif connection after the loop, otherwise it will break!
            if (bInvalidClientExists)
                m_dequeActiveConnections.erase(std::remove(m_dequeActiveConnections.begin(), m_dequeActiveConnections.end(), nullptr), m_dequeActiveConnections.end());
        }

        // called to explicity process some of the packet in that queue
        // controles when it is the best time to handle packets
        void update(size_t maxMessages = -1, bool wait = false)
        {
            // we do not need the server to occuoy 100% CPU of a core
            if (wait)
                m_queuePacketsIn.wait();

            std::cout << "[SERVER] PacketQueue Update called!\n";

            size_t nMessageCount = 0;
            while (nMessageCount < maxMessages && !m_queuePacketsIn.empty())
            {

                //grab the front packet
                auto packet = m_queuePacketsIn.pop_front();

                // pass it to the packet handler (owned, shared_ptr inside)
                onMessage(packet.remote, packet.packet);

                nMessageCount++;
            }
        }
    protected:
        //called when client connects, we can refuse the connection
        virtual bool onClientConnect(std::shared_ptr<Connection<T>> client)
        {
            // banned ip...
            return false;
        }

        // called when a client appears to have disconnected
        virtual void onClientDisconnect(std::shared_ptr<Connection<T>> client)
        {

        }

        //called when a packet arrives
        virtual void onMessage(std::shared_ptr<Connection<T>> client, Packet<T>& packet)
        {
            std::cout << "[SERVER] OnMessage (ServerInterface) called!\n";
        }

    protected:
        // server has its own threadsafe queue for incoming packets
        ThreadsafeQueue<OwnedPacket<T>> m_queuePacketsIn;

        // container of active validated connections
        std::deque<std::shared_ptr<Connection<T>>> m_dequeActiveConnections;

        //order of declaration is important - it is also the order of initialisation
        //the context is shared over all clients, means this context handles all client connections
        asio::io_context m_asioContext;
        std::thread m_contextThread;

        //the server socket is hidden behind asio classes. We need a acceptor to hold all sockets of the clients
        asio::ip::tcp::acceptor m_asioAcceptor;

        //clients will be identified by an id. It has to be unique for every connection! Consistence ID across the entire system, the clients know their own id.
        uint32_t nIDCounter = 1;

    };
}
