#pragma once

#include <boost/core/span.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/mysql/tcp_ssl.hpp>

#include "EntityState.h"

class DBConnection {
public:

    DBConnection();
    ~DBConnection();

    bool connect();
    bool query(const char* sql, boost::mysql::results& result);
    
    void createTables();
    void getEntities(std::vector<EntityPtr>& vec);
    void pushUpdate(int devId, const boost::span<UCHAR> facialFeatures);
    void getNewUpdates(std::vector<UpdatePtr>& updates);
    void updateUpdate(UpdatePtr update);
    LongTermStatePtr getLongTermState(int id);
    void getLongTermStates(std::vector<EntityStatePtr>& states);
    void createLongTermState(ShortTermStatePtr sts);
    void getShortTermStates(std::vector<EntityStatePtr>& states);
    int createShortTermState(UpdateCPtr update, LongTermStatePtr ltState = nullptr);
    void updateShortTermState(ShortTermStatePtr state);
    void clearShortTermStates();
    //void setAttendance();

private:

    boost::asio::io_context _ctx;
    boost::asio::ssl::context _ssl_ctx;
    boost::mysql::tcp_ssl_connection _conn;

};