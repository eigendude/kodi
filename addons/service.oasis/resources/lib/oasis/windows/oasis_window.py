################################################################################
#
#  Copyright (C) 2024 Garrett Brown
#  This file is part of OASIS - https://github.com/eigendude/OASIS
#
#  SPDX-License-Identifier: Apache-2.0
#  See the file LICENSE.txt for more information.
#
################################################################################

import xbmcgui  # pylint: disable=import-error


class OasisWindow(xbmcgui.WindowXML):
    """Window base class that swallows mouse input."""

    def onAction(self, action: xbmcgui.Action) -> None:  # type: ignore[override]
        action_id = action.getId()

        if xbmcgui.ACTION_MOUSE_START <= action_id <= xbmcgui.ACTION_MOUSE_END:
            return

        super().onAction(action)

    def onClick(self, control_id: int) -> None:  # type: ignore[override]
        return

    def onDoubleClick(self, control_id: int) -> None:  # type: ignore[override]
        return
