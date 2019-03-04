#!/bin/bash
#
#  Copyright (C) 2019 Aclima, Inc.
#  This file is part of Sundstrom - https://github.com/Aclima/sundstrom
#
#  SPDX-License-Identifier: MIT
#  See LICENSES/README.md for more information.
#

#
# Requires appdmg utility.
#
# On Debian-based distros:
#
#   sudo apt install npm
#   npm install -g appdmg
#
# On macOS:
#
#   brew install npm
#   npm install -g appdmg
#

set -o errexit
set -o pipefail
set -o nounset

# TODO: Get this from version.txt
VERSION="1.0-alpha4"

DATE=`date +%Y%m%d`

OPEN_SOURCE_DMG_PATH="sundstrom-hud-${DATE}-${VERSION}-macos.dmg"
PROPRIETARY_DMG_PATH="sundstrom-aclima-hud-${DATE}-${VERSION}-macos.dmg"

for dmg_path in \
  ${OPEN_SOURCE_DMG_PATH} \
  ${PROPRIETARY_DMG_PATH} \
; do
  if [ -f ${dmg_path} ]; then
    rm ${dmg_path}
  fi
done

appdmg spec-opensource.json ${OPEN_SOURCE_DMG_PATH}
appdmg spec-proprietary.json ${PROPRIETARY_DMG_PATH}
