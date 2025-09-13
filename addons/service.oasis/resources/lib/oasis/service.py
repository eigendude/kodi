################################################################################
#
#  Copyright (C) 2022 Garrett Brown
#  This file is part of OASIS - https://github.com/eigendude/OASIS
#
#  SPDX-License-Identifier: Apache-2.0
#  See the file LICENSE.txt for more information.
#
################################################################################

import xbmc  # pylint: disable=import-error
import xbmcaddon  # pylint: disable=import-error
import xbmcgui  # pylint: disable=import-error

from oasis.windows.camera_view import CameraView
from oasis.windows.fireworks_hud import FireworksHUD
from oasis.windows.ocean_hud import OceanHUD
from oasis.windows.station_hud import StationHUD
from oasis.windows.swellpatrol_hud import SwellPatrolHUD
from oasis.windows.ventura_hud import VenturaHUD


# Window IDs
HOME_WINDOW_ID: int = 10000  # WINDOW_HOME

# Window properties
WINDOW_PROPERTY_ADDON: str = "Addon.ID"
WINDOW_PROPERTY_HOSTNAME: str = "hostname"

# HD Animated Weather Icons add-on ID
WEATHER_ICONS_ADDON_ID: str = "resource.images.weathericons.hd.animated"

class OasisService:
    @staticmethod
    def _configure_weather_icons() -> None:
        """
        Configure Estuary skin to use HD animated weather icons, if available.
        """
        try:
            if xbmc.getCondVisibility(f"System.HasAddon({WEATHER_ICONS_ADDON_ID})"):
                hd_addon = xbmcaddon.Addon(id=WEATHER_ICONS_ADDON_ID)

                hd_path = f"resource://{WEATHER_ICONS_ADDON_ID}/"
                hd_name = hd_addon.getAddonInfo("name")

                if not xbmc.getCondVisibility(
                    "Skin.HasSetting(WeatherOutlookIcon.multi)"
                ):
                    xbmc.executebuiltin("Skin.SetBool(WeatherOutlookIcon.multi)")
                if xbmc.getInfoLabel(
                    "Skin.String(WeatherOutlookIcon.path)"
                ) != hd_path:
                    xbmc.executebuiltin(
                        f"Skin.SetString(WeatherOutlookIcon.path,{hd_path})"
                    )
                if xbmc.getInfoLabel(
                    "Skin.String(WeatherOutlookIcon.name)"
                ) != hd_name:
                    xbmc.executebuiltin(
                        f"Skin.SetString(WeatherOutlookIcon.name,{hd_name})"
                    )
        except Exception as exc:  # pylint: disable=broad-except
            xbmc.log(
                f"Failed to configure weather icons: {exc}", level=xbmc.LOGERROR
            )

    @classmethod
    def run(cls, hostname: str) -> None:
        addon: xbmcaddon.Addon = xbmcaddon.Addon()
        addon_id = addon.getAddonInfo("id")
        addon_path: str = addon.getAddonInfo("path")

        xbmc.log(f"Running OASIS service on {hostname}", level=xbmc.LOGDEBUG)

        # Set up weather icons
        cls._configure_weather_icons()

        # Set the hostname property for the home window
        home_win = xbmcgui.Window(HOME_WINDOW_ID)
        home_win.setProperty(WINDOW_PROPERTY_HOSTNAME, hostname)

        window: xbmcgui.WindowXML

        # TODO: Hardware configuration
        HAS_CAMERA: bool = False
        if hostname == "bar":
            HAS_CAMERA = True

        if HAS_CAMERA:
            window = CameraView("CameraView1.xml", addon_path, "default", "1080i", False)
        elif hostname == "patio":
            window = SwellPatrolHUD("PatioHUD.xml", addon_path, "default", "1080i", False)
        elif hostname == "hallway":
            window = SwellPatrolHUD("WeatherHUD.xml", addon_path, "default", "1080i", False)
        else:
            window = SwellPatrolHUD("VideoHUD.xml", addon_path, "default", "1080i", False)

        # Set the addon ID property for the window
        window.setProperty(WINDOW_PROPERTY_ADDON, addon_id)

        window.doModal()

        # Small delay to avoid issues when closing the window
        xbmc.sleep(100)
        del window

        xbmc.log("Exiting OASIS service", level=xbmc.LOGDEBUG)
