
#include <iostream>
#include <vector>
#include <set>
#include <fstream>
#include <string>

#include <fmt/core.h>

#include <utils/DBConnection.h>
#include <utils/EntityState.h>

DBConnection db;
int period = 1;

std::vector<Schedule> schedules;
std::vector<std::set<int>> devDoorsMatches;

int matchStudent(ShortTermStatePtr sts) {
    std::vector<PathGraphPtr> paths;
    db.getPaths(sts, paths);
    std::vector<int> devPath;
    for (PathGraphPtr& path : paths) {
        devPath.push_back(path->getFinalDev());
    }
    // db.getUpdatesPath(stsId, devPath);

    std::vector<Schedule> possible = schedules;

    for (int per = 0; per < devPath.size(); per++) {
        std::set<int> doors = devDoorsMatches[devPath[per]];
        for (std::vector<Schedule>::iterator i = possible.begin(); i != possible.end();) {
            if (doors.find(i->rooms[per]) == doors.end()) {
                i = possible.erase(i);
            } else {
                i++;
            }
        }
    }

    if (possible.size() == 0) {
        fmt::println("Failed to match student for sts {}", sts->id);
    }

    if (possible.size() > 1) {
        fmt::print("Error: failed to match student for sts: {}, matched to students \n", sts->id);
        for (Schedule& sch : possible) {
            fmt::print("{}, ", sch.studentId);
        }
        printf("\n");
    }

    if (possible.size() == 1) {
        fmt::println("Matched sts {} to student {}", sts->id, possible[0].studentId);
        return possible[0].studentId;
    }

    return -1;
}

void setAttendance(int lastDevId, int studentId) {
    AttendanceStatus status;
    int room = db.getScheduledRoom(studentId, period);
    std::set<int>& doors = devDoorsMatches[lastDevId];
    if (doors.find(room) != doors.end()) {
        status = AttendanceStatus::PRESENT;
    } else {
        status = AttendanceStatus::ABSENT;
    }
    db.setAttendance(room, period, studentId, status);
}

void nextPeriod() {

    fmt::print("Running period {}\n", period);

    std::set<int> ltsIds;
    db.getLinkedLongTermStateIds(ltsIds);
    
    for (auto i = ltsIds.begin(); i != ltsIds.end(); i++) {
        ShortTermStatePtr sts = db.getLastShortTermState(*i);
        LongTermStatePtr lts = db.getLongTermState(sts->longTermStateKey);
        if (lts->studentId != -1) {
            setAttendance(sts->lastUpdateDeviceId, lts->studentId);
        }
    }

    // db.setUpdatesPeriod(period);
    db.clearParticles();
}

void nextDay() {

    printf("\nRunning next day\n");

    std::vector<ShortTermStatePtr> shortTermStates;
    db.getShortTermStates(shortTermStates);

    // Update lts
    for (ShortTermStatePtr& sts : shortTermStates) {
        LongTermStatePtr lts = nullptr;
        if (sts->longTermStateKey != -1) {

            lts = db.getLongTermState(sts->longTermStateKey);

            lts->kalmanUpdate(sts);
            db.updateLongTermState(lts);

            std::vector<PathGraphPtr> stsPaths;
            db.getPaths(sts, stsPaths);
            for (PathGraphPtr path : stsPaths) {
                PathGraphPtr ltsPath = db.getPath(lts, path->period);
                ltsPath->fuse(path);
                db.updatePath(ltsPath);
            }

        // Promote sts to lts
        } else if (sts->updateCount > 2) {
            fmt::println("Promoting sts {} with {} updates", sts->id, sts->updateCount);
            int ltsId = db.createLongTermState(sts);
            lts = LongTermStatePtr(new LongTermState(ltsId)); 
            db.copyPaths(sts, lts);
        }

        // Match lts student
        if (lts != nullptr && lts->studentId == -1 && sts->updateCount > 1) {
            lts->studentId = matchStudent(sts);
            if (lts->studentId != -1)
                db.setLongTermStateStudent(lts);
        }
    }
    
    db.clearUpdates();
    db.clearParticles();
    db.clearStsPaths();
    db.clearShortTermStates();
}

int main() {

    PathGraph::initGraph("../../../map.xml", "pathGraph.csv");

    db.connect();
    // db.createTables();
    // db.initGlobals();

    printf("Loading map ... ");
    Map map("../../../map.xml");
    printf("Done\nMatching doors and devices ... ");
    std::vector<std::set<int>> doorDevsMatches;
    map.generatePathMaps(doorDevsMatches);

    // flip matches
    for (int i = 0; i < map.devs.size(); i++) {
        int dev = map.devs[i].id;
        std::set<int> doors;
        for (int door = 0; door < doorDevsMatches.size(); door++) {
            std::set<int> devs = doorDevsMatches[door];
            if (devs.find(dev) != devs.end()) {
                doors.insert(door);
            }
        }
        devDoorsMatches.push_back(doors);
    }

    printf("Done\n");

    db.getSchedules(schedules);

    printf("Ready\n");

    while (1) {
        while (period <= 3) {
            std::cin.get();
            nextPeriod();
            period++;
            db.setPeriod(period);
        }
        std::cin.get();
        nextDay();
        period = 1;
        db.setPeriod(period);
    }

    return 0;
}