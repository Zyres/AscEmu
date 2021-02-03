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
    asio::error_code ec;

    //context for asio to run
    asio::io_context context;

    //give context some fake tasks so it does not finish
    //if asio has nothing to do it will exist immediately
    //so give it something to do :-)
    asio::io_context::work idleWork(context);

    //start the asio context in its own thread
    //so it does not block main execution
    std::thread thrContext = std::thread([&]() { context.run(); });

    //create the endpoint where we will connect
    asio::ip::tcp::endpoint endpoint(asio::ip::make_address("93.184.216.34", ec), 80);

    //create socket for our context, so we can shoot a request to our endpoint
    asio::ip::tcp::socket socket(context);

    //connect to endpoint through our created socket.
    socket.connect(endpoint, ec);
    if (!ec)
    {
        std::cout << "Connected" << std::endl;
    }
    else
    {
        std::cout << "Failed to connect to address:\n" << ec.message() << std::endl;
    }

    // if we succesfull connect to our endpoint through our socket, do some stuff
    if (socket.is_open())
    {
        //prime the read function before we send the request. Otherwise the programm finish before receiving some!
        //read response until there is no data left on the socket.
        readSomeData(socket);

        // build our request, in this case a http request
        std::string sRequest =
            "GET /index.html HHTP/1.1\r\n"
            "Host: example.com\r\n"
            "Connection: close\r\n\r\n";

        //write our request to the socket (shoot our request through the socket to our endpoint)
        socket.write_some(asio::buffer(sRequest.data(), sRequest.size()), ec);

        //wait before programm is finished
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(200ms);

        // we have received all of our data, stop the asio context (close the socket)
        context.stop();

        // end our thread
        if (thrContext.joinable())
            thrContext.join();
    }

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

