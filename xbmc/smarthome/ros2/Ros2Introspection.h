/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <rmw_fastrtps_dynamic_cpp/TypeSupport.hpp>
#include <rosidl_dynamic_typesupport/dynamic_message.hpp>
#include <rosidl_dynamic_typesupport/dynamic_message_type_support.hpp>

namespace KODI
{
namespace SMART_HOME
{
class CRos2Introspection
{
public:
  CRos2Introspection();
  ~CRos2Introspection();
};
} // namespace SMART_HOME
} // namespace KODI
