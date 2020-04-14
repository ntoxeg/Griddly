#include "Grid.hpp"

#include <spdlog/spdlog.h>

#include <iostream>
#include <memory>
#include <utility>
#include <vector>

namespace griddy {

Grid::Grid() {

#ifndef NDEBUG
  spdlog::set_level(spdlog::level::debug);
#else
  spdlog::set_level(spdlog::level::info);
#endif

  gameTick = 0;
}

void Grid::init(uint width, uint height) {
  spdlog::debug("Setting grid dimensions to: [{0}, {1}]", width, height);
  height_ = height;
  width_ = width;

  occupiedLocations_.clear();
  objects_.clear();

}

bool Grid::updateLocation(std::shared_ptr<Object> object, GridLocation previousLocation, GridLocation newLocation) {
  occupiedLocations_.erase(previousLocation);
  occupiedLocations_.insert({newLocation, object});

  updatedLocations_.insert(previousLocation);
  updatedLocations_.insert(newLocation);
}

std::unordered_set<GridLocation, GridLocation::Hash> Grid::getUpdatedLocations() const {
  return updatedLocations_;
}

std::vector<int> Grid::performActions(int playerId, std::vector<std::shared_ptr<Action>> actions) {
  std::vector<int> rewards;

  // Reset the locations that need to be updated
  updatedLocations_.clear();

  spdlog::trace("Tick {0}", gameTick);
  for (auto action : actions) {
    auto sourceObject = getObject(action->getSourceLocation());
    auto destinationObject = getObject(action->getDestinationLocation());

    spdlog::debug("Player={0} performing action=({1})", playerId, action->getDescription());

    if (sourceObject == nullptr) {
      spdlog::trace("Cannot perform action on empty space.");
      rewards.push_back(0);
      continue;
    }

    auto sourceObjectPlayerId = sourceObject->getPlayerId();

    if (sourceObjectPlayerId != 0 && sourceObjectPlayerId != playerId) {
      spdlog::trace("Cannot perform action on objects not owned by player.");
      rewards.push_back(0);
      continue;
    }

    if (sourceObject->canPerformAction(action)) {
      spdlog::trace("Action={0} can be performed by Unit={1}", action->getDescription(), sourceObject->getDescription());

      auto actionResult = destinationObject == nullptr ? true : destinationObject->onActionPerformed(sourceObject, action);

      if (actionResult) {
        spdlog::debug("Action={0} performed on Object={1}", action->getDescription(), sourceObject->getDescription());
        auto actionReward = sourceObject->onPerformAction(destinationObject, action);
        rewards.push_back(actionReward);
      } else {
        spdlog::trace("Action={0} cannot be performed on Object={1}", action->getDescription(), destinationObject->getDescription());
        rewards.push_back(0);
      }
    } else {
      spdlog::trace("Action={0} cannot be performed by Unit={1}", action->getDescription(), sourceObject->getDescription());
      rewards.push_back(0);
    }
  }

  return rewards;
}

void Grid::update() {
  gameTick++;
}

uint Grid::getTickCount() const {
  return gameTick;
}

std::unordered_set<std::shared_ptr<Object>>& Grid::getObjects() {
  return this->objects_;
}

std::shared_ptr<Object> Grid::getObject(GridLocation location) const {
  auto i = occupiedLocations_.find(location);
  if (i == occupiedLocations_.end()) {
    return nullptr;
  } else {
    return i->second;
  }
}

void Grid::initObject(uint playerId, GridLocation location, std::shared_ptr<Object> object) {
  spdlog::debug("Adding object={0} to location: [{1},{2}]", object->getObjectType(), location.x, location.y);

  auto canAddObject = objects_.insert(object).second;
  if (canAddObject) {
    object->init(playerId, location, shared_from_this());
    auto canAddToLocation = occupiedLocations_.insert({location, object}).second;
    if (!canAddToLocation) {
      objects_.erase(object);
    }
  }
}

bool Grid::removeObject(std::shared_ptr<Object> object) {
  spdlog::debug("Removing object={0} from environment.", object->getDescription());

  if (objects_.erase(object) > 0 && occupiedLocations_.erase(object->getLocation()) > 0) {
    return true;
  } else {
    spdlog::error("Could not remove object={0} from environment.", object->getDescription());
    return false;
  }
}

uint Grid::getWidth() const { return width_; }

uint Grid::getHeight() const { return height_; }

Grid::~Grid() {}
}  // namespace griddy