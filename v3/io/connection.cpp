#include "connection.h"
//这个函数把用户的连接放进初始化表，
std::shared_ptr<servercore::Connection> servercore::ConnectionStore::Add(
    int fd, const sockaddr_in &addr, socklen_t addr_len)
{
    MutexlockGuard lock(_mutex);
    std::shared_ptr<Connection> conn(new Connection(fd));
    conn->_addr = addr;
    conn->_addr_len = addr_len;
    _connections[fd] = conn;
    return conn;
}

//这个函数实现在拿到FD
std::shared_ptr<servercore::Connection> servercore::ConnectionStore::Get(int fd)
{
    MutexlockGuard lock(_mutex);
    std::unordered_map<int, std::shared_ptr<Connection> >::iterator it = _connections.find(fd);
    if (it == _connections.end())
    {
        return std::shared_ptr<Connection>();
    }
    return it->second;
}

//删除连接？
void servercore::ConnectionStore::Remove(int fd)
{
    MutexlockGuard lock(_mutex);
    _connections.erase(fd);
}

std::vector<std::shared_ptr<servercore::Connection> > servercore::ConnectionStore::Snapshot()
{
    MutexlockGuard lock(_mutex);
    std::vector<std::shared_ptr<Connection> > snapshot;
    snapshot.reserve(_connections.size());
    for (std::unordered_map<int, std::shared_ptr<Connection> >::iterator it = _connections.begin();
         it != _connections.end();
         ++it)
    {
        snapshot.push_back(it->second);
    }
    return snapshot;
}
