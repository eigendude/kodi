/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "guishader_common.hlsl"

Texture2D texMain : register(t0);

float4 PS(PS_INPUT input) : SV_TARGET
{
  return texMain.Sample(NearestSampler, input.tex);
}

