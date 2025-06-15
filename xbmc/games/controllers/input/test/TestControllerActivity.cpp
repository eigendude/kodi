/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/controllers/input/ControllerActivity.h"

#include <gtest/gtest.h>
#include <thread>
#include <chrono>

using namespace KODI;
using namespace GAME;
using namespace std::chrono_literals;

//
// Spec: No activation should occur when the mouse doesn't move
//
TEST(TestControllerActivity, NoMotionNoActivation)
{
  CControllerActivity activity;

  activity.OnMouseMotion("pointer", 0, 0);
  activity.OnInputFrame();

  EXPECT_FLOAT_EQ(activity.GetActivation(), 0.0f);
}

//
// Spec: Mouse movement should activate the controller for the current frame
//
TEST(TestControllerActivity, MotionActivates)
{
  CControllerActivity activity;

  activity.OnMouseMotion("pointer", 10, -5);
  activity.OnInputFrame();

  EXPECT_FLOAT_EQ(activity.GetActivation(), 1.0f);
}

//
// Spec: Activation from motion should persist until the timeout expires
//
TEST(TestControllerActivity, MotionPersistsUntilTimeout)
{
  CControllerActivity activity;

  activity.OnMouseMotion("pointer", 1, 1);
  activity.OnInputFrame();
  EXPECT_FLOAT_EQ(activity.GetActivation(), 1.0f);

  // Timer is 200ms, so after 100ms activation should still continue
  std::this_thread::sleep_for(100ms);
  activity.OnInputFrame();
  EXPECT_FLOAT_EQ(activity.GetActivation(), 1.0f);

  // After waiting beyond the timeout, activation should stop
  std::this_thread::sleep_for(250ms);
  activity.OnInputFrame();
  EXPECT_FLOAT_EQ(activity.GetActivation(), 0.0f);
}

