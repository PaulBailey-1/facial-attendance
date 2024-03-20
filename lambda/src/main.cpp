
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
#include <utils/PathGraph.h>

#define MATCHING_THRESH 140.0

DBConnection db;

FFMat R = FFMat::Zero();
double speed = 1.0;

void computeParticleTimes(Particle particle, PathGraphPtr path) {
    std::chrono::time_point startTime = db.getTime();
    int node = particle.id;
    int nextNode = -1;
    while(nextNode = path->getNext(node) != -1) {
        double distance = PathGraph::getGraphEdgeLength(node, nextNode);
        db.addParticleTime(particle, node, startTime + std::chrono::milliseconds((distance / speed) * 1000));
    }
}

void getFacialMatches(UpdatePtr update, const std::vector<ShortTermStatePtr>& pool, std::vector<ShortTermStatePtr>& matches, std::vector<double>& matchDistances) {

    try {
        for (const ShortTermStatePtr &cmp : pool) {

            double distance = l2Distance(update->facialFeatures, cmp->facialFeatures);
            
            // Bhattacharyya distance
            //std::unique_ptr<FFVec> diff = std::unique_ptr<FFVec>(new FFVec());
            //std::unique_ptr<FFMat> sigma = std::unique_ptr<FFMat>(new FFMat());
            //*diff = update->facialFeatures - cmp->facialFeatures;
            //*sigma = (update->facialFeaturesCov + cmp->facialFeaturesCov) / 2;
            //distance = float(diff->transpose() * sigma->inverse() * *diff) / 8 + log(sigma->determinant() / sqrt(update->facialFeaturesCov.determinant() * cmp->facialFeaturesCov.determinant())) / 2;

            fmt::println("Matching update {} to state {} with distance {}", update->id, cmp->id, distance);
            if (std::isnan(distance)) {
                throw std::runtime_error("NaN distance");
            }
            if (distance == 0) {
                throw std::runtime_error("0 distance");
            }
            if (distance < MATCHING_THRESH) {
                matches.push_back(cmp);
                matchDistances.push_back(distance);
            }
        }
    } catch (std::exception& e) {
        fmt::println("main:getFacialMatches Error - {}", e.what());
    }
}

LongTermStatePtr getFacialMatch(ShortTermStatePtr sts, const std::vector<LongTermStatePtr>& pool) {
    try {
        double smallestDistance = -1;
        LongTermStatePtr closest = nullptr;
        for (const LongTermStatePtr &cmp : pool) {

            double distance = l2Distance(sts->facialFeatures, cmp->facialFeatures);
            
            fmt::println("Matching sts {} to lts {} with distance {}", sts->id, cmp->id, distance);
            if (std::isnan(distance)) {
                throw std::runtime_error("NaN distance");
            }
            if (distance == 0) {
                throw std::runtime_error("0 distance");
            }

            if (distance < smallestDistance || smallestDistance == -1) {
                smallestDistance = distance;
                closest = cmp;
            }
        }
        if (smallestDistance < MATCHING_THRESH) {
            return closest;
        }
    } catch (std::exception& e) {
        fmt::println("main:getFacialMatch Error - {}", e.what());
    }
    return nullptr;
}

void processUpdate(UpdatePtr update) {

    fmt::print("Proccessing update {} from device {}\n", update->id, update->deviceId);

    update->facialFeaturesCov = R;
    int period = db.getPeriod();

    std::vector<ShortTermStatePtr> shortTermStates;
    db.getShortTermStates(shortTermStates);

    std::vector<LongTermStatePtr> longTermStates;
    db.getLongTermStates(longTermStates);

    std::vector<ShortTermStatePtr> matches;
    std::vector<double> matchDistances;
    // TODO
    // match against who is probably there
    // match against who could be there

    // match against people seen
    getFacialMatches(update, shortTermStates, matches, matchDistances);

    fmt::println("Found {} matches in short term states", matches.size());
    for (int i = 0; i < matches.size(); i++) { 
        ShortTermStatePtr match = matches[i];

        //if matched to short term, apply update
        PathGraphPtr path = db.getPath(match, period);
        if (match->lastUpdateDeviceId != -1) {
            path->update(match->lastUpdateDeviceId, update->deviceId);
        } else {
            path->start(update->deviceId);
        }
        db.updatePath(path);

        match->lastUpdateDeviceId = update->deviceId;
        match->kalmanUpdate(update);

        fmt::println("Rematching sts {} to long term states", match->id);
        LongTermStatePtr ltMatch = getFacialMatch(match, longTermStates);
        if (ltMatch != nullptr) {
            match->longTermStateKey = ltMatch->id;
        }

        // update->shortTermStateId = match->id;
        match->updateCount++;
        db.updateShortTermState(match);

        double weight = 1 - (matchDistances[i] / MATCHING_THRESH); // 0 to 1
        Particle particle = db.createParticle(match->id, update, weight);

        computeParticleTimes(particle, path);

    }

    // } else {
    //     // match against people known
    //     // may not want to do this exclusion
    //     std::vector<EntityStatePtr> newPool;
    //     std::sort(shortTermStates.begin(), shortTermStates.end(),
    //         [] (const EntityStatePtr& a, const EntityStatePtr& b) {
    //             return std::static_pointer_cast<ShortTermState>(a)->longTermStateKey < std::static_pointer_cast<ShortTermState>(b)->longTermStateKey;
    //     });
    //     std::set_difference(longTermStates.begin(), longTermStates.end(), shortTermStates.begin(), shortTermStates.end(), std::back_inserter(newPool), 
    //         [](const EntityStatePtr& a, const EntityStatePtr& b) {
    //             return std::static_pointer_cast<LongTermState>(a)->id < std::static_pointer_cast<ShortTermState>(b)->longTermStateKey;
    //     });
    //     match = facialMatch(updateState, newPool);

    //     // if matched to long term, create short term matched to long term
    //     if (match != nullptr) {
    //         fmt::print("Match found in long term states\n");
    //         update->shortTermStateId = db.createShortTermState(update, std::static_pointer_cast<LongTermState>(match));
    //     }
    // }

    if (matches.size() == 0) {
        fmt::print("No match found\n");
        int stsId = db.createShortTermState(update);
        db.createParticle(stsId, update, 1.0);
    }

    db.removeUpdate(update);
    // db.removePreviousUpdates(update);
    // db.updateUpdate(update);
}

int main() {

    loadUpdateCov("../../../updateCov.csv", R);

    db.connect();
    //db.createTables();

    PathGraph::initGraph("../../../map.xml", "pathGraph.csv");

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