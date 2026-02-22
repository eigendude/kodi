/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/DmaBufImportProber.h"

#include "utils/IBufferObject.h"

#include <gtest/gtest.h>

#include <vector>

namespace
{
class CMockBufferObject : public IBufferObject
{
public:
  explicit CMockBufferObject(uint32_t stride) : m_stride(stride) {}

  bool CreateBufferObject(uint32_t, uint32_t, uint32_t) override { return true; }
  bool CreateBufferObject(uint64_t) override { return true; }
  void DestroyBufferObject() override {}
  uint8_t* GetMemory() override { return nullptr; }
  void ReleaseMemory() override {}
  int GetFd() override { return 42; }
  uint32_t GetStride() override { return m_stride; }
  uint64_t GetModifier() override { return DRM_FORMAT_MOD_LINEAR; }
  void SyncStart() override {}
  void SyncEnd() override {}
  std::string GetName() const override { return "mock"; }

private:
  uint32_t m_stride{0};
};

EGLImageKHR FakeCreateImage(EGLDisplay,
                            EGLContext,
                            EGLenum,
                            EGLClientBuffer,
                            const EGLint*)
{
  static int callCount = 0;
  ++callCount;

  // First two imports fail (GBM with modifier, then GBM implicit), third succeeds (DMA-heap implicit).
  return callCount >= 3 ? reinterpret_cast<EGLImageKHR>(0x1) : nullptr;
}

EGLBoolean FakeDestroyImage(EGLDisplay, EGLImageKHR)
{
  return EGL_TRUE;
}

} // namespace

// Verifies the deterministic retry order and selection of the first successful non-copy path.
TEST(TestDmaBufImportProber, RetrySequenceSelectsDMAHeapImplicit)
{
  CDmaBufImportProber::EGLFunctions eglFunctions{};
  eglFunctions.eglCreateImageKHR = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(FakeCreateImage);
  eglFunctions.eglDestroyImageKHR = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(FakeDestroyImage);

  CDmaBufImportProber prober(EGL_NO_DISPLAY, eglFunctions);
  prober.SetCapabilityCacheForTest({DRM_FORMAT_RGB565},
                                   {{DRM_FORMAT_RGB565,
                                     {{DRM_FORMAT_MOD_LINEAR, false}}}},
                                   true);

  std::vector<CDmaBufImportProber::AllocatorType> attempts;

  auto result = prober.ProbeAndAdapt(
      {.gpu = "testgpu", .drmFourcc = DRM_FORMAT_RGB565, .widthBucket = 512, .heightBucket = 512},
      320, 240, DRM_FORMAT_RGB565,
      [&](CDmaBufImportProber::AllocatorType allocator,
          uint32_t,
          uint32_t,
          uint32_t,
          uint64_t,
          uint32_t) -> std::unique_ptr<IBufferObject> {
        attempts.push_back(allocator);
        return std::make_unique<CMockBufferObject>(640);
      },
      true);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.allocator, CDmaBufImportProber::AllocatorType::DMAHeapImplicit);
  ASSERT_GE(attempts.size(), 3U);
  EXPECT_EQ(attempts[0], CDmaBufImportProber::AllocatorType::GBMModifiers);
  EXPECT_EQ(attempts[1], CDmaBufImportProber::AllocatorType::GBMImplicit);
  EXPECT_EQ(attempts[2], CDmaBufImportProber::AllocatorType::DMAHeapImplicit);
}
