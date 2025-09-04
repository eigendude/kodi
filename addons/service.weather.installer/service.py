# -*- coding: utf-8 -*-
import json
import time
import xbmc
import xbmcgui

ADDON_TO_INSTALL = "weather.openmeteo"
ADDON_NAME = "Open-Meteo"
SERVICE_TAG = "[weather.installer]"

def log(msg, level=xbmc.LOGINFO):
    try:
        xbmc.log(f"{SERVICE_TAG} {msg}", level)
    except Exception:
        pass

def has_addon(addon_id):
    return xbmc.getCondVisibility(f"System.HasAddon({addon_id})")

def addon_enabled(addon_id):
    return xbmc.getCondVisibility(f"System.AddonIsEnabled({addon_id})")

def online():
    # Prefer InternetState (true when connected to the internet)
    return xbmc.getCondVisibility("System.InternetState")

def enable_addon(addon_id):
    try:
        req = {
            "jsonrpc": "2.0",
            "id": 1,
            "method": "Addons.SetAddonEnabled",
            "params": {"addonid": addon_id, "enabled": True},
        }
        resp_raw = xbmc.executeJSONRPC(json.dumps(req))
        log(f"Enable response: {resp_raw}")
    except Exception as e:
        log(f"Failed to enable {addon_id}: {e}", xbmc.LOGERROR)

def install_addon(addon_id, timeout_sec=120):
    # Trigger install from repository
    xbmc.executebuiltin(f"InstallAddon({addon_id})")
    log(f"Requested installation for {addon_id}")

    monitor = xbmc.Monitor()
    start = time.time()
    # Wait for install to complete or timeout/abort
    while time.time() - start < timeout_sec and not monitor.abortRequested():
        if has_addon(addon_id):
            log(f"{addon_id} appears installed")
            return True
        # small sleep with abort support
        monitor.waitForAbort(1.0)
    return has_addon(addon_id)

def notify(title, message, level=xbmcgui.NOTIFICATION_INFO, ms=5000):
    try:
        xbmcgui.Dialog().notification(title, message, level, ms)
    except Exception:
        # Notifications can fail on some headless/test setups; ignore
        log(f"Notify: {title} - {message}")

def main():
    monitor = xbmc.Monitor()

    if has_addon(ADDON_TO_INSTALL):
        log(f"{ADDON_TO_INSTALL} already installed")
        if not addon_enabled(ADDON_TO_INSTALL):
            enable_addon(ADDON_TO_INSTALL)
            notify("Weather Installer", f"Enabled {ADDON_NAME}.")
        return

    # Not installed: ensure we have internet; wait a bit if offline
    max_wait = 300  # 5 minutes
    started = time.time()
    if not online():
        notify("Weather Installer", "Kodi appears offline. Waiting for internet...", xbmcgui.NOTIFICATION_INFO, 6000)
        log("Offline at startup; waiting up to 5 minutes for connectivity")
    while not online() and not monitor.abortRequested():
        if time.time() - started > max_wait:
            notify("Weather Installer", "Still offline. Will try again next start.", xbmcgui.NOTIFICATION_WARNING, 7000)
            log("Gave up waiting for internet")
            return
        monitor.waitForAbort(5.0)

    if monitor.abortRequested():
        log("Abort requested; exiting")
        return

    notify("Weather Installer", f"Installing {ADDON_NAME}...", xbmcgui.NOTIFICATION_INFO, 4000)
    ok = install_addon(ADDON_TO_INSTALL)
    if ok:
        if not addon_enabled(ADDON_TO_INSTALL):
            enable_addon(ADDON_TO_INSTALL)
        notify("Weather Installer", f"{ADDON_NAME} installed.", xbmcgui.NOTIFICATION_INFO, 5000)
        log(f"{ADDON_TO_INSTALL} installed successfully")
    else:
        notify("Weather Installer", f"Install failed. Open Add-on Browser to install {ADDON_NAME} manually.", xbmcgui.NOTIFICATION_ERROR, 8000)
        log(f"{ADDON_TO_INSTALL} failed to install", xbmc.LOGERROR)

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        log(f"Unhandled exception: {e}", xbmc.LOGERROR)