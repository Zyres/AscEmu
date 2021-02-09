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

        //////////////////////////////////////////////////////////////////////////////////////////
        //pushing datatypes to vector

        // write special types to vector - user for chars
        friend void write(Packet<T>& packet, const uint8_t* dataSource, size_t dataSize)
        {
            if (!dataSize)
                return;

            if (!dataSource)
                return;

            size_t i = packet.body.size();

            packet.body.resize(packet.body.size() + dataSize);

            std::memcpy(packet.body.data() + i, dataSource, dataSize);

            packet.header.size = static_cast<uint32_t>(packet.size());
        }

        // template function to write basic dataTypes to vector
        template<typename DataType>
        friend void write(Packet<T>& packet, const DataType& data)
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
        }

        // handle basic types
        friend Packet<T>& operator << (Packet<T>& packet, const bool& data)
        {
            write<char>(packet, static_cast<char>(data));
            return packet;
        }

        friend Packet<T>& operator << (Packet<T>& packet, const float& data)
        {
            write<float>(packet, data);
            return packet;
        }

        friend Packet<T>& operator << (Packet<T>& packet, const double& data)
        {
            write<double>(packet, data);
            return packet;
        }

        friend Packet<T>& operator << (Packet<T>& packet, const uint8_t& data)
        {
            write<uint8_t>(packet, data);
            return packet;
        }

        friend Packet<T>& operator << (Packet<T>& packet, const uint16_t& data)
        {
            write<uint16_t>(packet, data);
            return packet;
        }

        friend Packet<T>& operator << (Packet<T>& packet, const uint32_t& data)
        {
            write<uint32_t>(packet, data);
            return packet;
        }

        friend Packet<T>& operator << (Packet<T>& packet, const uint64_t& data)
        {
            write<uint64_t>(packet, data);
            return packet;
        }

        friend Packet<T>& operator << (Packet<T>& packet, const int8_t& data)
        {
            write<int8_t>(packet, data);
            return packet;
        }

        friend Packet<T>& operator << (Packet<T>& packet, const int16_t& data)
        {
            write<int16_t>(packet, data);
            return packet;
        }

        friend Packet<T>& operator << (Packet<T>& packet, const int32_t& data)
        {
            write<int32_t>(packet, data);
            return packet;
        }

        friend Packet<T>& operator << (Packet<T>& packet, const int64_t& data)
        {
            write<int64_t>(packet, data);
            return packet;
        }

        // handle special types
        friend Packet<T>& operator << (Packet<T>& packet, const std::string& data)
        {
            write(packet, reinterpret_cast<const uint8_t*>(data.c_str()), data.length());

            //add 0 to the end of the vector string bytes so we can find the end of the string when we read it.
            write<uint8_t>(packet, static_cast<uint8_t>(0));
            return packet;
        }

        //////////////////////////////////////////////////////////////////////////////////////////
        //pulling data from he vector

        //template function to read basic dataTypes
        template <typename DataType>
        friend void read(Packet<T>& packet, DataType& data)
        {
            // check if the type of data is trivial copyable
            static_assert(std::is_standard_layout<DataType>::value, "Data can not be pulled from a vector!");

            // cache location towards the end of the vector where the pulled data starts
            const size_t i = packet.body.size() -sizeof(DataType);

            // copy the data from the vector into user variable
            std::memcpy(&data, packet.body.data(), sizeof(DataType));

            //move n elements to the end
            std::rotate(packet.body.begin(), packet.body.begin() + sizeof(DataType), packet.body.end());

            //resize the vector, we have the data and n elements gets removed at the end of the vector
            packet.body.resize(i);

            // update size of our header
            packet.header.size = static_cast<uint32_t>(packet.size());
        }

        // handle basic types
        friend Packet<T>& operator >> (Packet<T>& packet, bool& data)
        {
            char singleCharacter;
            read<char>(packet, singleCharacter);
            data = singleCharacter > 0 ? true : false;
            return packet;
        }

        friend Packet<T>& operator >> (Packet<T>& packet, float& data)
        {
            read<float>(packet, data);
            return packet;
        }

        friend Packet<T>& operator >> (Packet<T>& packet, double& data)
        {
            read<double>(packet, data);
            return packet;
        }

        friend Packet<T>& operator >> (Packet<T>& packet, uint8_t& data)
        {
            read<uint8_t>(packet, data);
            return packet;
        }

        friend Packet<T>& operator >> (Packet<T>& packet, uint16_t& data)
        {
            read<uint16_t>(packet, data);
            return packet;
        }

        friend Packet<T>& operator >> (Packet<T>& packet, uint32_t& data)
        {
            read<uint32_t>(packet, data);
            return packet;
        }

        friend Packet<T>& operator >> (Packet<T>& packet, uint64_t& data)
        {
            read<uint64_t>(packet, data);
            return packet;
        }

        friend Packet<T>& operator >> (Packet<T>& packet, int8_t& data)
        {
            read<int8_t>(packet, data);
            return packet;
        }

        friend Packet<T>& operator >> (Packet<T>& packet, int16_t& data)
        {
            read<int16_t>(packet, data);
            return packet;
        }

        friend Packet<T>& operator >> (Packet<T>& packet, int32_t& data)
        {
            read<int32_t>(packet, data);
            return packet;
        }

        friend Packet<T>& operator >> (Packet<T>& packet, int64_t& data)
        {
            read<int64_t>(packet, data);
            return packet;
        }

        // handle special types
        friend Packet<T>& operator >> (Packet<T>& packet, std::string& data)
        {
            // clear std::string, who knows what the user already did to it.
            data.clear();

            while (true)
            {
                char singleCharacter;
                read<char>(packet, singleCharacter);

                // we found our 0 byte, get out of here!
                if (singleCharacter == 0)
                    break;

                // add char to std::string :-)
                data += singleCharacter;
            }

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
