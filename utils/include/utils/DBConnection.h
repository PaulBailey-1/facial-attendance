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
    void clearTables();

    void getEntities(std::vector<EntityPtr>& vec);
    bool getEntityFeatures(EntityPtr entity, int devId);

    void pushUpdate(int devId, const boost::span<UCHAR> facialFeatures);
    void getNewUpdates(std::vector<UpdatePtr>& updates);
    void updateUpdate(UpdatePtr update);
    void removePreviousUpdates(UpdatePtr update);
    void setUpdatesPeriod(int period);
    void getUpdatesPath(int stsId, std::vector<int>& devPath);
    void clearUpdates();

    LongTermStatePtr getLongTermState(int id);
    void getLongTermStates(std::vector<LongTermStatePtr>& states);
    int createLongTermState(ShortTermStatePtr sts);
    void updateLongTermState(LongTermStatePtr lts);
    void setLongTermStateStudent(LongTermStatePtr lts);

    void getShortTermStates(std::vector<ShortTermStatePtr>& states, bool small = false);
    void getLinkedLongTermStateIds(std::set<int>& ids);
    ShortTermStatePtr getLastShortTermState(int ltsId);
    int createShortTermState(UpdateCPtr update, LongTermStatePtr ltState = nullptr);
    void updateShortTermState(ShortTermStatePtr state);
    void clearShortTermStates();

    int getScheduledRoom(int studentId, int period);
    void getSchedules(std::vector<Schedule>& schedules);
    void addToSchedule(int studentId, int period, int roomId);
    void setAttendance(int room, int period, int studentId, AttendanceStatus status);
    int getPeriod();

    int addStudent();
    void pushStudentData(UpdatePtr data, int studentId);

private:

    boost::asio::io_context _ctx;
    boost::asio::ssl::context _ssl_ctx;
    boost::mysql::tcp_ssl_connection _conn;

};