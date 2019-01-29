#!/bin/bash
#
#  Copyright (C) 2019 Aclima, Inc.
#  This file is part of Sundstrom - https://github.com/Aclima/sundstrom
#
#  SPDX-License-Identifier: MIT
#  See LICENSES/README.md for more information.
#

#
# Requires appdmg utility:
#
#   npm install -g appdmg
#

set -o errexit
set -o pipefail
set -o nounset

DMG_PATH="Sundstrom-1.0-alpha1.dmg"

if [ -f ${DMG_PATH} ]; then
  rm ${DMG_PATH}
fi

appdmg spec.json ${DMG_PATH}
