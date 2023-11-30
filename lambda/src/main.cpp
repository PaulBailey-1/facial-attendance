
#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>

#include <fmt/core.h>

#include <utils/DBConnection.h>
#include <utils/EntityState.h>

DBConnection db;

EntityStatePtr facialMatch(EntityStatePtr update, const std::vector<EntityStatePtr>& pool) {
    for (const EntityStatePtr &cmp : pool) {
        float l2norm = 0.0;
        for (int j = 0; j < FACE_VEC_SIZE; j++) {
            l2norm += pow(update->facialFeatures[j] - cmp->facialFeatures[j], 2);
        }
        l2norm /= FACE_VEC_SIZE;
        // TODO: this should be using the variance
        if (l2norm < 0.01) {
            return cmp;
        }
    }
    return nullptr;
}

void applyUpdate(ShortTermStatePtr state, UpdateCPtr update) {
    // TODO: optimally fuse facial features
    state->lastUpdateDeviceId = update->deviceId;
}

void processUpdate(UpdatePtr update) {

    fmt::print("Proccessing update from device {}\n", update->deviceId);

    EntityStatePtr updateState = std::static_pointer_cast<EntityState>(update);
    EntityStatePtr match = nullptr;

    std::vector<EntityStatePtr> shortTermStates;
    db.getShortTermStates(shortTermStates);

    std::vector<EntityStatePtr> longTermStates;
    db.getLongTermStates(longTermStates);

    // match against who is probably there
    // match against who could be there

    // match against people seen
    match = facialMatch(updateState, shortTermStates);

    if (match != nullptr) {

        fmt::print("Match found in short term states\n");

        //if matched to short term, apply update, match to long term
        ShortTermStatePtr stMatch = std::static_pointer_cast<ShortTermState>(match);
        applyUpdate(stMatch, update);

        LongTermStatePtr ltMatch = std::static_pointer_cast<LongTermState>(facialMatch(stMatch, longTermStates));
        if (ltMatch != nullptr) {
            stMatch->longTermStateKey = ltMatch->id;
        }

        update->shortTermStateId = stMatch->id;
        db.updateShortTermState(stMatch);
        
    } else {
        // match against people known
        // may not want to do this exclusion
        std::vector<EntityStatePtr> newPool;
        std::sort(shortTermStates.begin(), shortTermStates.end(),
            [] (const EntityStatePtr& a, const EntityStatePtr& b) {
                return std::static_pointer_cast<ShortTermState>(a)->longTermStateKey < std::static_pointer_cast<ShortTermState>(b)->longTermStateKey;
        });
        std::set_difference(longTermStates.begin(), longTermStates.end(), shortTermStates.begin(), shortTermStates.end(), std::back_inserter(newPool), 
            [](const EntityStatePtr& a, const EntityStatePtr& b) {
                return std::static_pointer_cast<LongTermState>(a)->id < std::static_pointer_cast<ShortTermState>(b)->longTermStateKey;
        });
        match = facialMatch(updateState, newPool);

        // if matched to long term, create short term matched to long term
        if (match != nullptr) {
            fmt::print("Match found in long term states\n");
            update->shortTermStateId = db.createShortTermState(update, std::static_pointer_cast<LongTermState>(match));
        }
    }

    if (match == nullptr) {
        fmt::print("No match found\n");
        update->shortTermStateId = db.createShortTermState(update);
    }

    db.removePreviousUpdates(update);
    db.updateUpdate(update);
}

int main() {

    db.connect();
    //db.createTables();

    std::vector<UpdatePtr> updates;
    printf("Checking for new updates... \n");
    while (1) {
        db.getNewUpdates(updates);
        if (updates.size() == 0) continue;
        fmt::print("Got {} new updates\n", updates.size());
        for (auto i = updates.begin(); i != updates.end(); i++) {
            processUpdate(*i);
        }
        updates.clear();
    }
    
    return 0;
}