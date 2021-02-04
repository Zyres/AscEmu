/*
Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

#include "LogonStdAfx.h"

#define NEW_LOGONSERVER 1


#ifdef NEW_LOGONSERVER

#include <iostream>

#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

////////////////////////////////////////////////////////////////////////////////////////////////
// Message definitions

namespace AENetwork
{
    // message header (message type/id)
    template <typename T>
    struct message_header
    {
        T id{};
        uint32_t size = 0;
    };

    template <typename T>
    struct message
    {
        message_header<T> header{};
        std::vector<uint8_t> body;  //bytes

        // return size of the message in bytes
        size_t size() const
        {
            return sizeof(message_header<T>) + body.size();
        }

        //override for std::cout compatibility
        friend std::ostream& operator << (std::ostream& os, const message<T>& msg)
        {
            os << "ID:" << int(msg.header.id) << " Size:" << msg.header.size;
            return os;
        }

        //pushing data into message buffer (write buffer)
        template<typename DataType>
        friend message<T>& operator << (message<T>& msg, const DataType& data)
        {
            // check if the type of data is trivial copyable
            static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

            // cache current size of vector, this is the point where we insert data
            size_t i = msg.body.size();

            // resize the vector by the size of pushed data
            msg.body.resize(msg.body.size() + sizeof(DataType));

            // vector is now ready to hold the data
            // copy the data into the allocated space
            std::memcpy(msg.body.data() + i, &data, sizeof(DataType));

            // update size of our header
            msg.header.size = msg.size();

            // return the target message to chain more data to it
            return msg;
        }

        template<typename DataType>
        friend message<T>& operator >> (message<T>& msg, DataType& data)
        {
            // check if the type of data is trivial copyable
            static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

            // cache location towards the end of the vector where the pulled data starts
            size_t i = msg.body.size() - sizeof(DataType);

            // copy the data from the vector into user variable
            std::memcpy(&data, msg.body.data() + i, sizeof(DataType));

            //resize the vector, we have the data (read bytes) and reset end position
            msg.body.resize(i);

            // update size of our header
            msg.header.size = msg.size();

            // return the target message to chain more data to it
            return msg;
        }

    };

    //forward declare the connection
    template <typename T>
    class connection;

    //used to determine who is the owner of the message (client/server)
    template <typename T>
    struct owned_message
    {
        // pointer to the specific connection. It is necessary to know who send the message and from where so we can answer the right client/server.
        std::shared_ptr<connection<T>> remote = nullptr;

        //encapsulate message
        message<T> msg;

        //friendly strin maker
        friend std::ostream& operator << (std::ostream& os, const owned_message<T>& msg)
        {
            os << msg.msg;
            return os;
        }
    };
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Message definitions ENDS

////////////////////////////////////////////////////////////////////////////////////////////////
// Network Threadsafe Queue

namespace AENetwork
{
    // threadsafe queue for messages
    template<typename T>
    class tsqueue
    {
    public:
        tsqueue() = default;
        tsqueue(const tsqueue<T>&) = delete;
        virtual ~tsqueue() { clear(); }

    public:
        // returns and maintains item at the front of the Queue
        // reference of the object of the front of the queue
        const T& front()
        {
            // protect until the line below is running. lock will "unlock" after exiting the function.
            std::scoped_lock lock(muxQueue);
            return deqQueue.front();
        }

        // returns and maintains item at the back of the Queue
        // reference of the object of the back of the queue
        const T& back()
        {
            // protect until the line below is running. lock will "unlock" after exiting the function.
            std::scoped_lock lock(muxQueue);
            return deqQueue.back();
        }

        // add objects at the end of the queue
        void push_back(const T& item)
        {
            std::scoped_lock lock(muxQueue);
            deqQueue.emplace_back(std::move(item));

            std::unique_lock<std::mutex> ul(muxBlocking);
            cvBlocking.notify_one();
        }

        // add objects at the front of the queue
        void push_front(const T& item)
        {
            std::scoped_lock lock(muxQueue);
            deqQueue.emplace_front(std::move(item));

            std::unique_lock<std::mutex> ul(muxBlocking);
            cvBlocking.notify_one();
        }

        // returns true if the queue has no objects
        bool empty()
        {
            std::scoped_lock lock(muxQueue);
            return deqQueue.empty();
        }

        // returns number of items in the Queue
        size_t count()
        {
            std::scoped_lock lock(muxQueue);
            return deqQueue.size();
        }

        // clears queue (erases all objects from the Queue)
        void clear()
        {
            std::scoped_lock lock(muxQueue);
            deqQueue.clear();
        }

        //removes and returns object from the front of the queue
        T pop_front()
        {
            std::scoped_lock lock(muxQueue);
            auto t = std::move(deqQueue.front());
            deqQueue.pop_front();
            return t;
        }

        //removes and returns object from the back of the queue
        T pop_back()
        {
            std::scoped_lock lock(muxQueue);
            auto t = std::move(deqQueue.back());
            deqQueue.pop_back();
            return t;
        }

        void wait()
        {
            while (empty())
            {
                std::unique_lock<std::mutex> ul(muxBlocking);
                cvBlocking.wait(ul);
            }
        }

    protected:
        std::mutex muxQueue;
        std::deque<T> deqQueue;

        // used for waiting until data arrives
        std::condition_variable cvBlocking;
        std::mutex muxBlocking;
    };
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Network Threadsafe Queue ENDS

////////////////////////////////////////////////////////////////////////////////////////////////
// Network Connection

namespace AENetwork
{
    //allowing creation of a shared pointer within this object.
    template <typename T>
    class connection : public std::enable_shared_from_this<connection<T>>
    {
    public:

        enum class owner
        {
            server,
            client
        };

        connection(owner parent, asio::io_context& asioContext, asio::ip::tcp::socket socket, tsqueue<owned_message<T>>& qIn)
            : m_asioContext(asioContext), m_socket(std::move(socket)), m_qMessagesIn(qIn)
        {
            m_nOwnerType = parent;
        }

        virtual ~connection()
        {}

        uint32_t GetID() const
        {
            return id;
        }

    public:

        void ConnectToClient(uint32_t uid = 0)
        {
            // only for server
            if (m_nOwnerType == owner::server)
            {
                if (m_socket.is_open())
                {
                    id = uid;
                    ReadHeader();
                }
            }
        }

        // can be called only for clients
        void ConnectToServer(const asio::ip::tcp::resolver::results_type& endponts)
        {
            // only for clients
            if (m_nOwnerType == owner::clients)
            {
                // give asio something to do
                asio::async_connect(m_socket, endpoints,
                    [this](std::error_code ec, asio::ip::tcp::endpoint endpoints)
                    {
                        if (!ec)
                        {
                            ReadHeader();
                        }
                    });
            }
        }

        //can be called by server and clients
        void Disconnect()
        {
            if (IsConnected())
                asio::post(m_asioContext, [this]() { m_socket.close(); });
        }

        //can be called by server and clients
        bool IsConnected() const
        {
            return m_socket.is_open();
        }

    public:
        // send messages with the connection
        void Send(const message<T>& msg)
        {
            asio::post(m_asioContext,
                [this, msg]()
                {
                    bool bWritinMessage = !m_qMessagesOut.empty();
                    m_qMessagesOut.push_back(msg);
                    if (!bWritinMessage)
                    {
                        WriteHeader();
                    }
                });
        }

    private:
        //async prime context ready to read a message header
        void ReadHeader()
        {
            asio::async_read(m_socket, asio::buffer(&n_msgTemporaryIn.header, sizeof(message_header<T>)),
                [this](std::error_code ec, std::size_t length)
                {
                    if (!ec)
                    {
                        if (n_msgTemporaryIn.header.size > 0)
                        {
                            std::cout << "[" << id << "] "<< "Received message header with headerId: " << uint32_t(n_msgTemporaryIn.header.id) << " length: " << n_msgTemporaryIn.header.size << "\n";

                            // make enough space to store the body
                            n_msgTemporaryIn.body.resize(n_msgTemporaryIn.header.size);
                            ReadBody();
                        }
                        else
                        {
                            // if we have no body data
                            AddToIncomingMessageQueue();
                        }
                    }
                    else
                    {
                        std::cout << "[" << id << "] Read Header Fail.\n";
                        m_socket.close();
                    }
                });
        }

        //async prime context ready to read a message body
        void ReadBody()
        {
            asio::async_read(m_socket, asio::buffer(n_msgTemporaryIn.body.data(), n_msgTemporaryIn.body.size()),
                [this](std::error_code ec, std::size_t length)
                {
                    if (!ec)
                    {
                        std::cout << "[" << id << "] " << "Received message body with length: " << n_msgTemporaryIn.body.size() << "\n";

                        AddToIncomingMessageQueue();
                    }
                    else
                    {
                        std::cout << "[" << id << "] Read Body Fail.\n";
                        m_socket.close();
                    }
                });
        }

        //async prime context ready to write a message header
        void WriteHeader()
        {
            asio::async_write(m_socket, asio::buffer(&m_qMessagesOut.front(), sizeof(message_header<T>)),
                [this](std::error_code ec, std::size_t length)
                {
                    if (!ec)
                    {
                        if (m_qMessagesOut.front().body.size() > 0)
                        {
                            WriteBody();
                        }
                        else
                        {
                            m_qMessagesOut.pop_front();

                            if (!m_qMessagesOut.empty())
                            {
                                WriteHeader();
                            }
                        }
                    }
                    else
                    {
                        std::cout << "[" << id << "] Write Header Fail.\n";
                        m_socket.close();
                    }
                });
        }

        //async prime context ready to write a message body
        void WriteBody()
        {
            asio::async_write(m_socket, asio::buffer(m_qMessagesOut.front().body.data(), m_qMessagesOut.front().body.size()),
                [this](std::error_code ec, std::size_t length)
                {
                    if (!ec)
                    {

                        m_qMessagesOut.pop_front();

                        if (!m_qMessagesOut.empty())
                        {
                            WriteHeader();
                        }
                    }
                    else
                    {
                        std::cout << "[" << id << "] Write Body Fail.\n";
                        m_socket.close();
                    }
                });
        }

        void AddToIncomingMessageQueue()
        {
            if (m_nOwnerType == owner::server)
                m_qMessagesIn.push_back({ this->shared_from_this(), n_msgTemporaryIn });
            else
                m_qMessagesIn.push_back({ nullptr, n_msgTemporaryIn });

            ReadHeader();
        }

    protected:
        // asio stuff here since it belongs to the connection

        // each connection has a unique socket to a remote
        asio::ip::tcp::socket m_socket;

        // a server can have multiple connections but we want to run the connections in one context
        // this context is shared with the whole asio instance provided for the client/server interface
        asio::io_context& m_asioContext;

        // this queue holds all messages to be sent to the server of this connection
        // it is threadsafe btw.
        tsqueue<message<T>> m_qMessagesOut;

        // this queue holds all messages for the server/client
        // It is a referende as the owner of this connection is expected to provide a queue
        tsqueue<owned_message<T>>& m_qMessagesIn;

        //temp read buffer
        message<T> n_msgTemporaryIn;

        // ownership decides how some of the connection behaves
        owner m_nOwnerType = owner::server;
        uint32_t id = 0;

    };
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Network Connection ENDS

////////////////////////////////////////////////////////////////////////////////////////////////
// Network Server

namespace AENetwork
{
    template <typename T>
    class server_interface
    {
    public:
        server_interface(uint16_t port)
            // listen socket of the server
            : m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
        {

        }

        virtual ~server_interface()
        {
            Stop();
        }

        bool Start()
        {
            try
            {
                // set some work for the asio context
                WaitForClientConnection();

                // set up the thread
                m_threadContext = std::thread([this]() {m_asioContext.run(); });
            }
            catch (std::exception& e)
            {
                std::cerr << "[SERVER] Exception: " << e.what() << "\n";
                return false;
            }

            std::cout << "[SERVER] Started!\n";
            return true;
        }

        void Stop()
        {

            // stop the context
            m_asioContext.stop();

            // stop the thread
            if (m_threadContext.joinable())
                m_threadContext.join();

            std::cout << "[SERVER] Stopped!\n";
        }

        //async instruct asio to wait for connections
        void WaitForClientConnection()
        {
            //issue a task to the asio contect to sit and wait for connections
            m_asioAcceptor.async_accept(
                [this](std::error_code ec, asio::ip::tcp::socket socket)
                {
                    if (!ec)
                    {
                        //connection was successfull, print out ip address
                        std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << "\n";

                        //create a new connection object
                        std::shared_ptr<connection<T>> newconn = std::make_shared<connection<T>>(connection<T>::owner::server,
                            m_asioContext, std::move(socket), m_qMessagesIn);

                        //user can deny the connection. Must be provided by an override, otherwise it will be false!
                        if (OnClientConnect(newconn))
                        {
                            // if connection is allowed, add the connection to the container
                            m_deqConnections.push_back(std::move(newconn));

                            // add the id to the connection so we can identify it
                            m_deqConnections.back()->ConnectToClient(nIDCounter++);

                            //display on console the id of the connection
                            std::cout << "[" << m_deqConnections.back()->GetID() << "] Connection Approved\n";

                        }
                        else
                        {
                            std::cout << "[----] Connection Denied\n";
                        }
                    }
                    else
                    {
                        std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
                    }

                    // whatever happen reprime the asio context with more work to wait for another connetion
                    WaitForClientConnection();
                });
        }

        // send message to a specific client
        void MessageClient(std::shared_ptr<connection<T>> client, const message<T>& msg)
        {
            // make sure the shared_ptr to the client is "valid" we do not know if we can communicate with the client, even with this check!
            if (client && client->IsConnected())
            {
                client->Send(msg);
            }
            else
            {
                // a tcp connection has its downfall, we do not know if a client has disconnected
                // we assume the client is disconnected
                OnClientDisconnect(client);
                client.reset(
                    std::remove(m_deqConnections.begin(), m_deqConnections.end(), client), m_deqConnections.end());
            }
        }

        void MessageAllClients(const message<T>& msg, std::shared_ptr<connection<T>> pIgnoreClient = nullptr)
        {
            bool bInvalidClientExists = false;

            for (auto& client : m_deqConnections)
            {
                //check client is connected
                if (client && client->IsConnected())
                {
                    //it is
                    if (client != pIgnoreClient)
                        client->Send(msg);
                }
                else
                {
                    OnClientDisconnect(client);
                    client.reset();
                    bInvalidClientExists = true;
                }
            }

            // remove invalif connection after the loop, otherwise it will break!
            if (bInvalidClientExists)
                m_deqConnections.erase(std::remove(m_deqConnections.begin(), m_deqConnections.end(), nullptr), m_deqConnections.end());
        }

        // called to explicity process some of the message in that queue
        // controles when it is the best time to handle messages
        void Update(size_t nMaxMessages = -1, bool bWait = false)
        {
            // we do not need the server to occuoy 100% CPU of a core
            if (bWait)
                m_qMessagesIn.wait();

            size_t nMessageCount = 0;
            while (nMessageCount < nMaxMessages && !m_qMessagesIn.empty())
            {
                //grab the front message
                auto msg = m_qMessagesIn.pop_front();

                // pass it to the message handler (owned, shared_ptr inside)
                OnMessage(msg.remote, msg.msg);

                nMessageCount++;
            }
        }
        
    protected:
        //called when client connects, we can refuse the connection
        virtual bool OnClientConnect(std::shared_ptr<connection<T>> client)
        {
            // banned ip...
            return false;
        }

        // called when a client appears to have disconnected
        virtual void OnClientDisconnect(std::shared_ptr<connection<T>> client)
        {
            
        }

        //called when a message arrives
        virtual void OnMessage(std::shared_ptr<connection<T>> client, message<T>& msg)
        {
            
        }

    protected:
        // server has its own threadsafe queue for incoming messages
        tsqueue<owned_message<T>> m_qMessagesIn;

        // container of active validated connections
        std::deque<std::shared_ptr<connection<T>>> m_deqConnections;

        //order of declaration is important - it is also the order of initialisation
        //the context is shared over all clients, means this context handles all client connections
        asio::io_context m_asioContext;
        std::thread m_threadContext;

        //the server socket is hidden behind asio classes. We need a acceptor to hold all sockets of the clients
        asio::ip::tcp::acceptor m_asioAcceptor;

        //clients will be identified by an id. It has to be unique for every connection! Consistence ID across the entire system, the clients know their own id.
        uint32_t nIDCounter = 10000;

    };
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Network Server ENDS

////////////////////////////////////////////////////////////////////////////////////////////////
// Network Client

namespace AENetwork
{
    //responsible to set up asio and the connection, accesspoint to talk to the server
    template <typename T>
    class client_interface
    {
    public:
        client_interface() : m_socket(m_context)
        {

        }

        virtual ~client_interface()
        {
            Disconnect();
        }

    public:
        //connect to the server with hostname/ip-address and port
        bool Connect(const std::string& host, const uint16_t port)
        {
            try
            {
                //resolve domains
                asio::ip::tcp::resolver resolver(m_context);
                asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host; std::to_string(port));

                //create connection
                m_connection = std::make_unique<connection<T>>(
                    connection<T>::owner::client,
                    m_context,
                    asio::ip::tcp::socket(m_context), m_qMessagesIn);

                //connect to server
                m_connection->ConnectToServer(endpoints);


                // start context thread
                thrContext = std::thread([this]() {m_context.run(); });

            }
            catch
            {
                std::cerr << "Client Exception: " << e.what() << "\n";
                return false;
            }
            return false;
        }

        void Disconnect()
        {
            // if we have a valid connection, disconnect
            if (IsConnected())
            {
                m_connection->Disconnect();
            }

            //stop the asio context
            m_context.stop();

            // if the thread is running, stop it
            if (thrContext.joinable())
                thrContext.join();

            // release the connection object
            m_connection.release();
        }

        bool IsConnected()
        {
            if (m_connection)
                return m_connection->IsConnected();
            else
                return false;
        }

        // send message to server
        void Send(const message<T>& msg)
        {
            if (IsConnected())
                m_connection->Send(msg);
        }

        tsqueue<owned_message<T>>& Incoming()
        {
            return m_qMessagesIn;
        }

    protected:
        // the client interface ownes the asio context to handle the data transfer
        asio::io_context m_context;

        // the thread for the context
        std::thread thrContext;

        // this is the hardware socket that is connected to the server
        asio::ip::tcp::socket m_socket;

        //the client has a single instance of a "connection" object, which handles data transfer
        std::unique_ptr<connection<T>> m_connection;

    private:
        // physical queue for incomming messages from the server
        tsqueue<owned_message<T>> m_qMessagesIn;
    };
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Network Client ENDS

////////////////////////////////////////////////////////////////////////////////////////////////
// Network

namespace AENetwork
{

}

////////////////////////////////////////////////////////////////////////////////////////////////
// Network  ENDS

////////////////////////////////////////////////////////////////////////////////////////////////
// Network

namespace AENetwork
{

}

////////////////////////////////////////////////////////////////////////////////////////////////
// Network  ENDS

////////////////////////////////////////////////////////////////////////////////////////////////
// Message types

enum class LogonCommTypes : uint16_t
{
    LRCMSG_REALM_REGISTER_REQUEST = 0x001,  // try register our realm
    LRSMSG_REALM_REGISTER_RESULT = 0x002,  // register result from logonserver
    LRCMSG_ACC_SESSION_REQUEST = 0x003,
    LRSMSG_ACC_SESSION_RESULT = 0x004,
    LRCMSG_LOGON_PING_STATUS = 0x005,  // request logon online
    LRSMSG_LOGON_PING_RESULT = 0x006,  // send result if logon is online
    LRCMSG_FREE_01 = 0x007,  // unused
    LRSMSG_FREE_02 = 0x008,  // unused
    LRCMSG_AUTH_REQUEST = 0x009,  // try authenticate our realm
    LRSMSG_AUTH_RESPONSE = 0x00A,  // authentication result from logonserver
    LRSMSG_ACC_CHAR_MAPPING_REQUEST = 0x00B,
    LRCMSG_ACC_CHAR_MAPPING_RESULT = 0x00C,
    LRCMSG_ACC_CHAR_MAPPING_UPDATE = 0x00D,
    LRSMSG_SEND_ACCOUNT_DISCONNECT = 0x00E,  // send when account is disconnected
    LRCMSG_LOGIN_CONSOLE_REQUEST = 0x00F,
    LRSMSG_LOGIN_CONSOLE_RESULT = 0x010,
    LRCMSG_ACCOUNT_DB_MODIFY_REQUEST = 0x011,  // request logon db change
    LRSMSG_ACCOUNT_DB_MODIFY_RESULT = 0x012,
    LRSMSG_REALM_POPULATION_REQUEST = 0x013,
    LRCMSG_REALM_POPULATION_RESULT = 0x014,
    LRCMSG_ACCOUNT_REQUEST = 0x015,  // request account data
    LRSMSG_ACCOUNT_RESULT = 0x016,  // send account information to realm
    LRCMSG_ALL_ACCOUNT_REQUEST = 0x017,  // request all account data
    LRSMSG_ALL_ACCOUNT_RESULT = 0x018,  // send id, name, rank of all accounts to realm

    LRMSG_MAX_OPCODES                           // max opcodes
};

////////////////////////////////////////////////////////////////////////////////////////////////
// Message types ENDS

////////////////////////////////////////////////////////////////////////////////////////////////
// Client implementation

class CustomClient : public AENetwork::client_interface<LogonCommTypes>
{
public:
    void SendAuth()
    {
        AENetwork::message<LogonCommTypes> msg;
        msg.header.id = LogonCommTypes::LRCMSG_AUTH_REQUEST;

        //fill msg here
        //msg << data;
        Send(msg);
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////
// Client implementation ENDS

////////////////////////////////////////////////////////////////////////////////////////////////
// Server implementation

class CustomServer : public AENetwork::server_interface<LogonCommTypes>
{
public:
    CustomServer(uint16_t nPort) : AENetwork::server_interface<LogonCommTypes>(nPort)
    {

    }

protected:
    //called when client connects, we can refuse the connection
    virtual bool OnClientConnect(std::shared_ptr<AENetwork::connection<LogonCommTypes>> client)
    {
        // banned ip...
        return true;
    }

    // called when a client appears to have disconnected
    virtual void OnClientDisconnect(std::shared_ptr<AENetwork::connection<LogonCommTypes>> client)
    {
        std::cout << "Removing client [" << client->GetID() << "]\n";
    }

    //called when a message arrives
    virtual void OnMessage(std::shared_ptr<AENetwork::connection<LogonCommTypes>> client, AENetwork::message<LogonCommTypes>& msg)
    {
        switch (msg.header.id)
        {
            case LogonCommTypes::LRCMSG_REALM_REGISTER_REQUEST:
            {
                std::cout << "[" << client->GetID() << "]: Auth Register Request\n";
            } break;
            case LogonCommTypes::LRCMSG_AUTH_REQUEST:
            {
                std::cout << "[" << client->GetID() << "]: Auth Request\n";
                // do stuff

                //read packet <<

                // ckeck security

                // create new packet
                
                //send back the answer
                client->Send(msg);
            } break;
            default:
                client->Disconnect();
                break;
        }
    }

};

////////////////////////////////////////////////////////////////////////////////////////////////
// Server implementation ENDS

//asynch implementation
std::vector<char> vBuffer(20 * 1024);

void readSomeData(asio::ip::tcp::socket& socket)
{
    // lamda function, will read the asio buffer until there is no data left.
    socket.async_read_some(asio::buffer(vBuffer.data(), vBuffer.size()),
        [&](std::error_code ec, std::size_t length)
        {
            if (!ec)
            {
                // print out data
                std::cout << "\n\nRead " << length << " bytes\n\n";
                for (int i = 0; i < length; i++)
                    std::cout << vBuffer[i];

                // read left data in our buffer
                readSomeData(socket);
            }
        }
    );
};

int main(int argc, char** argv)
{
    /* test client function
    CustomClient c;
    c.Connect("127.0.0.1", 80);

    bool bQuit = false;
    while (!bQuit)
    {
        if (c.IsConnected())
        {
            if (!c.Incoming().empty())
            {
                auto msg = c.Incoming().pop_front().msg;

                switch (msg.header.id)
                {
                    case LogonCommTypes::LRSMSG_AUTH_RESPONSE;
                    {
                        uint32_t result;
                        //do stuff with the message
                        msg >> result;

                    } break;
                }
            }
        }
        else
        {
            std::out << "Server Down\n";
            bQuit = true;
        }
    }

    */

    // testserver
    CustomServer server(8093);
    server.Start();

    while (1)
    {
        server.Update(-1, true);
    }

    //asio::error_code ec;

    ////context for asio to run
    //asio::io_context context;

    ////give context some fake tasks so it does not finish
    ////if asio has nothing to do it will exist immediately
    ////so give it something to do :-)
    //asio::io_context::work idleWork(context);

    ////start the asio context in its own thread
    ////so it does not block main execution
    //std::thread thrContext = std::thread([&]() { context.run(); });

    ////create the endpoint where we will connect
    //asio::ip::tcp::endpoint endpoint(asio::ip::make_address("93.184.216.34", ec), 80);

    ////create socket for our context, so we can shoot a request to our endpoint
    //asio::ip::tcp::socket socket(context);

    ////connect to endpoint through our created socket.
    //socket.connect(endpoint, ec);
    //if (!ec)
    //{
    //    std::cout << "Connected" << std::endl;
    //}
    //else
    //{
    //    std::cout << "Failed to connect to address:\n" << ec.message() << std::endl;
    //}

    //// if we succesfull connect to our endpoint through our socket, do some stuff
    //if (socket.is_open())
    //{
    //    //prime the read function before we send the request. Otherwise the programm finish before receiving some!
    //    //read response until there is no data left on the socket.
    //    readSomeData(socket);

    //    // build our request, in this case a http request
    //    std::string sRequest =
    //        "GET /index.html HHTP/1.1\r\n"
    //        "Host: example.com\r\n"
    //        "Connection: close\r\n\r\n";

    //    //write our request to the socket (shoot our request through the socket to our endpoint)
    //    socket.write_some(asio::buffer(sRequest.data(), sRequest.size()), ec);

    //    //wait before programm is finished
    //    using namespace std::chrono_literals;
    //    std::this_thread::sleep_for(200ms);

    //    // we have received all of our data, stop the asio context (close the socket)
    //    context.stop();

    //    // end our thread
    //    if (thrContext.joinable())
    //        thrContext.join();
    //}



    ///// test message
    //AENetwork::message<LogonCommTypes> msg;
    //msg.header.id = LogonCommTypes::LRSMSG_AUTH_RESPONSE;

    //uint32_t result = 1;

    //msg << result;



    system("pause");
    return 0;
}





#else

int main(int argc, char** argv)
{
#ifndef WIN32
    rlimit rl;
    if (getrlimit(RLIMIT_CORE, &rl) == -1)
        printf("getrlimit failed. This could be problem.\n");
    else
    {
        rl.rlim_cur = rl.rlim_max;
        if (setrlimit(RLIMIT_CORE, &rl) == -1)
            printf("setrlimit failed. Server may not save core.dump files.\n");
    }
#endif

    sMasterLogon.Run(argc, argv);
}

#endif // NEW_LOGONSERVER

