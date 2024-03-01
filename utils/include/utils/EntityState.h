#pragma once

#include <memory>

#include <boost/core/span.hpp>
#include <Eigen/dense>

#include "utils/Map.h"
#include "utils/PathGraph.h"

#define FACE_VEC_SIZE 128

typedef unsigned char UCHAR;
typedef Eigen::Matrix<float, FACE_VEC_SIZE, 1> FFVec;
typedef Eigen::Matrix<float, FACE_VEC_SIZE, FACE_VEC_SIZE> FFMat;

double l2Distance(const FFVec& first, const FFVec& second);

class Update;

class EntityState {
public:

	int id;
	FFVec facialFeatures;
	FFMat facialFeaturesCov;

	EntityState(int id_) {
		id = id_;
	}

	EntityState(int id_, boost::span<const UCHAR> facialFeatures_) {
		id = id_;
		setFacialFeatures(facialFeatures_);
	}

	EntityState(int id_, boost::span<const UCHAR> facialFeatures_, boost::span<const UCHAR> facialFeaturesCov_) : EntityState(id_, facialFeatures_) {
		memcpy(facialFeaturesCov.data(), facialFeaturesCov_.data(), facialFeaturesCov_.size_bytes());
	}

	const boost::span<UCHAR> getFacialFeatures() const { return boost::span<UCHAR>(reinterpret_cast<UCHAR*>(const_cast<float*>(facialFeatures.data())), facialFeatures.size() * sizeof(float)); }
	void setFacialFeatures(boost::span<const UCHAR> facialFeatures_) {
		memcpy(facialFeatures.data(), facialFeatures_.data(), facialFeatures_.size_bytes());
	}
	virtual const boost::span<UCHAR> getFacialFeaturesCovSpan() const { return boost::span<UCHAR>(reinterpret_cast<UCHAR*>(const_cast<float*>(facialFeaturesCov.data())), facialFeaturesCov.size() * sizeof(float)); }

	void kalmanUpdate(std::shared_ptr<Update> update);

	friend bool operator < (const EntityState& a, const EntityState& b) {
		return a.id < b.id;
	}

};

class Entity : public EntityState {
public:

	std::vector<int> schedule;

	Entity() : EntityState(0) {}

	Entity(int id) : EntityState(id) {}

	Entity(int id, std::vector<int> schedule_) : EntityState(id) {
		schedule = schedule_;
	}

	Entity(int id, boost::span<const UCHAR> facialFeatures) : EntityState(id, facialFeatures) {}

	void step(float dt);

	int getNextDoor(int period) const { return schedule[period - 1]; }
	const glm::vec2& getPos() const { return _pos; }
	float getHeading() const { return _heading; }

	void setPathMap(const iGrid* pathMap) { _pathMap = pathMap; }
	void setStartPos(glm::vec2 pos) { _pos = pos; }

private:

	glm::vec2 _pos = {0,0};
	float _heading = 0.0;
	const iGrid* _pathMap = nullptr;

};

class Update : public EntityState {
public:

	int deviceId;
	int shortTermStateId;
	int period;

	Update(int id_, int deviceId_, boost::span<const UCHAR> facialFeatures_) : EntityState(id_, facialFeatures_) {
		deviceId = deviceId_;
		shortTermStateId = -1;
		period = -1;
	}
	
	Update(int id_, int deviceId_) : EntityState(id_) {
		deviceId = deviceId_;
		shortTermStateId = -1;
		period = -1;
	}
};

class LongTermState : public EntityState {

public:

	int studentId;

	LongTermState(int id_, boost::span<const UCHAR> facialFeatures_, boost::span<const UCHAR> facialFeaturesCov_, int studentId_) : EntityState(id_, facialFeatures_, facialFeaturesCov_) {
		studentId = studentId_;
	}

	LongTermState(int id_, boost::span<const UCHAR> facialFeatures_, boost::span<const UCHAR> facialFeaturesCov_) : EntityState(id_, facialFeatures_, facialFeaturesCov_) {
		studentId = -1;
	}

	LongTermState(int id_) : EntityState(id_) {
		studentId = -1;
	}

};

class ShortTermState : public EntityState {

public:

	int lastUpdateDeviceId;
	int longTermStateKey;
	int updateCount;

	ShortTermState(int id_, boost::span<const UCHAR> facialFeatures_, boost::span<const UCHAR> facialFeaturesCov_, int updateCount_, int lastUpdateDeviceId_, int longTermStateKey_) : EntityState(id_, facialFeatures_, facialFeaturesCov_) {
		lastUpdateDeviceId = lastUpdateDeviceId_;
		longTermStateKey = longTermStateKey_;
		updateCount = updateCount_;
	}

	ShortTermState(int id_, boost::span<const UCHAR> facialFeatures_, boost::span<const UCHAR> facialFeaturesCov_, int updateCount_, int lastUpdateDeviceId_) : EntityState(id_, facialFeatures_, facialFeaturesCov_) {
		lastUpdateDeviceId = lastUpdateDeviceId_;
		longTermStateKey = -1;
		updateCount = updateCount_;
	}

	ShortTermState(int id_, int updateCount_, int lastUpdateDeviceId_, int longTermStateKey_) : EntityState(id_) {
		lastUpdateDeviceId = lastUpdateDeviceId_;
		longTermStateKey = longTermStateKey_;
		updateCount = updateCount_;
	}

	ShortTermState(int id_, int updateCount_, int lastUpdateDeviceId_) : EntityState(id_) {
		lastUpdateDeviceId = lastUpdateDeviceId_;
		longTermStateKey = -1;
		updateCount = updateCount_;
	}
};

typedef std::shared_ptr<EntityState> EntityStatePtr;
typedef std::shared_ptr<Entity> EntityPtr;
typedef std::shared_ptr<Update> UpdatePtr;
typedef std::shared_ptr<const Update> UpdateCPtr;
typedef std::shared_ptr<ShortTermState> ShortTermStatePtr;
typedef std::shared_ptr<const ShortTermState> ShortTermStateCPtr;
typedef std::shared_ptr<LongTermState> LongTermStatePtr;
typedef std::shared_ptr<const LongTermState> LongTermStateCPtr;

enum AttendanceStatus {
	PRESENT,
	ABSENT
};

struct Schedule {
	int studentId;
	std::vector<int> rooms;
};

struct Particle {
	int originDeviceId;
	int shortTermStateId;
	float weight;
};