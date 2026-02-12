
/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2VehicleSubscriber.h"

#include "utils/log.h"

#include <cmath>

#include <rclcpp/rclcpp.hpp>

using namespace KODI;
using namespace SMART_HOME;
using std::placeholders::_1;

namespace
{
constexpr const char* SUBSCRIBE_FORWARD_TWIST_TOPIC = "forward_twist";
constexpr const char* SUBSCRIBE_TILT_TOPIC = "tilt";
} // namespace

CRos2VehicleSubscriber::CRos2VehicleSubscriber(std::string rosNamespace)
  : m_rosNamespace(std::move(rosNamespace))
{
}

void CRos2VehicleSubscriber::Initialize(std::shared_ptr<rclcpp::Node> node,
                                        const std::string& vehicleName)
{
  // Calculate topic names
  const std::string subscribeForwardTwistTopic =
      std::string("/") + m_rosNamespace + "/" + vehicleName + "/" + SUBSCRIBE_FORWARD_TWIST_TOPIC;

  // Initialize ROS
  CLog::Log(LOGDEBUG, "ROS2: Subscribing to {}", subscribeForwardTwistTopic);

  // Subscribers
  m_forwardTwistSubscriber = node->create_subscription<TwistWithCovarianceStamped>(
      subscribeForwardTwistTopic, rclcpp::SensorDataQoS{},
      std::bind(&CRos2VehicleSubscriber::OnForwardTwist, this, _1));
  m_tiltSubscriber = node->create_subscription<TwistWithCovarianceStamped>(
      subscribeForwardTwistTopic, rclcpp::SensorDataQoS{},
      std::bind(&CRos2VehicleSubscriber::OnTilt, this, _1));
}

void CRos2VehicleSubscriber::Deinitialize()
{
  m_forwardTwistSubscriber.reset();
}

float CRos2VehicleSubscriber::ForwardSpeed() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_forwardSpeed;
}

float CRos2VehicleSubscriber::ForwardSpeedStdDev() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_forwardSpeedStdDev;
}

float CRos2VehicleSubscriber::Tilt() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_tilt;
}

float CRos2VehicleSubscriber::TiltStdDev() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_tiltStdDev;
}

void CRos2VehicleSubscriber::OnForwardTwist(const TwistWithCovarianceStamped::SharedPtr msg)
{
  if (!msg)
    return;

  const double velocity = msg->twist.twist.linear.x;

  // Covariance is a 6x6 row-major matrix for twist:
  // indices: 0..5 linear (x,y,z) then angular (x,y,z)
  // variance of linear.x is covariance[0]
  double variance = 0.0;
  if (msg->twist.covariance.size() >= 1)
    variance = msg->twist.covariance[0];

  float speed = 0.0f;
  float stddev = 0.0f;

  // Validate speed
  if (std::isfinite(velocity))
    speed = static_cast<float>(velocity);

  // Validate variance and compute std dev
  if (std::isfinite(variance) && variance >= 0.0)
    stddev = static_cast<float>(std::sqrt(variance));
  else
    stddev = 0.0f; // unknown/invalid covariance

  std::lock_guard<std::mutex> lock(m_mutex);

  m_forwardSpeed = speed;
  m_forwardSpeedStdDev = stddev;
}

void CRos2VehicleSubscriber::OnTilt(const Imu::SharedPtr msg)
{
  if (!msg)
    return;

  //! @todo

  std::lock_guard<std::mutex> lock(m_mutex);

  /*
  m_tilt = tilt;
  m_tiltStdDev = stddev;
  */
}
