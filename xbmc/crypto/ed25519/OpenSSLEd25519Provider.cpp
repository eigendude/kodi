/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OpenSSLEd25519Provider.h"

#include <openssl/evp.h>

using namespace KODI;
using namespace CRYPTO;

constexpr size_t ED25519_KEY_SIZE = 32u;

COpenSSLEd25519Provider::~COpenSSLEd25519Provider() = default;

KeyPair COpenSSLEd25519Provider::Generate()
{
  EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
  if (nullptr == pctx)
    return {};

  //auto free_pctx = gsl::finally([pctx] { EVP_PKEY_CTX_free(pctx); });

  if (1 != EVP_PKEY_keygen_init(pctx))
    return {};

  EVP_PKEY *pkey{nullptr};  // it is mandatory to nullify the pointer!
  if (1 != EVP_PKEY_keygen(pctx, &pkey))
    return {};

  //auto free_pkey = gsl::finally([pkey] { EVP_PKEY_free(pkey); });

  KeyPair keyPair{
    PublicKey{
      Key::Type::Ed25519,
      ByteArray(ED25519_KEY_SIZE),
    },
    PrivateKey{
      Key::Type::Ed25519,
      ByteArray(ED25519_KEY_SIZE),
    },
  };

  size_t privLen{keyPair.privateKey.data.size()};
  size_t pubLen{keyPair.publicKey.data.size()};

  if (1 != EVP_PKEY_get_raw_private_key(pkey, const_cast<uint8_t*>(keyPair.privateKey.data.data()), &privLen))
    return {};

  if (1 != EVP_PKEY_get_raw_public_key(pkey, const_cast<uint8_t*>(keyPair.publicKey.data.data()), &pubLen))
    return {};

  return keyPair;
}
