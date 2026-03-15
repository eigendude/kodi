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

CGameClientDiscModel CGameClientDiscMergeUtils::ReconcileModels(
    const CGameClientDiscModel& frontendDiscs, const CGameClientDiscModel& coreDiscs)
{
  CGameClientDiscModel mergedDiscs;

  // Set ejected
  mergedDiscs.SetEjected(coreDiscs.IsEjected());

  // Set discs
  for (size_t discIndex = 0; discIndex < coreDiscs.Size(); ++discIndex)
  {
    const GameClientDiscEntry& coreDisc = coreDiscs.GetDiscs().at(discIndex);

    // Use core metadata as source of truth, unless empty
    std::string mergedPath = coreDisc.path;
    std::string mergedLabel = coreDisc.cachedLabel;

    //
    // Anomalous reasons the path could be empty:
    //
    //   - The core doesn't support disc metadata and always returns empty
    //     paths/labels
    //   - The core can't report disc metadata immediately, but will report it
    //     after a disc is added or replaced
    //   - The core doesn't compact disc slots after removal, and the slot is
    //     currently removed, so there's no real disc to report
    //   - Some cores report the selected index before exposing disc path/label
    //     metadata
    //
    if (mergedPath.empty())
    {
      // Some cores don't compact indexes when removing discs, they simply mark
      // the index as an empty slot. In this case, check if the slot was
      // previously removed.
      if (frontendDiscs.Size() == coreDiscs.Size())
      {
        // Check if it's a removed disc in the frontend
        if (frontendDiscs.IsRemovedSlotByIndex(discIndex))
        {
          mergedDiscs.AddRemovedSlot();
          continue;
        }
      }

      // If the core slot is empty but we already have frontend state for the
      // same slot, keep the frontend identity until the core can report it.
      if (frontendDiscs.IsSelectableSlotByIndex(discIndex))
      {
        mergedPath = frontendDiscs.GetPathByIndex(discIndex);
        if (mergedLabel.empty())
          mergedLabel = frontendDiscs.GetLabelByIndex(discIndex);
      }
    }

    if (mergedLabel.empty())
    {
      // If the core can report a path but not a label, preserve any frontend
      // label to avoid UI flicker. This can happen during startup when the
      // core reports paths before it can report labels, or if the core simply
      // doesn't support disc labels.
      mergedLabel = frontendDiscs.GetLabelByIndex(discIndex);
    }

    mergedDiscs.AddDisc(mergedPath, mergedLabel);
  }

  // Set selected
  if (!coreDiscs.IsSelectedNoDisc())
    mergedDiscs.SetSelectedDiscByIndex(*coreDiscs.GetSelectedDiscIndex());
  else
    mergedDiscs.SetSelectedNoDisc();

  return mergedDiscs;
}
