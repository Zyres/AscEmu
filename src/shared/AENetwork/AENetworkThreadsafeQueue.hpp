/*
Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

#pragma once

#include <AENetwork/AENetworkCommon.hpp>

namespace AENetwork
{
    // threadsafe queue for packets
    template<typename T>
    class ThreadsafeQueue
    {
    public:
        ThreadsafeQueue() = default;
        ThreadsafeQueue(const ThreadsafeQueue<T>&) = delete;
        virtual ~ThreadsafeQueue() { clear(); }

    public:
        // returns and maintains item at the front of the Queue
        // reference of the object of the front of the queue
        const T& front()
        {
            // protect until the line below is running. lock will "unlock" after exiting the function.
            std::scoped_lock lock(m_queueMutex);
            return m_dequeQueue.front();
        }

        // returns and maintains item at the back of the Queue
        // reference of the object of the back of the queue
        const T& back()
        {
            // protect until the line below is running. lock will "unlock" after exiting the function.
            std::scoped_lock lock(m_queueMutex);
            return m_dequeQueue.back();
        }

        // add objects at the end of the queue
        void push_back(const T& item)
        {
            std::scoped_lock lock(m_queueMutex);
            m_dequeQueue.emplace_back(std::move(item));

            std::unique_lock<std::mutex> ul(m_blockMutex);
            m_blockingCondition.notify_one();
        }

        // add objects at the front of the queue
        void push_front(const T& item)
        {
            std::scoped_lock lock(m_queueMutex);
            m_dequeQueue.emplace_front(std::move(item));

            std::unique_lock<std::mutex> ul(m_blockMutex);
            m_blockingCondition.notify_one();
        }

        // returns true if the queue has no objects
        bool empty()
        {
            std::scoped_lock lock(m_queueMutex);
            return m_dequeQueue.empty();
        }

        // returns number of items in the Queue
        size_t count()
        {
            std::scoped_lock lock(m_queueMutex);
            return m_dequeQueue.size();
        }

        // clears queue (erases all objects from the Queue)
        void clear()
        {
            std::scoped_lock lock(m_queueMutex);
            m_dequeQueue.clear();
        }

        //removes and returns object from the front of the queue
        T pop_front()
        {
            std::scoped_lock lock(m_queueMutex);
            auto t = std::move(m_dequeQueue.front());
            m_dequeQueue.pop_front();
            return t;
        }

        //removes and returns object from the back of the queue
        T pop_back()
        {
            std::scoped_lock lock(m_queueMutex);
            auto t = std::move(m_dequeQueue.back());
            m_dequeQueue.pop_back();
            return t;
        }

        void wait()
        {
            while (empty())
            {
                std::unique_lock<std::mutex> ul(m_blockMutex);
                m_blockingCondition.wait(ul);
            }
        }

    protected:
        std::mutex m_queueMutex;
        std::deque<T> m_dequeQueue;

        // used for waiting until data arrives
        std::condition_variable m_blockingCondition;
        std::mutex m_blockMutex;
    };
}
