/***************************************************************************//**
 * @file
 * @brief Non-Volatile Memory API
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

#include <stddef.h>
#include "sl_status.h"

typedef enum {
  NVM_KEY_SCALE = 1,
  NVM_KEY_OFFSET,
  NVM_KEY_OBJECT_ID,
  NVM_KEY_INTERVAL_IND,
  NVM_KEY_INTERVAL_ADV
} nmv_key_t;

sl_status_t nvm_init(void);
sl_status_t nvm_read(nmv_key_t key, void *buf, size_t *len);
sl_status_t nvm_write(nmv_key_t key, const void *buf, size_t len);
sl_status_t nvm_erase(nmv_key_t key);
sl_status_t nvm_erase_all(void);
