#pragma once

#include <boost/core/span.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/mysql/tcp_ssl.hpp>

#include "EntityState.h"
#include "PathGraph.h"

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
    void getEntitiesFeatures(std::vector<EntityPtr>& vec);

    void pushUpdate(int devId, const boost::span<UCHAR> facialFeatures);
    void getNewUpdates(std::vector<UpdatePtr>& updates);
    void updateUpdate(UpdatePtr update);
    void removeUpdate(UpdatePtr update);
    void removePreviousUpdates(UpdatePtr update);
    void setUpdatesPeriod(int period);
    void getUpdatesPath(int stsId, std::vector<int>& devPath);
    void clearUpdates();

    void createParticle(int stsId, UpdatePtr update, double weight);
    void getParticles(std::vector<Particle>& particles);
    void clearParticles();

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

    PathGraphPtr getPath(ShortTermStatePtr sts, int period);
    PathGraphPtr getPath(LongTermStatePtr lts, int period);
    void getPaths(ShortTermStatePtr sts, std::vector<PathGraphPtr>& paths);
    void updatePath(PathGraphPtr path);
    void copyPaths(ShortTermStatePtr sts, LongTermStatePtr lts);
    void clearStsPaths();

    int getScheduledRoom(int studentId, int period);
    void getSchedules(std::vector<Schedule>& schedules);
    void addToSchedule(int studentId, int period, int roomId);
    void setAttendance(int room, int period, int studentId, AttendanceStatus status);

    int getPeriod();
    void setPeriod(int period);

    int addStudent();
    void pushStudentData(UpdatePtr data, int studentId);

    void initGlobals();

private:

    boost::asio::io_context _ctx;
    boost::asio::ssl::context _ssl_ctx;
    boost::mysql::tcp_ssl_connection _conn;

};