
#include <fmt/core.h>

#include "Device.h"

Device::Device(DeviceView view) : DeviceView(view) {
	_db.connect();
}

void Device::run(const std::vector<EntityPtr>& entities) {
	for (EntityPtr entity : entities) {
		auto seen = _seenEntities.find(entity->id);
		if (view.contains(entity->getPos()) && abs(M_PI - entity->getHeading() - angle) < M_PI / 4) {
		// if (view.contains(entity->getPos())) {
			if (seen == _seenEntities.end()) {
				_seenEntities.insert(entity->id);
				_db.getEntityFeatures(entity, id);
				_db.pushUpdate(id, entity->getFacialFeatures());
			}
		} else if (seen != _seenEntities.end()) {
			_seenEntities.erase(seen);
		}
	}
}