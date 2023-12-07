
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

void applyLongTermUpdate(LongTermStatePtr lts, ShortTermStatePtr sts) {
    // TODO: should be optimal fusion
    for (int i = 0; i < FACE_VEC_SIZE; i++) {
        lts->facialFeatures[i] = sts->facialFeatures[i];
    }
}

int matchStudent(int stsId) {
    std::vector<int> devPath;
    db.getUpdatesPath(stsId, devPath);
    std::vector<Schedule> possible = schedules;

    for (int per = 0; per < devPath.size(); per++) {
        std::set<int> doors = devDoorsMatches[devPath[per]];
        for (auto i = possible.begin(); i != possible.end(); i++) {
            if (doors.find(i->rooms[per]) == doors.end()) {
                possible.erase(i);
                i--;
            }
        }
    }

    if (possible.size() > 1) {
        fmt::print("Error: failed to match student for sts: {}, matched to students \n");
        for (Schedule& sch : possible) {
            fmt::print("{}, ", sch.studentId);
        }
        printf("\n");
    }

    if (possible.size() == 1) {
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

    std::vector<EntityStatePtr> shortTermStates;
    db.getShortTermStates(shortTermStates);

    for (EntityStatePtr& es : shortTermStates) {
        ShortTermStatePtr sts = std::static_pointer_cast<ShortTermState>(es);
        if (sts->longTermStateKey != -1) {
                  
            LongTermStatePtr lts = db.getLongTermState(sts->longTermStateKey);
            if (lts->studentId != -1) {
                setAttendance(sts->lastUpdateDeviceId, lts->studentId);
            }
        }
    }

    db.setUpdatesPeriod(period);
}

void nextDay() {

    printf("Running next day\n");

    std::vector<EntityStatePtr> shortTermStates;
    db.getShortTermStates(shortTermStates);

    for (EntityStatePtr& es : shortTermStates) {
        ShortTermStatePtr sts = std::static_pointer_cast<ShortTermState>(es);
        LongTermStatePtr lts;
        if (sts->longTermStateKey != -1) {

            lts = db.getLongTermState(sts->longTermStateKey);
            applyLongTermUpdate(lts, sts);

            db.updateLongTermState(lts);

        } else {
            int ltsId = db.createLongTermState(sts);
            lts = LongTermStatePtr(new LongTermState(ltsId));
        }

        if (lts->studentId == -1) {
            lts->studentId = matchStudent(sts->id);
            if (lts->studentId != -1)
                db.setLongTermStateStudent(lts);
        }
    }
    
    db.clearUpdates();
    db.clearShortTermStates();
}

void uploadDataSet(std::string filename, int entities, int imgs) {
    fmt::print("Uploading dataset to db from {} ... ", filename);
    try {
        std::ifstream file(filename);
        int updateNum = 0;
        int entity = 0;
        if (file.is_open()) {
            std::string line;
            while (1) {
                std::getline(file, line);
                if (file.eof()) break;
                std::stringstream s(line);
                std::string num;
                UpdatePtr data = UpdatePtr(new Update(0, updateNum));
                updateNum++;
                if (updateNum == imgs) {
                    updateNum = 0;
                    entity++;
                }
                int feature = 0;
                while (std::getline(s, num, ',')) {
                    data->facialFeatures[feature] = stof(num);
                    feature++;
                }
                if (feature != FACE_VEC_SIZE) {
                    throw std::runtime_error("improper feature vector dimensions\n");
                }
                db.pushStudentData(data, entity);
            }
            if (updateNum != 0) {
                throw std::runtime_error("inproper alignment\n");
            }
            if (entity == entities) {
                throw std::runtime_error("unexpected number of entities\n");
            }
        }
        file.close();
        printf("Done\n");
    }
    catch (const std::exception& err) {
        std::cerr << "Failed to read dataset: " << err.what() << std::endl;
    }
}

int main() {

    db.connect();
    db.createTables();

    uploadDataSet("../../../../dataset.csv", 9, 12);

    printf("Loading map ... ");
    Map map("../../../../map.xml");
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

    while (1) {
        while (period <= 3) {
            std::cin.get();
            nextPeriod();
            period++;
        }
        nextDay();
        period = 1;
    }

    return 0;
}