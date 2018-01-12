#pragma once

#include <thread>
#include <vector>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <condition_variable>

#include "comm/Connection.h"
#include "jarvis.h"

namespace athena {
    class CommunicationManager
    {
        static const int MAX_CONNECTED_CLIENTS = 500;

        // For the thread pool
        std::mutex _mlock;
        std::condition_variable _cv;
        int _num_threads;
        std::vector<std::thread> _pool;

        std::mutex _conn_list_lock;
        std::unordered_map<long, comm::Connection *> _connection_map;

        // Monitor new connections queued in for worker threads
        std::queue<comm::Connection *> _workq;

        bool _shutdown;
        //until we have a separate PMGD server this db lives here
        Jarvis::Graph *_db;

        // Need this lock till we have concurrency support in JL
        // TODO: Make this reader writer.
        std::mutex *_dblock;

    public:
        CommunicationManager(Jarvis::Graph *db, std::mutex *mtx);
        ~CommunicationManager();
        void process_queue();
        void add_connection(comm::Connection *c);
        void shutdown();
    };
};
