#pragma once

#include "utils/Digest.h"

namespace KODI
{
namespace GAME
{
namespace DIGEST
{

// Helper to compute digest using a provided updater callback
// The callback receives a reference to UTILITY::CDigest and can call Update()
// with all pieces contributing to the digest.

template <typename Updater>
inline std::string ComposeDigest(UTILITY::CDigest::Type type, Updater updater)
{
  UTILITY::CDigest digest{type};
  updater(digest);
  return digest.FinalizeRaw();
}

} // namespace DIGEST
} // namespace GAME
} // namespace KODI

