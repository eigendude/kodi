/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDiscMergeUtils.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

MergedDiscSlots KODI::GAME::MergeCoreSlotsByIndex(
    const std::vector<GameClientDiscEntry>& previousDiscs,
    const std::vector<GameClientDiscEntry>& coreDiscs)
{
  MergedDiscSlots merged;

  // One entry per core slot. Each element tells us which merged slot index a
  // given core slot ended up at, if any.
  merged.coreToMerged.resize(coreDiscs.size());

  // The merged list is at least as large as the larger input in the common
  // case, so reserve up front to avoid reallocations while appending.
  merged.discs.reserve(std::max(previousDiscs.size(), coreDiscs.size()));

  // For the overlapping prefix, trust the live core view. This preserves slot
  // numbering coming from the core while still allowing trailing removed
  // tombstones from the previous frontend model to be carried forward below.
  const size_t overlapCount = std::min(previousDiscs.size(), coreDiscs.size());
  for (size_t i = 0; i < overlapCount; ++i)
  {
    merged.discs.push_back(coreDiscs[i]);
    merged.coreToMerged[i] = i;
  }

  // If the previous frontend model had extra trailing removed slots, preserve
  // them so tombstones are not lost when the core reports fewer slots than the
  // frontend remembered.
  for (size_t i = overlapCount; i < previousDiscs.size(); ++i)
  {
    if (previousDiscs[i].slotType == GameClientDiscEntry::DiscSlotType::RemovedSlot)
      merged.discs.push_back(previousDiscs[i]);
  }

  // If the core has extra trailing slots, append them and record where each
  // core slot landed in the merged vector.
  for (size_t i = overlapCount; i < coreDiscs.size(); ++i)
  {
    merged.coreToMerged[i] = merged.discs.size();
    merged.discs.push_back(coreDiscs[i]);
  }

  return merged;
}
