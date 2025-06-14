/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/controllers/input/ControllerState.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

//
// Spec: Should store and return controller state for all feature types
//
TEST(TestControllerState, SetAndGet)
{
  CControllerState state;

  state.SetDigitalButton("a", true);
  state.SetAnalogButton("b", 0.5f);
  state.SetAnalogStick("stick", {1.0f, -1.0f});
  state.SetAccelerometer("accel", {0.1f, 0.2f, 0.3f});
  state.SetThrottle("thr", 0.8f);
  state.SetWheel("wheel", -0.25f);

  EXPECT_TRUE(state.GetDigitalButton("a"));
  EXPECT_FLOAT_EQ(state.GetAnalogButton("b"), 0.5f);
  EXPECT_EQ(state.GetAnalogStick("stick"), (CControllerState::AnalogStick{1.0f, -1.0f}));
  EXPECT_EQ(state.GetAccelerometer("accel"), (CControllerState::Accelerometer{0.1f, 0.2f, 0.3f}));
  EXPECT_FLOAT_EQ(state.GetThrottle("thr"), 0.8f);
  EXPECT_FLOAT_EQ(state.GetWheel("wheel"), -0.25f);
}

//
// Spec: States with identical values should compare equal
//
TEST(TestControllerState, EqualityOperator)
{
  CControllerState state1;
  state1.SetDigitalButton("btn", true);
  state1.SetAnalogButton("analog", 1.0f);
  state1.SetAnalogStick("stick", {0.1f, 0.2f});
  state1.SetAccelerometer("accel", {9.8f, 0.0f, 0.0f});
  state1.SetThrottle("thr", 0.75f);
  state1.SetWheel("wheel", -0.25f);

  CControllerState state2(state1);
  EXPECT_EQ(state1, state2);
  EXPECT_FALSE(state1 != state2);
}

//
// Spec: Unknown features should return default values
//
TEST(TestControllerState, DefaultValues)
{
  CControllerState state;

  EXPECT_FALSE(state.GetDigitalButton("missing"));
  EXPECT_FLOAT_EQ(state.GetAnalogButton("missing"), 0.0f);
  EXPECT_EQ(state.GetAnalogStick("missing"), (CControllerState::AnalogStick{}));
  EXPECT_EQ(state.GetAccelerometer("missing"), (CControllerState::Accelerometer{}));
  EXPECT_FLOAT_EQ(state.GetThrottle("missing"), 0.0f);
  EXPECT_FLOAT_EQ(state.GetWheel("missing"), 0.0f);
}

//
// Spec: States with different values should not compare equal
//
TEST(TestControllerState, Inequality)
{
  CControllerState state1;
  state1.SetDigitalButton("btn", true);

  CControllerState state2(state1);
  state2.SetDigitalButton("btn", false);

  EXPECT_TRUE(state1 != state2);
}

//
// Spec: Should handle multiple inputs across all feature types
//
TEST(TestControllerState, MultipleInputs)
{
  CControllerState state;

  // Two digital buttons
  state.SetDigitalButton("a", true);
  state.SetDigitalButton("b", false);

  // Two analog buttons
  state.SetAnalogButton("c", -0.5f);
  state.SetAnalogButton("d", 0.25f);

  // Two analog sticks
  state.SetAnalogStick("stick1", {0.1f, 0.2f});
  state.SetAnalogStick("stick2", {-1.0f, 1.0f});

  // Two accelerometers
  state.SetAccelerometer("accel1", {1.0f, 2.0f, 3.0f});
  state.SetAccelerometer("accel2", {-1.0f, -2.0f, -3.0f});

  // Additional throttle and wheel
  state.SetThrottle("thr1", 0.5f);
  state.SetWheel("wheel1", 0.75f);

  EXPECT_TRUE(state.GetDigitalButton("a"));
  EXPECT_FALSE(state.GetDigitalButton("b"));
  EXPECT_FLOAT_EQ(state.GetAnalogButton("c"), -0.5f);
  EXPECT_FLOAT_EQ(state.GetAnalogButton("d"), 0.25f);
  EXPECT_EQ(state.GetAnalogStick("stick1"), (CControllerState::AnalogStick{0.1f, 0.2f}));
  EXPECT_EQ(state.GetAnalogStick("stick2"), (CControllerState::AnalogStick{-1.0f, 1.0f}));
  EXPECT_EQ(state.GetAccelerometer("accel1"), (CControllerState::Accelerometer{1.0f, 2.0f, 3.0f}));
  EXPECT_EQ(state.GetAccelerometer("accel2"),
            (CControllerState::Accelerometer{-1.0f, -2.0f, -3.0f}));
  EXPECT_FLOAT_EQ(state.GetThrottle("thr1"), 0.5f);
  EXPECT_FLOAT_EQ(state.GetWheel("wheel1"), 0.75f);

  EXPECT_EQ(state.DigitalButtons().size(), 2u);
  EXPECT_EQ(state.AnalogButtons().size(), 2u);
  EXPECT_EQ(state.AnalogSticks().size(), 2u);
  EXPECT_EQ(state.Accelerometers().size(), 2u);
  EXPECT_EQ(state.Throttles().size(), 1u);
  EXPECT_EQ(state.Wheels().size(), 1u);
}

//
// Spec: Latest value should overwrite previous state
//
TEST(TestControllerState, OverwriteValues)
{
  CControllerState state;

  state.SetDigitalButton("btn", true);
  state.SetDigitalButton("btn", false);
  EXPECT_FALSE(state.GetDigitalButton("btn"));

  state.SetAnalogButton("analog", 0.0f);
  state.SetAnalogButton("analog", 1.0f);
  EXPECT_FLOAT_EQ(state.GetAnalogButton("analog"), 1.0f);

  state.SetAnalogStick("stick", {0.0f, 0.0f});
  state.SetAnalogStick("stick", {0.5f, -0.5f});
  EXPECT_EQ(state.GetAnalogStick("stick"), (CControllerState::AnalogStick{0.5f, -0.5f}));

  state.SetAccelerometer("accel", {0.0f, 0.0f, 0.0f});
  state.SetAccelerometer("accel", {1.0f, 1.0f, 1.0f});
  EXPECT_EQ(state.GetAccelerometer("accel"), (CControllerState::Accelerometer{1.0f, 1.0f, 1.0f}));

  state.SetThrottle("thr", 0.0f);
  state.SetThrottle("thr", 0.9f);
  EXPECT_FLOAT_EQ(state.GetThrottle("thr"), 0.9f);

  state.SetWheel("wheel", 0.0f);
  state.SetWheel("wheel", -0.25f);
  EXPECT_FLOAT_EQ(state.GetWheel("wheel"), -0.25f);
}
