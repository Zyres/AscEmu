/*
Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

#pragma once

#include <AENetwork/AENetworkCommon.hpp>

namespace AENetwork
{
    // packet header (packet type/id)
    template <typename T>
    struct PacketHeader
    {
        T id{};
        uint32_t size = 0;
    };

    template <typename T>
    struct Packet
    {
        PacketHeader<T> header{};
        std::vector<uint8_t> body;  //bytes

        // return size of the packet in bytes
        size_t size() const
        {
            return body.size();
        }

        //pushing data into packet buffer
        template<typename DataType>
        friend Packet<T>& operator << (Packet<T>& packet, const DataType& data)
        {
            // check if the type of data is trivial copyable
            static_assert(std::is_standard_layout<DataType>::value, "Data can not be pushed into a vector!");

            // cache current size of vector, this is the point where we insert data
            size_t i = packet.body.size();

            // resize the vector by the size of pushed data
            packet.body.resize(packet.body.size() + sizeof(DataType));

            // vector is now ready to hold the data
            // copy the data into the allocated space
            std::memcpy(packet.body.data() + i, &data, sizeof(DataType));

            // update size of our header
            packet.header.size = static_cast<uint32_t>(packet.size());

            // return the target message to chain more data to it
            return packet;
        }

        // pulling data from packet buffer
        template<typename DataType>
        friend Packet<T>& operator >> (Packet<T>& packet, DataType& data)
        {
            // check if the type of data is trivial copyable
            static_assert(std::is_standard_layout<DataType>::value, "Data can not be pushed into a vector!");

            // cache location towards the end of the vector where the pulled data starts
            size_t i = packet.body.size() - sizeof(DataType);

            // copy the data from the vector into user variable
            std::memcpy(&data, packet.body.data() + i, sizeof(DataType));

            //resize the vector, we have the data (read bytes) and reset end position
            packet.body.resize(i);

            // update size of our header
            packet.header.size = static_cast<uint32_t>(packet.size());

            // return the target packet to chain more data to it
            return packet;
        }

    };

    //forward declare the connection
    template <typename T>
    class Connection;

    //used to determine who is the owner of the packet (client/server)
    template <typename T>
    struct OwnedPacket
    {
        // pointer to the specific connection. It is necessary to know who send the packet and from where so we can answer the right client/server.
        std::shared_ptr<Connection<T>> remote = nullptr;

        //encapsulate packet
        Packet<T> packet;
    };
}
