/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "input/joysticks/interfaces/IButtonMap.h"
#include "input/mouse/generic/MouseInputHandling.h"
#include "input/mouse/interfaces/IMouseInputHandler.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace MOUSE;
using namespace JOYSTICK;

// Test stub capturing mouse events
class TestMouseHandler : public IMouseInputHandler
{
public:
  std::string ControllerID() const override { return "test.controller"; }

  bool OnPosition(const PointerName& relpointer, int distanceX, int distanceY) override
  {
    calledPosition = true;
    lastRelPointer = relpointer;
    lastDistanceX = distanceX;
    lastDistanceY = distanceY;
    return true;
  }

  bool OnMotion(const PointerName& relpointer, int differenceX, int differenceY) override
  {
    calledMotion = true;
    lastRelPointerMotion = relpointer;
    lastDifferenceX = differenceX;
    lastDifferenceY = differenceY;
    return true;
  }

  bool OnButtonPress(const ButtonName& button) override
  {
    calledButtonPress = true;
    lastButtonPressed = button;
    return true;
  }

  void OnButtonRelease(const ButtonName& button) override
  {
    calledButtonRelease = true;
    lastButtonReleased = button;
  }
  void OnInputFrame() override { ++inputFrameCount; }

  bool calledPosition{false};
  bool calledMotion{false};
  unsigned int inputFrameCount{0};
  std::string lastRelPointer;
  std::string lastRelPointerMotion;
  int lastDistanceX{0};
  int lastDistanceY{0};
  int lastDifferenceX{0};
  int lastDifferenceY{0};
  bool calledButtonPress{false};
  bool calledButtonRelease{false};
  std::string lastButtonPressed;
  std::string lastButtonReleased;
};

// Minimal button map returning a single pointer feature
class TestButtonMap : public IButtonMap
{
public:
  std::string ControllerID() const override { return "test.controller"; }
  std::string Location() const override { return {}; }
  bool Load() override { return true; }
  void Reset() override {}
  bool IsEmpty() const override { return false; }
  std::string GetAppearance() const override { return {}; }
  bool SetAppearance(const std::string&) const override { return false; }

  bool GetFeature(const CDriverPrimitive& primitive, FeatureName& feature) override
  {
    if (primitive.Type() == PRIMITIVE_TYPE::RELATIVE_POINTER &&
        primitive.PointerDirection() == RELATIVE_POINTER_DIRECTION::RIGHT)
    {
      feature = "pointer";
      return true;
    }

    if (primitive.Type() == PRIMITIVE_TYPE::BUTTON && primitive.MouseButton() == BUTTON_ID::LEFT)
    {
      feature = "left_button";
      return true;
    }

    return false;
  }

  FEATURE_TYPE GetFeatureType(const FeatureName&) override { return FEATURE_TYPE::RELPOINTER; }

  bool GetScalar(const FeatureName&, CDriverPrimitive&) override { return false; }
  void AddScalar(const FeatureName&, const CDriverPrimitive&) override {}
  bool GetAnalogStick(const FeatureName&, ANALOG_STICK_DIRECTION, CDriverPrimitive&) override
  {
    return false;
  }
  void AddAnalogStick(const FeatureName&, ANALOG_STICK_DIRECTION, const CDriverPrimitive&) override
  {
  }

  bool GetRelativePointer(const FeatureName& feature,
                          RELATIVE_POINTER_DIRECTION direction,
                          CDriverPrimitive& primitive) override
  {
    if (feature == "pointer")
    {
      primitive = CDriverPrimitive(direction);
      return true;
    }
    return false;
  }

  void AddRelativePointer(const FeatureName&,
                          RELATIVE_POINTER_DIRECTION,
                          const CDriverPrimitive&) override
  {
  }
  bool GetAccelerometer(const FeatureName&,
                        CDriverPrimitive&,
                        CDriverPrimitive&,
                        CDriverPrimitive&) override
  {
    return false;
  }
  void AddAccelerometer(const FeatureName&,
                        const CDriverPrimitive&,
                        const CDriverPrimitive&,
                        const CDriverPrimitive&) override
  {
  }
  bool GetWheel(const FeatureName&, WHEEL_DIRECTION, CDriverPrimitive&) override { return false; }
  void AddWheel(const FeatureName&, WHEEL_DIRECTION, const CDriverPrimitive&) override {}
  bool GetThrottle(const FeatureName&, THROTTLE_DIRECTION, CDriverPrimitive&) override
  {
    return false;
  }
  void AddThrottle(const FeatureName&, THROTTLE_DIRECTION, const CDriverPrimitive&) override {}
  bool GetKey(const FeatureName&, CDriverPrimitive&) override { return false; }
  void AddKey(const FeatureName&, const CDriverPrimitive&) override {}
  void SetIgnoredPrimitives(const std::vector<CDriverPrimitive>&) override {}
  bool IsIgnored(const CDriverPrimitive&) override { return false; }
  bool GetAxisProperties(unsigned int, int&, unsigned int&) override { return false; }
  void SaveButtonMap() override {}
  void RevertButtonMap() override {}
};

// When the first coordinates arrive the handler should just record the
// position. No callbacks or frame updates should occur before the first
// position initializes the handling logic.
TEST(TestMouseInputHandling, FirstPositionInitializes)
{
  TestMouseHandler handler;
  TestButtonMap buttonMap;
  CMouseInputHandling handling(&handler, &buttonMap);

  bool handled = handling.OnPosition(5, 3);

  EXPECT_TRUE(handled);
  EXPECT_FALSE(handler.calledPosition);
  EXPECT_FALSE(handler.calledMotion);
  EXPECT_EQ(handler.inputFrameCount, 0u);
}

// Once initialized, subsequent position updates should dispatch both absolute
// and relative motion events and advance the frame counter.
TEST(TestMouseInputHandling, PositionDispatchesEvents)
{
  TestMouseHandler handler;
  TestButtonMap buttonMap;
  CMouseInputHandling handling(&handler, &buttonMap);

  handling.OnPosition(0, 0); // initialize
  handler.calledPosition = false;
  handler.calledMotion = false;
  handler.inputFrameCount = 0;

  bool handled = handling.OnPosition(10, 0);

  EXPECT_TRUE(handled);
  EXPECT_TRUE(handler.calledPosition);
  EXPECT_TRUE(handler.calledMotion);
  EXPECT_EQ(handler.lastRelPointer, "pointer");
  EXPECT_EQ(handler.lastDistanceX, 10);
  EXPECT_EQ(handler.lastDistanceY, 0);
  EXPECT_EQ(handler.lastRelPointerMotion, "pointer");
  EXPECT_EQ(handler.lastDifferenceX, 10);
  EXPECT_EQ(handler.lastDifferenceY, 0);
  EXPECT_EQ(handler.inputFrameCount, 1u);
}

// Pressing a mapped button should trigger the handler callback and also update
// the frame counter.
TEST(TestMouseInputHandling, ButtonPressDispatchesEvent)
{
  TestMouseHandler handler;
  TestButtonMap buttonMap;
  CMouseInputHandling handling(&handler, &buttonMap);

  // Start with a clean frame count so we can verify the increment
  handler.inputFrameCount = 0;

  // Simulate pressing the mapped left button
  bool handled = handling.OnButtonPress(BUTTON_ID::LEFT);

  // The handler should be notified and the frame counter advanced
  EXPECT_TRUE(handled);
  EXPECT_TRUE(handler.calledButtonPress);
  EXPECT_EQ(handler.lastButtonPressed, "left_button");
  EXPECT_EQ(handler.inputFrameCount, 1u);
}

// Releasing a mapped button should forward the release event and advance the
// frame counter.
TEST(TestMouseInputHandling, ButtonReleaseDispatchesEvent)
{
  TestMouseHandler handler;
  TestButtonMap buttonMap;
  CMouseInputHandling handling(&handler, &buttonMap);

  // Clear frame count to verify that release triggers a new frame
  handler.inputFrameCount = 0;

  // Send the release event for the mapped left button
  handling.OnButtonRelease(BUTTON_ID::LEFT);

  // The handler should be notified and the frame counter advanced
  EXPECT_TRUE(handler.calledButtonRelease);
  EXPECT_EQ(handler.lastButtonReleased, "left_button");
  EXPECT_EQ(handler.inputFrameCount, 1u);
}

// An unmapped button should return false but still advance the frame counter
TEST(TestMouseInputHandling, UnmappedButtonStillUpdatesFrame)
{
  TestMouseHandler handler;
  TestButtonMap buttonMap;
  CMouseInputHandling handling(&handler, &buttonMap);

  // Only the left button is mapped in our stub. Press right to simulate unmapped input
  handler.inputFrameCount = 0;

  bool handled = handling.OnButtonPress(BUTTON_ID::RIGHT);

  // The event is not handled by the handler but the frame should still update
  EXPECT_FALSE(handled);
  EXPECT_FALSE(handler.calledButtonPress);
  EXPECT_EQ(handler.inputFrameCount, 1u);
}

// Receiving the same position twice should not dispatch motion events but
// should still produce a new input frame
TEST(TestMouseInputHandling, DuplicatePositionOnlyUpdatesFrame)
{
  TestMouseHandler handler;
  TestButtonMap buttonMap;
  CMouseInputHandling handling(&handler, &buttonMap);

  // Initialize position
  handling.OnPosition(4, 4);
  handler.calledPosition = false;
  handler.calledMotion = false;
  handler.inputFrameCount = 0;

  bool handled = handling.OnPosition(4, 4);

  // No callbacks occur but the call returns handled and frame is advanced
  EXPECT_TRUE(handled);
  EXPECT_FALSE(handler.calledPosition);
  EXPECT_FALSE(handler.calledMotion);
  EXPECT_EQ(handler.inputFrameCount, 1u);
}
