

#include "Device.h"

Device::Device(DeviceLoc loc) : _id(loc.id), _pos(loc.pos) {

	_view.moveTo({0, 0});
	_view.lineTo({ CAM_RANGE, 0 });
	_view.arc({0, 0}, CAM_RANGE, 0.0, CAM_ANGLE);
	_view.close();
	glm::mat3 trans(1.0);
	trans = glm::translate(trans, _pos);
	trans = glm::rotate(trans, float(-CAM_ANGLE / 2 + loc.angle * (M_PI / 180)));
	_view.transform(trans);

	_db.connect();
}

void Device::run(const std::vector<EntityPtr>& entities) {
	for (EntityPtr entity : entities) {
		auto seen = _seenEntities.find(entity->id);
		if (_view.contains(entity->getPos())) {
			if (seen == _seenEntities.end()) {
				_seenEntities.insert(entity->id);
				_db.pushUpdate(_id, entity->getFacialFeatures());
			}
		} else if (seen != _seenEntities.end()) {
			_seenEntities.erase(seen);
		}
	}
}