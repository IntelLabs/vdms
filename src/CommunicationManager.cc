#include "CommunicationManager.h"
#include "QueryHandler.h"

#include "AthenaConfig.h"

using namespace athena;
using namespace Jarvis;

CommunicationManager::CommunicationManager(Jarvis::Graph *db,std::mutex *mtx):
    _db(db), _dblock(mtx)
{
    _num_threads = AthenaConfig::instance() ->get_int_value(
                                        "max_simultaneous_clients",
                                        MAX_CONNECTED_CLIENTS);

    if (_num_threads > MAX_CONNECTED_CLIENTS)
        _num_threads = MAX_CONNECTED_CLIENTS;

    _shutdown = false;
    for (int i = 0; i < _num_threads; ++i)
        _pool.push_back(std::thread(&CommunicationManager::process_queue, this));
}

void CommunicationManager::process_queue()
{
    comm::Connection *c;
    while(true) {
        {
            std::unique_lock<std::mutex> lock(_mlock);
            _cv.wait(lock, [&] { return !_workq.empty(); });
            if (_shutdown)
                break;
            c = _workq.front();
            _workq.pop();
        }
        if (c != NULL) {
            _conn_list_lock.lock();
            auto c_it = _conn_list.insert(_conn_list.begin(), c);
            _conn_list_lock.unlock();

            QueryHandler qh(_db, _dblock);
            printf("Connection received...\n");
            qh.process_connection(c);

            std::unique_lock<std::mutex> conn_list_lock(_conn_list_lock);
            _conn_list.erase(c_it);
            delete c;
        }
    }
}

CommunicationManager::~CommunicationManager()
{
    // Kill all connections by closing the sockets
    // If not, QueryHandler will be blocked on process_connection()
    for (auto connection : _conn_list) {
        connection->shutdown();
    }

    for (int i = 0; i < _num_threads; ++i) {
        _pool[i].join();
    }
}

void CommunicationManager::add_connection(comm::Connection *c)
{
    {
        std::unique_lock<std::mutex> lock(_mlock);
        _workq.push(c);
    }
    _cv.notify_one();
}

void CommunicationManager::shutdown()
{
    _shutdown = true;
    add_connection(NULL);
    _cv.notify_all();
}
