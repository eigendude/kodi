/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "BufferObject.h"

#include <functional>
#include <list>
#include <memory>
#include <string>

/**
 * @brief Factory that provides CBufferObject registration and creation
 *
 */
class CBufferObjectFactory
{
public:
  /**
   * @brief Create a CBufferObject from the registered BufferObject types.
   *        In the future this may include some criteria for selecting a specific
   *        CBufferObject derived type. Currently it returns the CBufferObject
   *        implementation that was registered last.
   *
   * @return std::unique_ptr<CBufferObject>
   */
  static std::unique_ptr<CBufferObject> CreateBufferObject(bool needsCreateBySize);

  /**
   * @brief Registers a CBufferObject class to class to the factory.
   *
   */
  static void RegisterBufferObject(const std::string& name,
                                   const std::function<std::unique_ptr<CBufferObject>()>&);

  /**
   * @brief Clears the list of registered CBufferObject types
   *
   */
  static void ClearBufferObjects();

protected:
  struct BufferObjectRegistration
  {
    std::string name;
    std::function<std::unique_ptr<CBufferObject>()> create;
  };

  static std::list<BufferObjectRegistration> m_bufferObjects;
};
