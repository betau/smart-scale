/***************************************************************************//**
 * @file
 * @brief Non-Volatile Memory
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

#include "nvm3.h"
#include "nvm.h"

static sl_status_t ecode2sc(Ecode_t ec);

sl_status_t nvm_init(void)
{
  return ecode2sc(nvm3_initDefault());
}

sl_status_t nvm_read(nmv_key_t key, void *buf, size_t *len)
{
  uint32_t nvm3_objecttype;
  // Store input buffer length temporarily
  size_t tmp = *len;
  // Read length of the data
  Ecode_t nvm3_ec = nvm3_getObjectInfo(nvm3_defaultHandle,
                                       (uint32_t)key,
                                       &nvm3_objecttype,
                                       len);

  // If would be too long, fail
  if (*len > tmp) {
    return SL_STATUS_WOULD_OVERFLOW;
  }

  // If failure has occurred, return proper error code
  if (ECODE_NVM3_OK != nvm3_ec) {
    return ecode2sc(nvm3_ec);
  }

  // Read the data into the buffer based on the read length
  nvm3_ec = nvm3_readData(nvm3_defaultHandle,
                          (uint32_t)key,
                          buf,
                          *len);

  return ecode2sc(nvm3_ec);
}

sl_status_t nvm_write(nmv_key_t key,
                                 const void *buf,
                                 size_t len)
{
  Ecode_t nvm3_ec = nvm3_writeData(nvm3_defaultHandle,
                                   (uint32_t)key,
                                   buf, len);

  return ecode2sc(nvm3_ec);
}

sl_status_t nvm_erase(nmv_key_t key)
{
  Ecode_t nvm3_ec = nvm3_deleteObject(nvm3_defaultHandle,
                                      key);

  return ecode2sc(nvm3_ec);
}

sl_status_t nvm_erase_all(void)
{
  Ecode_t nvm3_ec = nvm3_eraseAll(nvm3_defaultHandle);

  return ecode2sc(nvm3_ec);
}

static sl_status_t ecode2sc(Ecode_t ec)
{
  switch (ec) {
    case ECODE_NVM3_OK:
      return SL_STATUS_OK;
    case ECODE_NVM3_ERR_SIZE_TOO_SMALL:
    case ECODE_NVM3_ERR_NO_VALID_PAGES:
    case ECODE_NVM3_ERR_RESIZE_NOT_ENOUGH_SPACE:
      return SL_STATUS_NO_MORE_RESOURCE;
    case ECODE_NVM3_ERR_PAGE_SIZE_NOT_SUPPORTED:
    case ECODE_NVM3_ERR_PARAMETER:
    case ECODE_NVM3_ERR_WRITE_DATA_SIZE:
    case ECODE_NVM3_ERR_OBJECT_SIZE_NOT_SUPPORTED:
    case ECODE_NVM3_ERR_RESIZE_PARAMETER:
    case ECODE_NVM3_ERR_ADDRESS_RANGE:
      return SL_STATUS_INVALID_PARAMETER;
    case ECODE_NVM3_ERR_STORAGE_FULL:
      return SL_STATUS_FULL;
    case ECODE_NVM3_ERR_NOT_OPENED:
      return SL_STATUS_INVALID_STATE;
    case ECODE_NVM3_ERR_OPENED_WITH_OTHER_PARAMETERS:
      return SL_STATUS_ALREADY_INITIALIZED;
    case ECODE_NVM3_ERR_KEY_INVALID:
    case ECODE_NVM3_ERR_INT_KEY_MISMATCH:
      return SL_STATUS_INVALID_KEY;
    case ECODE_NVM3_ERR_KEY_NOT_FOUND:
      return SL_STATUS_NOT_FOUND;
    case ECODE_NVM3_ERR_ERASE_FAILED:
      return SL_STATUS_FLASH_ERASE_FAILED;
    case ECODE_NVM3_ERR_WRITE_FAILED:
      return SL_STATUS_FLASH_PROGRAM_FAILED;
    default:
      return SL_STATUS_FAIL;
  }
}
