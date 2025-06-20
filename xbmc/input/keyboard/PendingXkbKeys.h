/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// Temporary key names for pending additions to xorgproto.
// These values match Linux input-event keycodes. Once upstream
// names stabilize these constants should be replaced by the
// official XF86 definitions.
// TODO: switch to upstream names when
// https://gitlab.freedesktop.org/xorg/proto/xorgproto/-/merge_requests/93
// is merged and released.

namespace KODI
{
namespace KEYBOARD
{
namespace PENDING
{
enum Key
{
  KEY_OK = 0x160,
  KEY_SELECT = 0x161,
  KEY_GOTO = 0x162,
  KEY_CLEAR = 0x163,
  KEY_APPLICATION_POWER = 0x164,
  KEY_OPTION = 0x165,
  KEY_INFO = 0x166,
  KEY_TIME = 0x167,
  KEY_VENDOR = 0x168,
  KEY_ARCHIVE = 0x169,
  KEY_PROGRAM_GUIDE = 0x16a,
  KEY_CHANNEL = 0x16b,
  KEY_FAVORITES = 0x16c,
  KEY_EPG = 0x16d,
  KEY_PVR = 0x16e,
  KEY_MHP = 0x16f,
  KEY_LANGUAGE = 0x170,
  KEY_TITLE = 0x171,
  KEY_SUBTITLE = 0x172,
  KEY_CYCLE_ANGLE = 0x173,
  KEY_FULL_SCREEN = 0x174,
  KEY_SOUND_MODE = 0x175,
  KEY_KEYBOARD = 0x176,
  KEY_ASPECT_RATIO = 0x177,
  KEY_PC = 0x178,
  KEY_TV = 0x179,
  KEY_TV2 = 0x17a,
  KEY_VCR = 0x17b,
  KEY_VCR2 = 0x17c,
  KEY_SAT = 0x17d,
  KEY_SAT2 = 0x17e,
  KEY_CD = 0x17f,
  KEY_TAPE = 0x180,
  KEY_RADIO = 0x181,
  KEY_TUNER = 0x182,
  KEY_LAUNCH_AV_PLAYER = 0x183,
  KEY_TEXT = 0x184,
  KEY_DVD = 0x185,
  KEY_AUX = 0x186,
  KEY_MP3 = 0x187,
  KEY_AUDIO = 0x188,
  KEY_VIDEO = 0x189,
  KEY_DIRECTORY = 0x18a,
  KEY_LIST = 0x18b,
  KEY_MEMO = 0x18c,
  KEY_CALENDAR = 0x18d,
  KEY_RED = 0x18e,
  KEY_GREEN = 0x18f,
  KEY_YELLOW = 0x190,
  KEY_BLUE = 0x191,
  KEY_CHANNEL_UP = 0x192,
  KEY_CHANNEL_DOWN = 0x193,
  KEY_FIRST = 0x194,
  KEY_LAST = 0x195,
  KEY_AB = 0x196,
  KEY_NEXT = 0x197,
  KEY_RESTART = 0x198,
  KEY_SLOW = 0x199,
  KEY_SHUFFLE = 0x19a,
  KEY_BREAK = 0x19b,
  KEY_PREVIOUS = 0x19c,
  KEY_DIGITS = 0x19d,
  KEY_TEEN = 0x19e,
  KEY_TWEN = 0x19f
};
} // namespace PENDING
} // namespace KEYBOARD
} // namespace KODI

