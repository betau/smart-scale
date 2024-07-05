/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
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
#include <stdbool.h>
#include "sl_status.h"
#include "sl_simple_button_instances.h"
#include "app_timer.h"
#include "app_log.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"
#include "gatt_db.h"
#include "hx711.h"

#define MEASUREMENT_INTERVAL_MS  1000
#define DEFAULT_SCALE            375
#define AVERAGE_COUNT            5


// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

// Button state.
static volatile bool tare_button_pressed = false;
static volatile bool on_off_button_pressed = false;

// Periodic timer handle.
static app_timer_t app_periodic_timer;

// Periodic timer callback.
static void app_periodic_timer_cb(app_timer_t *timer, void *data);

void measurement_indication_changed_cb(sl_bt_gatt_client_config_flag_t client_config);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  app_log("smart scale\n");
  HX711_init(128);
  app_log("HX711_init done\n");
  HX711_tare(AVERAGE_COUNT);
  app_log("HX711_tare done\n");
  HX711_set_scale(DEFAULT_SCALE);
  app_log("HX711_set_scale done\n");
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
  if (tare_button_pressed) {
    tare_button_pressed = false;
    HX711_tare(AVERAGE_COUNT);
    app_log("HX711_tare done\n");
  }
  if (on_off_button_pressed) {
    on_off_button_pressed = false;
    app_log("value: %f\n", HX711_get_mean_units(AVERAGE_COUNT));
  }
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;

  // Handle stack events
  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      // Print boot message.
      app_log_info("Bluetooth stack booted: v%d.%d.%d-b%d\n",
                   evt->data.evt_system_boot.major,
                   evt->data.evt_system_boot.minor,
                   evt->data.evt_system_boot.patch,
                   evt->data.evt_system_boot.build);

      // Extract unique ID from BT Address.
      sc = sl_bt_system_get_identity_address(&address, &address_type);
      app_assert_status(sc);

      app_log_info("Bluetooth %s address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                   address_type ? "static random" : "public device",
                   address.addr[5],
                   address.addr[4],
                   address.addr[3],
                   address.addr[2],
                   address.addr[1],
                   address.addr[0]);

      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle, // advertising set handle
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);

      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);

      app_log_info("Started advertising\n");
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log_info("Connection opened\n");
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log_info("Connection closed\n");

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      app_log_info("Started advertising\n");
      app_timer_stop(&app_periodic_timer);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    case sl_bt_evt_gatt_server_characteristic_status_id:
      if (gattdb_measurement == evt->data.evt_gatt_server_characteristic_status.characteristic) {
        // client characteristic configuration changed by remote GATT client
        if (sl_bt_gatt_server_client_config == (sl_bt_gatt_server_characteristic_status_flag_t)evt->data.evt_gatt_server_characteristic_status.status_flags) {
          measurement_indication_changed_cb(
            (sl_bt_gatt_client_config_flag_t)evt->data.evt_gatt_server_characteristic_status.client_config_flags);
        }
      }
      break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

/**************************************************************************//**
 * Called when indication of the measurement is enabled/disabled by the client.
 *****************************************************************************/
void measurement_indication_changed_cb(sl_bt_gatt_client_config_flag_t client_config)
{
  sl_status_t sc;
  // Indication or notification enabled.
  if (sl_bt_gatt_disable != client_config) {
    // Start timer used for periodic indications.
    sc = app_timer_start(&app_periodic_timer,
                         MEASUREMENT_INTERVAL_MS,
                         app_periodic_timer_cb,
                         NULL,
                         true);
    app_assert_status(sc);
    // Send first indication.
    app_periodic_timer_cb(&app_periodic_timer, NULL);
  }
  // Indications disabled.
  else {
    // Stop timer used for periodic indications.
    (void)app_timer_stop(&app_periodic_timer);
  }
}

/**************************************************************************//**
 * Simple Button
 * Button state changed callback
 * @param[in] handle Button event handle
 *****************************************************************************/
void sl_button_on_change(const sl_button_t *handle)
{
  // Button pressed.
  if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED) {
    if (&sl_button_btn0 == handle) {
      on_off_button_pressed = true;
    } else if (&sl_button_btn1 == handle) {
      tare_button_pressed = true;
    }
  }
}

/**************************************************************************//**
 * Timer callback
 * Called periodically to time periodic measurements and indications.
 *****************************************************************************/
static void app_periodic_timer_cb(app_timer_t *timer, void *data)
{
  (void)data;
  (void)timer;
  double value = HX711_get_mean_units(AVERAGE_COUNT);
  int32_t value_int = (int32_t)value;
  app_log("value: %f\n", value);
  sl_bt_gatt_server_notify_all(gattdb_measurement, sizeof(value_int), (uint8_t*)&value_int);
}
