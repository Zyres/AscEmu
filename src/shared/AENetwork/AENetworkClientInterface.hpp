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
    //responsible to set up asio and the connection, accesspoint to talk to the server
    template <typename T>
    class ClientInterface
    {
    public:
        ClientInterface() : m_socket(m_context)
        {

        }

        virtual ~ClientInterface()
        {
            disconnect();
        }

    public:
        //connect to the server with hostname/ip-address and port
        bool connectToServer(const std::string& host, const uint16_t port)
        {
            try
            {
                //resolve domains
                asio::ip::tcp::resolver resolver(m_context);
                asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

                //create connection
                m_connection = std::make_unique<Connection<T>>(
                    Connection<T>::ConnectionOwner::Client,
                    m_context,
                    asio::ip::tcp::socket(m_context), m_queuePacketsIn);

                //connect to server
                m_connection->connectToServer(endpoints);


                // start context thread
                m_contextThread = std::thread([this]() { m_context.run(); });

            }
            catch (std::exception& exception)
            {
                std::cerr << "Client Exception: " << exception.what() << "\n";
                return false;
            }
            return false;
        }

        void disconnect()
        {
            // if we have a valid connection, disconnect
            if (isConnected())
            {
                m_connection->disconnect();
            }

            //stop the asio context
            m_context.stop();

            // if the thread is running, stop it
            if (m_contextThread.joinable())
                m_contextThread.join();

            // release the connection object
            m_connection.release();
        }

        bool isConnected()
        {
            if (m_connection)
                return m_connection->isConnected();
            else
                return false;
        }

        // send packet to server
        void sendPacket(const Packet<T>& packet)
        {
            if (isConnected())
                m_connection->sendPacket(packet);
        }

        ThreadsafeQueue<OwnedPacket<T>>& incomingQueue()
        {
            return m_queuePacketsIn;
        }

    protected:
        // the client interface ownes the asio context to handle the data transfer
        asio::io_context m_context;

        // the thread for the context
        std::thread m_contextThread;

        // this is the hardware socket that is connected to the server
        asio::ip::tcp::socket m_socket;

        //the client has a single instance of a "connection" object, which handles data transfer
        std::unique_ptr<Connection<T>> m_connection;

    private:
        // physical queue for incomming packets from the server
        ThreadsafeQueue<OwnedPacket<T>> m_queuePacketsIn;
    };
}
