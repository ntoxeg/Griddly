#include <spdlog/spdlog.h>

#include <fstream>

#include "VulkanConfiguration.hpp"
#include "VulkanDevice.hpp"
#include "VulkanInstance.hpp"
#include "VulkanObserver.hpp"

namespace griddly {

std::shared_ptr<vk::VulkanInstance> VulkanObserver::instance_ = nullptr;

VulkanObserver::VulkanObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig, ShaderVariableConfig shaderVariableConfig) : Observer(grid), resourceConfig_(resourceConfig), shaderVariableConfig_(shaderVariableConfig) {
}

VulkanObserver::~VulkanObserver() {
}

/**
 * Only load vulkan if update() called, allows many environments with vulkan-based global observers to be used. 
 * But only loads them if global observations are requested, for example for creating videos
 * 
 * This a) allows significantly more enviroments to be loaded (if only one of them is being used to create videos) and b) 
 */
void VulkanObserver::lazyInit() {
  if (observerState_ != ObserverState::RESET) {
    throw std::runtime_error("Cannot initialize Vulkan Observer when it is not in RESET state.");
  }

  spdlog::debug("Vulkan lazy initialization....");

  gridBoundary_ = glm::ivec2(grid_->getWidth(), grid_->getHeight());

  auto imagePath = resourceConfig_.imagePath;
  auto shaderPath = resourceConfig_.shaderPath;

  auto configuration = vk::VulkanConfiguration();
  if (instance_ == nullptr) {
    instance_ = std::shared_ptr<vk::VulkanInstance>(new vk::VulkanInstance(configuration));
  }

  std::unique_ptr<vk::VulkanDevice> vulkanDevice(new vk::VulkanDevice(instance_, observerConfig_.tileSize, shaderPath));

  device_ = std::move(vulkanDevice);
  device_->initDevice(false);

  // This is probably far too big for most circumstances, but not sure how to work this one out in a smarter way,
  const int maxObjects = grid_->getWidth() * grid_->getHeight() * 3;

  device_->initializeSSBOs(
      shaderVariableConfig_.exposedGlobalVariables.size(),
      grid_->getPlayerCount(),
      shaderVariableConfig_.exposedObjectVariables.size(),
      maxObjects);

  observerState_ = ObserverState::READY;
}

void VulkanObserver::reset() {
  Observer::reset();

  if (observerState_ == ObserverState::READY) {
    resetRenderSurface();
  }
}

uint8_t* VulkanObserver::update() {
  if (observerState_ == ObserverState::RESET) {
    lazyInit();
    resetRenderSurface();
  } else if (observerState_ != ObserverState::READY) {
    throw std::runtime_error("Observer is not in READY state, cannot render");
  }

  bool updateCommandBuffers = updateShaderBuffers();

  if (updateCommandBuffers) {
    device_->startRecordingCommandBuffer();

    render();

    device_->endRecordingCommandBuffer(std::vector<VkRect2D>{{{0, 0}, {pixelWidth_, pixelHeight_}}});
  }

  grid_->purgeUpdatedLocations(observerConfig_.playerId);

  return device_->renderFrame();
}

void VulkanObserver::resetRenderSurface() {
  spdlog::debug("Initializing Render Surface. Grid width={0}, height={1}. Pixel width={2}. height={3}", gridWidth_, gridHeight_, pixelWidth_, pixelHeight_);
  observationStrides_ = device_->resetRenderSurface(pixelWidth_, pixelHeight_);
}

void VulkanObserver::release() {
  device_.reset();
}

void VulkanObserver::print(std::shared_ptr<uint8_t> observation) {
  auto tileSize = observerConfig_.tileSize;
  std::string filename = fmt::format("{0}.ppm", *grid_->getTickCount());
  std::ofstream file(filename, std::ios::out | std::ios::binary);

  auto width = grid_->getWidth() * tileSize.x;
  auto height = grid_->getHeight() * tileSize.y;

  // ppm header
  file << "P6\n"
       << width << "\n"
       << height << "\n"
       << 255 << "\n";

  file.write((char*)observation.get(), width * height * 3);
  file.close();
}

}  // namespace griddly