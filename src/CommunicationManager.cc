#include "CommunicationManager.h"
#include "QueryHandler.h"

#define BUFFER_QUERY_SIZE (1024*4) // Buffer for receiving query is 4KB

using namespace athena;
using namespace Jarvis;

CommunicationManager::CommunicationManager(Graph *db, std::mutex *mtx)
{
    _db = db;
    _dblock = mtx;

    // TODO: Need network configuration here across all partition instances.

    _num_threads = std::thread::hardware_concurrency();
    _shutdown = false;
    for (int i = 0; i < _num_threads; ++i)
        _pool.push_back(std::thread(&CommunicationManager::process_queue, this));
}

void CommunicationManager::process_queue()
{
    comm::Connection *c;
    while(true) {
        {
            if (_shutdown)
                break;
            std::unique_lock<std::mutex> _lock(_mlock);
            while (_workq.empty())
                _cv.wait(_lock);
            c = _workq.front();
            _workq.pop();
        }
        if (c != NULL) {
            QueryHandler _qh(_db, _dblock);
            _qh.process_query(c);
        }
    }
}

CommunicationManager::~CommunicationManager()
{
    for (int i = 0; i < _num_threads; ++i)
        _pool[i].join();
}

void CommunicationManager::add_connection(comm::Connection *c)
{
    std::unique_lock<std::mutex> _lock(_mlock);
    _workq.push(c);
    _cv.notify_one();
}

void CommunicationManager::shutdown()
{
    _shutdown = true;
    for (int i = 0; i < _num_threads; ++i)
        add_connection(NULL);
}
