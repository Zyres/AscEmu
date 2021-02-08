/*
Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

#include "LogonStdAfx.h"

//#define NEW_LOGONSERVER 1


#ifdef NEW_LOGONSERVER


int main(int argc, char** argv)
{
    /* test client function
    AENetwork::LogonCommClient c;
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
    AENetwork::LogonCommServer server(8093);
    server.Start();

    while (1)
    {
        server.Update(-1, true);
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

