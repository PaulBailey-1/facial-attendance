#pragma once

#include <array>
#include <memory>

#include <boost/core/span.hpp>

#include "Map.h"

#define FACE_VEC_SIZE 128

typedef unsigned char UCHAR;

class EntityState {
public:

	int id;
	std::array<float, FACE_VEC_SIZE> facialFeatures;

	EntityState(int id_, boost::span<const UCHAR> facialFeatures_) {
		id = id_;
		memcpy(facialFeatures.data(), facialFeatures_.data(), facialFeatures_.size_bytes());
	}

	const boost::span<UCHAR> getFacialFeatures() const { return boost::span<UCHAR>(reinterpret_cast<UCHAR*>(const_cast<float*>(facialFeatures.data())), facialFeatures.size() * sizeof(float)); }

	friend bool operator < (const EntityState& a, const EntityState& b) {
		return a.id < b.id;
	}
};

typedef std::vector<std::vector<int>> iGrid;

class Entity : public EntityState {
public:

	Entity::Entity(int id, boost::span<const UCHAR> facialFeatures, std::vector<int> schedule) : EntityState(id, facialFeatures) {
		_schedule = schedule;
	}

	void generatePathMaps(const Map& map);
	void loadPathMap(int period);
	void step(float dt);

	const iGrid& getPathMap() const { return _pathMaps[_period - 1]; }
	const glm::vec2& getPos() const { return _pos; }

private:

	int _period = 1;

	std::vector<int> _schedule;
	std::vector<glm::ivec2> _startPositions;
	std::vector<iGrid> _pathMaps;
	glm::vec2 _pos = {0,0};

	void createPathMap(const Map& map, iGrid& pathMap, glm::ivec2 start);

};

class Update : public EntityState {
public:

	int deviceId;
	int shortTermStateId;

	Update(int id_, int device_id_, boost::span<const UCHAR> facialFeatures_) : EntityState(id_, facialFeatures_) {
		deviceId = device_id_;
		shortTermStateId = -1;
	}
};

class LongTermState : public EntityState {

public:

	int studentId;

	LongTermState(int id_, boost::span<const UCHAR> facialFeatures_, int studentId_) : EntityState(id_, facialFeatures_) {
		studentId = studentId_;
	}

	LongTermState(int id_, boost::span<const UCHAR> facialFeatures_) : LongTermState(id_, facialFeatures_, -1) {}
};

class ShortTermState : public EntityState {

public:

	int lastUpdateDeviceId;
	int longTermStateKey;

	ShortTermState(int id_, boost::span<const UCHAR> facialFeatures_, int lastUpdateDeviceId_, int longTermStateKey_) : EntityState(id_, facialFeatures_) {
		lastUpdateDeviceId = lastUpdateDeviceId_;
		longTermStateKey = longTermStateKey_;
	}

	ShortTermState(int id_, boost::span<const UCHAR> facialFeatures_, int lastUpdateDeviceId_) : ShortTermState(id_, facialFeatures_, lastUpdateDeviceId_, -1) {}
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