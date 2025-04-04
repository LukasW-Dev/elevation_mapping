/*
 *  Input.cpp
 *
 *  Created on: Oct 06, 2020
 *  Author: Magnus Gärtner
 *  Institute: ETH Zurich, ANYbotics
 */

#include <memory>

#include "elevation_mapping/input_sources/Input.hpp"

#include "elevation_mapping/sensor_processors/LaserSensorProcessor.hpp"
#include "elevation_mapping/sensor_processors/PerfectSensorProcessor.hpp"
#include "elevation_mapping/sensor_processors/StereoSensorProcessor.hpp"
#include "elevation_mapping/sensor_processors/StructuredLightSensorProcessor.hpp"

namespace elevation_mapping {

Input::Input(rclcpp::Node::SharedPtr node) : node_(node) {}

bool Input::configure(std::string name, const std::string& inputSourcesNamespace,
                 const SensorProcessorBase::GeneralParameters& generalSensorProcessorParameters) {
  // Configuration Guards.
  Parameters parameters;

  parameters.isEnabled_ = node_->declare_parameter<bool>(inputSourcesNamespace + "." + name + ".enabled",
                                                         true);

  node_->declare_parameter<std::string>(inputSourcesNamespace + "." + name + ".type");
  node_->declare_parameter<std::string>(inputSourcesNamespace + "." + name + ".topic");
  node_->declare_parameter<int>(inputSourcesNamespace + "." + name + ".queue_size");
  node_->declare_parameter<bool>(inputSourcesNamespace + "." + name + ".publish_on_update");

  for (const std::string& param : std::vector<std::string>{"type", "topic", "queue_size", "publish_on_update"}){
    if (!node_->has_parameter(inputSourcesNamespace + "." + name + "." + param)){
      RCLCPP_ERROR(node_->get_logger(), 
          "Could not configure input source %s because member %s has the "
          "is not set.",
          name.c_str(), param.c_str());
      return false;
    }
  }

  parameters.name_ = name;
  parameters.type_ = node_->get_parameter(inputSourcesNamespace + "." + name + ".type").as_string();
  parameters.topic_ = node_->get_parameter(inputSourcesNamespace + "." + name + ".topic").as_string();
  const int queueSize = node_->get_parameter(inputSourcesNamespace + "." + name + ".queue_size").as_int();
  if (queueSize >= 0) {
    parameters.queueSize_ = queueSize;
  } else {
    RCLCPP_ERROR(node_->get_logger(), "The specified queue_size is negative.");
    return false;
  }
  parameters.publishOnUpdate_ = node_->get_parameter(inputSourcesNamespace + "." + name + ".publish_on_update").as_bool();

  parameters_.setData(parameters);

  // SensorProcessor
  if (!configureSensorProcessor(generalSensorProcessorParameters)) {
    return false;
  }

  RCLCPP_DEBUG(node_->get_logger(), "Configured %s:%s @ %s/%s (publishing_on_update: %s), using %s to process data.\n", parameters.type_.c_str(),
            parameters.name_.c_str(), node_->get_fully_qualified_name(), parameters.topic_.c_str(), parameters.publishOnUpdate_ ? "true" : "false",
            parameters.type_.c_str());
  return true;
}

std::string Input::getSubscribedTopic() const {
  const Parameters parameters{parameters_.getData()};
  return std::string(node_->get_fully_qualified_name()) + "/" + parameters.topic_;
}

bool Input::configureSensorProcessor(const SensorProcessorBase::GeneralParameters& generalSensorProcessorParameters) {
  std::string sensorType = parameters_.getData().type_;
  if ( sensorType == "structured_light") {
    sensorProcessor_ = std::make_unique<StructuredLightSensorProcessor>(node_, generalSensorProcessorParameters);
  } else if (sensorType == "stereo") {
    sensorProcessor_ = std::make_unique<StereoSensorProcessor>(node_, generalSensorProcessorParameters);
  } else if (sensorType == "laser") {
    sensorProcessor_ = std::make_unique<LaserSensorProcessor>(node_, generalSensorProcessorParameters);
  } else if (sensorType == "perfect") {
    sensorProcessor_ = std::make_unique<PerfectSensorProcessor>(node_, generalSensorProcessorParameters);
  } else {
    RCLCPP_ERROR(node_->get_logger(), "The sensor type %s is not available.", sensorType.c_str());
    return false;
  }

  return sensorProcessor_->readParameters();
}

}  // namespace elevation_mapping
