
/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2VehicleSubscriber.h"

#include "utils/log.h"

#include <algorithm>
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
  const std::string subscribeTiltTopic =
      std::string("/") + m_rosNamespace + "/" + vehicleName + "/" + SUBSCRIBE_TILT_TOPIC;

  // Initialize ROS
  CLog::Log(LOGDEBUG, "ROS2: Subscribing to {}", subscribeForwardTwistTopic);
  CLog::Log(LOGDEBUG, "ROS2: Subscribing to {}", subscribeTiltTopic);

  // Subscribers
  m_forwardTwistSubscriber = node->create_subscription<TwistWithCovarianceStamped>(
      subscribeForwardTwistTopic, rclcpp::SensorDataQoS{},
      std::bind(&CRos2VehicleSubscriber::OnForwardTwist, this, _1));
  m_tiltSubscriber = node->create_subscription<Imu>(
      subscribeTiltTopic, rclcpp::SensorDataQoS{},
      std::bind(&CRos2VehicleSubscriber::OnTilt, this, _1));
}

void CRos2VehicleSubscriber::Deinitialize()
{
  m_forwardTwistSubscriber.reset();
  m_tiltSubscriber.reset();
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

  constexpr double RAD_TO_DEG = 180.0 / std::acos(-1.0);

  // Tilt is the angular deviation from gravity (vehicle z-axis against world z-axis)
  // derived from the orientation quaternion.
  const double qx = msg->orientation.x;
  const double qy = msg->orientation.y;
  const double qz = msg->orientation.z;
  const double qw = msg->orientation.w;

  float tilt = 0.0f;
  float stddev = 0.0f;

  const double norm = qx * qx + qy * qy + qz * qz + qw * qw;
  if (std::isfinite(norm) && norm > 0.0)
  {
    const double invNorm = 1.0 / std::sqrt(norm);
    const double nx = qx * invNorm;
    const double ny = qy * invNorm;
    const double nz = qz * invNorm;

    // R(2,2) for quaternion -> rotation matrix.
    const double r33 = std::clamp(1.0 - 2.0 * (nx * nx + ny * ny), -1.0, 1.0);
    const double tiltRad = std::acos(r33);

    if (std::isfinite(tiltRad))
      tilt = static_cast<float>(tiltRad * RAD_TO_DEG);
  }

  // If covariance is present for orientation (not all zeros, and not unknown sentinel),
  // estimate a tilt variance from roll/pitch variances.
  if (msg->orientation_covariance.size() >= 5 && msg->orientation_covariance[0] >= 0.0)
  {
    const double rollVariance = msg->orientation_covariance[0];
    const double pitchVariance = msg->orientation_covariance[4];
    const double tiltVariance = rollVariance + pitchVariance;

    if (std::isfinite(tiltVariance) && tiltVariance >= 0.0)
      stddev = static_cast<float>(std::sqrt(tiltVariance) * RAD_TO_DEG);
  }

  std::lock_guard<std::mutex> lock(m_mutex);

  m_tilt = tilt;
  m_tiltStdDev = stddev;
}
