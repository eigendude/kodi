/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DmaBufImportProber.h"

#include "ServiceBroker.h"
#include "utils/BufferObject.h"
#include "utils/DRMHelpers.h"
#include "utils/EGLUtils.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <sstream>
#include <utility>

namespace
{
constexpr std::array<EGLint, CDmaBufImportProber::MAX_PLANES> EGL_PLANE_FD = {
    EGL_DMA_BUF_PLANE0_FD_EXT,
    EGL_DMA_BUF_PLANE1_FD_EXT,
    EGL_DMA_BUF_PLANE2_FD_EXT,
};

constexpr std::array<EGLint, CDmaBufImportProber::MAX_PLANES> EGL_PLANE_OFFSET = {
    EGL_DMA_BUF_PLANE0_OFFSET_EXT,
    EGL_DMA_BUF_PLANE1_OFFSET_EXT,
    EGL_DMA_BUF_PLANE2_OFFSET_EXT,
};

constexpr std::array<EGLint, CDmaBufImportProber::MAX_PLANES> EGL_PLANE_PITCH = {
    EGL_DMA_BUF_PLANE0_PITCH_EXT,
    EGL_DMA_BUF_PLANE1_PITCH_EXT,
    EGL_DMA_BUF_PLANE2_PITCH_EXT,
};

#if defined(EGL_EXT_image_dma_buf_import_modifiers)
constexpr std::array<EGLint, CDmaBufImportProber::MAX_PLANES> EGL_PLANE_MOD_LO = {
    EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT,
    EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT,
    EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT,
};

constexpr std::array<EGLint, CDmaBufImportProber::MAX_PLANES> EGL_PLANE_MOD_HI = {
    EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT,
    EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT,
    EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT,
};
#endif

constexpr uint32_t STRIDE_ALIGNMENT_CONSERVATIVE{256};

std::string AllocatorTypeToString(CDmaBufImportProber::AllocatorType type)
{
  switch (type)
  {
    case CDmaBufImportProber::AllocatorType::GBMModifiers:
      return "GBM(modifiers)";
    case CDmaBufImportProber::AllocatorType::GBMImplicit:
      return "GBM(implicit)";
    case CDmaBufImportProber::AllocatorType::DMAHeapImplicit:
      return "DMA-heap(implicit)";
    case CDmaBufImportProber::AllocatorType::DMAHeapAligned:
      return "DMA-heap(aligned)";
  }

  return "unknown";
}

CDmaBufImportProber::DmaBufDesc BuildDesc(IBufferObject& bo, uint32_t width, uint32_t height)
{
  CDmaBufImportProber::DmaBufDesc desc;
  desc.width = width;
  desc.height = height;
  desc.planes[0].fd = bo.GetFd();
  desc.planes[0].offset = 0;
  desc.planes[0].pitch = bo.GetStride();
  return desc;
}

} // namespace

bool CDmaBufImportProber::ProbeKey::operator<(const ProbeKey& other) const
{
  return std::tie(gpu, drmFourcc, widthBucket, heightBucket) <
         std::tie(other.gpu, other.drmFourcc, other.widthBucket, other.heightBucket);
}

CDmaBufImportProber::CDmaBufImportProber(EGLDisplay display)
    : CDmaBufImportProber(
          display,
          {CEGLUtils::GetRequiredProcAddress<PFNEGLCREATEIMAGEKHRPROC>("eglCreateImageKHR"),
           CEGLUtils::GetRequiredProcAddress<PFNEGLDESTROYIMAGEKHRPROC>("eglDestroyImageKHR"),
           nullptr,
           nullptr})
{
#if defined(EGL_EXT_image_dma_buf_import_modifiers)
  if (CEGLUtils::HasExtension(display, "EGL_EXT_image_dma_buf_import_modifiers"))
  {
    m_eglQueryDmaBufFormatsEXT =
        CEGLUtils::GetRequiredProcAddress<PFNEGLQUERYDMABUFFORMATSEXTPROC>(
            "eglQueryDmaBufFormatsEXT");
    m_eglQueryDmaBufModifiersEXT =
        CEGLUtils::GetRequiredProcAddress<PFNEGLQUERYDMABUFMODIFIERSEXTPROC>(
            "eglQueryDmaBufModifiersEXT");
  }
#endif
}

CDmaBufImportProber::CDmaBufImportProber(EGLDisplay display, EGLFunctions eglFunctions)
  : m_display(display),
    m_eglCreateImageKHR(eglFunctions.eglCreateImageKHR),
    m_eglDestroyImageKHR(eglFunctions.eglDestroyImageKHR),
    m_eglQueryDmaBufFormatsEXT(eglFunctions.eglQueryDmaBufFormatsEXT),
    m_eglQueryDmaBufModifiersEXT(eglFunctions.eglQueryDmaBufModifiersEXT)
{
}

void CDmaBufImportProber::SetDisplay(EGLDisplay display)
{
  std::scoped_lock lock(m_mutex);

  if (m_display == display)
    return;

  m_display = display;
  m_caps = {};
  m_knownGood.clear();
}

void CDmaBufImportProber::RefreshCapabilitiesLocked()
{
  if (m_caps.queried)
    return;

  m_caps.queried = true;
#if defined(EGL_EXT_image_dma_buf_import_modifiers)
  m_caps.hasModifiersExt = (m_eglQueryDmaBufFormatsEXT != nullptr && m_eglQueryDmaBufModifiersEXT != nullptr);

  if (!m_caps.hasModifiersExt)
    return;

  EGLint count = 0;
  if (m_eglQueryDmaBufFormatsEXT(m_display, 0, nullptr, &count) != EGL_TRUE || count <= 0)
    return;

  std::vector<EGLint> formats(count);
  if (m_eglQueryDmaBufFormatsEXT(m_display, count, formats.data(), &count) != EGL_TRUE)
    return;

  m_caps.formats.reserve(count);
  for (int i = 0; i < count; ++i)
  {
    const uint32_t drmFourcc = static_cast<uint32_t>(formats[i]);
    m_caps.formats.emplace_back(drmFourcc);

    EGLint modifierCount = 0;
    if (m_eglQueryDmaBufModifiersEXT(m_display, formats[i], 0, nullptr, nullptr, &modifierCount) != EGL_TRUE ||
        modifierCount <= 0)
    {
      continue;
    }

    std::vector<EGLuint64KHR> modifiers(modifierCount);
    std::vector<EGLBoolean> externalOnly(modifierCount);
    if (m_eglQueryDmaBufModifiersEXT(m_display, formats[i], modifierCount, modifiers.data(),
                                     externalOnly.data(), &modifierCount) != EGL_TRUE)
    {
      continue;
    }

    auto& list = m_caps.modifiers[drmFourcc];
    list.reserve(modifierCount);
    for (int index = 0; index < modifierCount; ++index)
    {
      list.push_back(ModifierInfo{static_cast<uint64_t>(modifiers[index]),
                                  externalOnly[index] == EGL_TRUE});
    }
  }
#endif
}

std::vector<uint64_t> CDmaBufImportProber::BuildModifierPriority(uint32_t drmFourcc,
                                                                 bool preferLinear) const
{
  std::vector<uint64_t> ordered;

  auto it = m_caps.modifiers.find(drmFourcc);
  if (it == m_caps.modifiers.end())
    return ordered;

  for (const auto& mod : it->second)
  {
    if (!mod.externalOnly)
      ordered.push_back(mod.modifier);
  }

  if (preferLinear)
  {
    auto linearIt = std::ranges::find(ordered, DRM_FORMAT_MOD_LINEAR);
    if (linearIt != ordered.end() && linearIt != ordered.begin())
    {
      ordered.erase(linearIt);
      ordered.insert(ordered.begin(), DRM_FORMAT_MOD_LINEAR);
    }
  }

  return ordered;
}

bool CDmaBufImportProber::TryImportDmaBufAsEGLImage(const DmaBufDesc& desc,
                                                    uint32_t drmFourcc,
                                                    uint64_t modifierOrInvalid,
                                                    EGLImageKHR* outImage,
                                                    std::string* outError) const
{
  if (!m_eglCreateImageKHR || !m_eglDestroyImageKHR)
  {
    if (outError)
      *outError = "missing required EGL image functions";
    return false;
  }

  std::vector<EGLint> attributes = {
      EGL_WIDTH,
      static_cast<EGLint>(desc.width),
      EGL_HEIGHT,
      static_cast<EGLint>(desc.height),
      EGL_LINUX_DRM_FOURCC_EXT,
      static_cast<EGLint>(drmFourcc),
  };

  for (size_t plane = 0; plane < desc.planes.size(); ++plane)
  {
    if (desc.planes[plane].fd < 0)
      continue;

    attributes.insert(attributes.end(),
                      {EGL_PLANE_FD[plane], desc.planes[plane].fd, EGL_PLANE_OFFSET[plane],
                       static_cast<EGLint>(desc.planes[plane].offset), EGL_PLANE_PITCH[plane],
                       static_cast<EGLint>(desc.planes[plane].pitch)});

#if defined(EGL_EXT_image_dma_buf_import_modifiers)
    if (modifierOrInvalid != DRM_FORMAT_MOD_INVALID)
    {
      attributes.insert(attributes.end(), {EGL_PLANE_MOD_LO[plane],
                                           static_cast<EGLint>(modifierOrInvalid & 0xffffffff),
                                           EGL_PLANE_MOD_HI[plane],
                                           static_cast<EGLint>(modifierOrInvalid >> 32)});
    }
#endif
  }

  attributes.push_back(EGL_NONE);

  EGLImageKHR image =
      m_eglCreateImageKHR(m_display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, attributes.data());
  if (!image)
  {
    const EGLint eglError = eglGetError();

    std::ostringstream attrDump;
    for (size_t index = 0; index + 1 < attributes.size() && attributes[index] != EGL_NONE; index += 2)
      attrDump << StringUtils::Format("attr={} value={}\n", attributes[index], attributes[index + 1]);

    CLog::Log(LOGDEBUG,
              "CDmaBufImportProber::{} import failed, fourcc={} modifier={} error={:#4x} attrs:\n{}",
              __FUNCTION__, DRMHELPERS::FourCCToString(drmFourcc),
              DRMHELPERS::ModifierToString(modifierOrInvalid), eglError, attrDump.str());

    if (outError)
      *outError = StringUtils::Format("eglCreateImageKHR failed: {:#4x}", eglError);

    return false;
  }

  if (outImage)
    *outImage = image;
  else
    m_eglDestroyImageKHR(m_display, image);

  return true;
}

std::optional<CDmaBufImportProber::ProbeResult>
CDmaBufImportProber::FindKnownGoodLocked(const ProbeKey& key) const
{
  auto it = m_knownGood.find(key);
  if (it == m_knownGood.end())
    return std::nullopt;

  return it->second;
}

void CDmaBufImportProber::StoreKnownGoodLocked(const ProbeKey& key, const ProbeResult& result)
{
  if (result.success)
    m_knownGood[key] = result;
}

CDmaBufImportProber::ProbeResult CDmaBufImportProber::ProbeAndAdapt(const ProbeKey& key,
                                                                     uint32_t width,
                                                                     uint32_t height,
                                                                     uint32_t drmFourcc,
                                                                     const AllocateFn& allocateFn,
                                                                     bool preferLinear)
{
  ProbeResult result;

  {
    std::scoped_lock lock(m_mutex);
    RefreshCapabilitiesLocked();

    if (auto cached = FindKnownGoodLocked(key); cached.has_value())
      return cached.value();
  }

  const auto tryAllocator = [&](AllocatorType type, uint64_t modifier, bool explicitModifier,
                                uint32_t strideAlignment) -> bool {
    auto bo = allocateFn(type, drmFourcc, width, height, modifier, strideAlignment);
    if (!bo)
      return false;

    auto desc = BuildDesc(*bo, width, height);
    EGLImageKHR image{nullptr};
    std::string error;

    const uint64_t importModifier = explicitModifier ? modifier : DRM_FORMAT_MOD_INVALID;
    if (!TryImportDmaBufAsEGLImage(desc, drmFourcc, importModifier, &image, &error))
      return false;

    m_eglDestroyImageKHR(m_display, image);

    result.success = true;
    result.allocator = type;
    result.modifier = modifier;
    result.explicitModifier = explicitModifier;
    result.strideAlignment = strideAlignment;
    return true;
  };

  std::vector<uint64_t> prioritizedModifiers;
  {
    std::scoped_lock lock(m_mutex);
    prioritizedModifiers = BuildModifierPriority(drmFourcc, preferLinear);
  }

  for (uint64_t modifier : prioritizedModifiers)
  {
    // Path A: GBM modifier allocation and explicit import to avoid implicit-modifier ambiguity.
    if (tryAllocator(AllocatorType::GBMModifiers, modifier, true, 0))
      break;
  }

  // Path B: GBM without explicit modifiers, lets EGL/driver resolve implicit layout.
  if (!result.success)
    tryAllocator(AllocatorType::GBMImplicit, DRM_FORMAT_MOD_INVALID, false, 0);

  // Path C: DMA-heap with implicit modifier import.
  if (!result.success)
    tryAllocator(AllocatorType::DMAHeapImplicit, DRM_FORMAT_MOD_INVALID, false, 0);

  // Path D: DMA-heap with conservative stride alignment requirement to satisfy stricter importers.
  if (!result.success)
    tryAllocator(AllocatorType::DMAHeapAligned, DRM_FORMAT_MOD_INVALID, false,
                 STRIDE_ALIGNMENT_CONSERVATIVE);

  if (result.success)
  {
    std::scoped_lock lock(m_mutex);
    StoreKnownGoodLocked(key, result);

    CLog::Log(LOGDEBUG,
              "CDmaBufImportProber::{} selected {} for fourcc={} buckets={}x{} modifier={}"
              " explicitModifier={} strideAlignment={}",
              __FUNCTION__, AllocatorTypeToString(result.allocator), DRMHELPERS::FourCCToString(drmFourcc),
              key.widthBucket, key.heightBucket, DRMHELPERS::ModifierToString(result.modifier),
              result.explicitModifier, result.strideAlignment);
  }
  else
  {
    result.error = "all non-copy DMA import paths failed";
  }

  return result;
}


void CDmaBufImportProber::SetCapabilityCacheForTest(
    const std::vector<uint32_t>& formats,
    const std::map<uint32_t, std::vector<ModifierInfo>>& modifiers,
    bool hasModifiersExtension)
{
  std::scoped_lock lock(m_mutex);
  m_caps.queried = true;
  m_caps.hasModifiersExt = hasModifiersExtension;
  m_caps.formats = formats;
  m_caps.modifiers = modifiers;
}

void CDmaBufImportProber::Invalidate(const ProbeKey& key)
{
  std::scoped_lock lock(m_mutex);
  m_knownGood.erase(key);
}
