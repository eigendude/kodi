/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "CryptoTypes.h"

namespace KODI::CRYPTO
{
  struct Key
  {
    // Supported types of all keys
    enum class Type
    {
      Unspecified,
      ECDSA,
      Ed25519,
      Secp256k1,
      RSA,
    };

    // Key type
    Type type = Type::Unspecified;

    // Key content
    ByteArray data{};
  };

  struct PublicKey : public Key {};
  struct PrivateKey : public Key {};

  struct KeyPair
  {
    PublicKey publicKey;
    PrivateKey privateKey;
  };
}
