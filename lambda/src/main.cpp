
#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <string>
#include <fstream>

#include <fmt/core.h>
#include <Eigen/dense>

#include <utils/DBConnection.h>
#include <utils/EntityState.h>

#define MATCHING_THRESH 130.0

DBConnection db;

FFMat R = FFMat::Zero();

EntityStatePtr facialMatch(EntityStatePtr update, const std::vector<EntityStatePtr>& pool) {

    for (const EntityStatePtr &cmp : pool) {
        float distance = 0.0;

        // L2Norm
        for (int j = 0; j < FACE_VEC_SIZE; j++) {
            distance += pow(update->facialFeatures[j] - cmp->facialFeatures[j], 2);
        }
        
        // Bhattacharyya distance
        //std::unique_ptr<FFVec> diff = std::unique_ptr<FFVec>(new FFVec());
        //std::unique_ptr<FFMat> sigma = std::unique_ptr<FFMat>(new FFMat());
        //*diff = update->facialFeatures - cmp->facialFeatures;
        //*sigma = (update->getFacialFeaturesCov() + cmp->facialFeaturesCov) / 2;
        //distance = float(diff->transpose() * sigma->inverse() * *diff) / 8 + log(sigma->determinant() / sqrt(update->getFacialFeaturesCov().determinant() * cmp->facialFeaturesCov.determinant())) / 2;

        fmt::println("Matching update {} to state {} with distance {}", update->id, cmp->id, distance);
        if (distance < MATCHING_THRESH) {
            return cmp;
        }
    }
    return nullptr;
}

void processUpdate(UpdatePtr update) {

    fmt::print("Proccessing update {} from device {}\n", update->id, update->deviceId);

    EntityStatePtr updateState = std::static_pointer_cast<EntityState>(update);
    EntityStatePtr match = nullptr;

    std::vector<EntityStatePtr> shortTermStates;
    db.getShortTermStates(shortTermStates);

    std::vector<EntityStatePtr> longTermStates;
    db.getLongTermStates(longTermStates);

    // TODO
    // match against who is probably there
    // match against who could be there

    // match against people seen
    match = facialMatch(updateState, shortTermStates);

    if (match != nullptr) {

        fmt::print("Match found in short term states\n");

        //if matched to short term, apply update, match to long term
        ShortTermStatePtr stMatch = std::static_pointer_cast<ShortTermState>(match);
  
        stMatch->lastUpdateDeviceId = update->deviceId;
        stMatch->kalmanUpdate(update);

        fmt::println("Rematching sts {} to long term states", stMatch->id);
        LongTermStatePtr ltMatch = std::static_pointer_cast<LongTermState>(facialMatch(stMatch, longTermStates));
        if (ltMatch != nullptr) {
            stMatch->longTermStateKey = ltMatch->id;
        }

        update->shortTermStateId = stMatch->id;
        stMatch->updateCount++;
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

void loadUpdateCov(std::string filename) {
    fmt::print("Loading update covariance matrix from {} ... ", filename);
    try {
        std::ifstream file(filename);
        int updateNum = 0;
        int entity = 0;
        if (file.is_open()) {
            std::string line;
            int row = 0;
            while (1) {
                std::getline(file, line);
                if (file.eof()) break;
                std::stringstream s(line);
                std::string num;
                int col = 0;
                while (std::getline(s, num, ',')) {
                    if (row == col)
                       R(row, col) = stof(num);
                    col++;
                }
                if (col != FACE_VEC_SIZE) {
                    throw std::runtime_error("invalid column dimension\n");
                }
                row++;
            }
            if (row != FACE_VEC_SIZE) {
                throw std::runtime_error("invalid row dimension\n");
            }
        }
        Update::defaultCov = &R;
        printf("Done\n");
    }
    catch (const std::exception& err) {
        std::cerr << "Failed to read update covariance: " << err.what() << std::endl;
    }
}

int main() {

    db.connect();
    //db.createTables();
    
    loadUpdateCov("../../../../updateCov.csv");

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