/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDiscMergeUtils.h"

#include "games/addons/disc/GameClientDiscModel.h"

#include <algorithm>
#include <optional>
#include <string>

using namespace KODI;
using namespace GAME;

void CGameClientDiscMergeUtils::ReconcileModels(CGameClientDiscModel& frontendDiscs,
                                                const CGameClientDiscModel& coreDiscs)
{
  std::optional<std::string> previousSelectedPath;
  if (!frontendDiscs.IsSelectedNoDisc())
  {
    const std::optional<size_t> selectedIndex = frontendDiscs.GetSelectedDiscIndex();
    if (selectedIndex.has_value() && frontendDiscs.IsRealDiscByIndex(*selectedIndex))
      previousSelectedPath = frontendDiscs.GetPathByIndex(*selectedIndex);
  }

  std::optional<std::string> coreSelectedPath;
  if (!coreDiscs.IsSelectedNoDisc())
  {
    const std::optional<size_t> selectedIndex = coreDiscs.GetSelectedDiscIndex();
    if (selectedIndex.has_value() && coreDiscs.IsRealDiscByIndex(*selectedIndex))
      coreSelectedPath = coreDiscs.GetPathByIndex(*selectedIndex);
  }

  CGameClientDiscModel reconciled;

  const size_t overlapSize = std::min(frontendDiscs.Size(), coreDiscs.Size());
  for (size_t i = 0; i < overlapSize; ++i)
  {
    if (coreDiscs.IsRealDiscByIndex(i))
      reconciled.AddDisc(coreDiscs.GetPathByIndex(i), coreDiscs.GetLabelByIndex(i));
    else if (frontendDiscs.IsRemovedSlotByIndex(i))
      reconciled.AddRemovedSlot();
  }

  for (size_t i = overlapSize; i < frontendDiscs.Size(); ++i)
  {
    if (frontendDiscs.IsRemovedSlotByIndex(i))
      reconciled.AddRemovedSlot();
  }

  for (size_t i = overlapSize; i < coreDiscs.Size(); ++i)
  {
    if (coreDiscs.IsRealDiscByIndex(i))
      reconciled.AddDisc(coreDiscs.GetPathByIndex(i), coreDiscs.GetLabelByIndex(i));
  }

  reconciled.SetEjected(coreDiscs.IsEjected());

  if (previousSelectedPath.has_value() &&
      reconciled.GetDiscIndexByPath(*previousSelectedPath).has_value())
  {
    reconciled.SetSelectedDiscByPath(*previousSelectedPath);
  }
  else if (coreSelectedPath.has_value() &&
           reconciled.GetDiscIndexByPath(*coreSelectedPath).has_value())
  {
    reconciled.SetSelectedDiscByPath(*coreSelectedPath);
  }
  else
  {
    reconciled.SetSelectedNoDisc();
  }

  frontendDiscs = reconciled;
}
