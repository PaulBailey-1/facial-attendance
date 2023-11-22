
#include <iostream>
#include <vector>

#include <fmt/core.h>

#include "DBConnection.h"
#include "EntityState.h"

DBConnection db;
int period = 1;

std::vector<EntityPtr> entites;

void applyLongTermUpdate(LongTermStatePtr lts, ShortTermStatePtr sts) {
    // TODO: should be optimal fusion
    for (int i = 0; i < FACE_VEC_SIZE; i++) {
        lts->facialFeatures[i] = sts->facialFeatures[i];
    }
}

void matchStudent(ShortTermStatePtr sts, LongTermStatePtr lts) {
    // requires map preprocessing
    // get the linked updates
    // go through all the schedules, period by period, getting rid of those that don't fit
    //      consult map of what rooms should have what devices, built from flood fill
    // return match or error
}

AttendanceStatus resolveAttendance(ShortTermStatePtr sts) {
    // see above, use same map
    return AttendanceStatus::PRESENT;
}

void nextPeriod() {

    std::vector<EntityStatePtr> shortTermStates;
    db.getShortTermStates(shortTermStates);

    for (EntityStatePtr& es : shortTermStates) {
        ShortTermStatePtr sts = std::static_pointer_cast<ShortTermState>(es);
        if (sts->longTermStateKey != -1) {
                  
            LongTermStatePtr lts = db.getLongTermState(sts->longTermStateKey);
            if (lts->studentId != -1) {
                AttendanceStatus status = resolveAttendance(sts);
                //db.setAttendance(lts, period);
            }
        }
    }
}

void nextDay() {

    std::vector<EntityStatePtr> shortTermStates;
    db.getShortTermStates(shortTermStates);

    for (EntityStatePtr& es : shortTermStates) {
        ShortTermStatePtr sts = std::static_pointer_cast<ShortTermState>(es);
        if (sts->longTermStateKey != -1) {

            LongTermStatePtr lts = db.getLongTermState(sts->longTermStateKey);
            applyLongTermUpdate(lts, sts);

            if (lts->studentId == -1) {
                matchStudent(sts, lts);
            }
        } else {
            db.createLongTermState(sts);
        }
    }
    db.clearShortTermStates();
}

int main() {

    db.connect();
    db.createTables();
    db.getEntities(entites);

    while (period <= 7) {
        std::cin.get();
        nextPeriod();
        period++;
    }
    nextDay();
    
    return 0;
}