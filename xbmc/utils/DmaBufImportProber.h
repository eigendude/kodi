/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "system_egl.h"

#include <array>

#include <drm_fourcc.h>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

class IBufferObject;

class CDmaBufImportProber
{
public:
  static constexpr int MAX_PLANES{3};

  enum class AllocatorType
  {
    GBMModifiers,
    GBMImplicit,
    DMAHeapImplicit,
    DMAHeapAligned,
  };

  struct DmaBufPlane
  {
    int fd{-1};
    uint32_t offset{0};
    uint32_t pitch{0};
  };

  struct DmaBufDesc
  {
    uint32_t width{0};
    uint32_t height{0};
    std::array<DmaBufPlane, MAX_PLANES> planes;
  };

  struct ModifierInfo
  {
    uint64_t modifier{DRM_FORMAT_MOD_INVALID};
    bool externalOnly{false};
  };

  struct EGLFunctions
  {
    PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR{nullptr};
    PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR{nullptr};
    PFNEGLQUERYDMABUFFORMATSEXTPROC eglQueryDmaBufFormatsEXT{nullptr};
    PFNEGLQUERYDMABUFMODIFIERSEXTPROC eglQueryDmaBufModifiersEXT{nullptr};
  };

  struct ProbeResult
  {
    bool success{false};
    AllocatorType allocator{AllocatorType::DMAHeapImplicit};
    uint64_t modifier{DRM_FORMAT_MOD_INVALID};
    bool explicitModifier{false};
    uint32_t strideAlignment{0};
    std::string error;
  };

  struct ProbeKey
  {
    std::string gpu;
    uint32_t drmFourcc{0};
    uint32_t widthBucket{0};
    uint32_t heightBucket{0};

    bool operator<(const ProbeKey& other) const;
  };

  using AllocateFn = std::function<std::unique_ptr<IBufferObject>(AllocatorType,
                                                                   uint32_t drmFourcc,
                                                                   uint32_t width,
                                                                   uint32_t height,
                                                                   uint64_t modifier,
                                                                   uint32_t strideAlignment)>;

  explicit CDmaBufImportProber(EGLDisplay display);
  CDmaBufImportProber(EGLDisplay display, EGLFunctions eglFunctions);

  void SetDisplay(EGLDisplay display);

  bool TryImportDmaBufAsEGLImage(const DmaBufDesc& desc,
                                 uint32_t drmFourcc,
                                 uint64_t modifierOrInvalid,
                                 EGLImageKHR* outImage,
                                 std::string* outError) const;

  ProbeResult ProbeAndAdapt(const ProbeKey& key,
                            uint32_t width,
                            uint32_t height,
                            uint32_t drmFourcc,
                            const AllocateFn& allocateFn,
                            bool preferLinear);

  void Invalidate(const ProbeKey& key);

  void SetCapabilityCacheForTest(const std::vector<uint32_t>& formats,
                                 const std::map<uint32_t, std::vector<ModifierInfo>>& modifiers,
                                 bool hasModifiersExtension);

private:
  struct CapabilityCache
  {
    bool queried{false};
    bool hasModifiersExt{false};
    std::vector<uint32_t> formats;
    std::map<uint32_t, std::vector<ModifierInfo>> modifiers;
  };

  void RefreshCapabilitiesLocked();
  std::vector<uint64_t> BuildModifierPriority(uint32_t drmFourcc, bool preferLinear) const;
  std::optional<ProbeResult> FindKnownGoodLocked(const ProbeKey& key) const;
  void StoreKnownGoodLocked(const ProbeKey& key, const ProbeResult& result);

  EGLDisplay m_display{EGL_NO_DISPLAY};
  PFNEGLCREATEIMAGEKHRPROC m_eglCreateImageKHR{nullptr};
  PFNEGLDESTROYIMAGEKHRPROC m_eglDestroyImageKHR{nullptr};

  PFNEGLQUERYDMABUFFORMATSEXTPROC m_eglQueryDmaBufFormatsEXT{nullptr};
  PFNEGLQUERYDMABUFMODIFIERSEXTPROC m_eglQueryDmaBufModifiersEXT{nullptr};

  mutable std::mutex m_mutex;
  CapabilityCache m_caps;
  std::map<ProbeKey, ProbeResult> m_knownGood;
};

