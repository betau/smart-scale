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
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"
#include "gatt_db.h"
#include "hx711.h"
#include "bthome_v2.h"
#include "sl_component_catalog.h"
#if defined(SL_CATALOG_APP_LOG_PRESENT)
#include "app_log.h"
#else
#define app_log(...)
#endif // SL_CATALOG_APP_LOG_PRESENT

#define MEASUREMENT_INTERVAL_IND_MS  1000
#define MEASUREMENT_INTERVAL_ADV_MS  10000
#define TARE_DELAY_MS                2000
#define DEFAULT_SCALE                375
#define AVERAGE_COUNT                5

static uint8_t device_name[] = "Mass";

// Button state.
static volatile bool tare_button_pressed = false;
static volatile bool on_off_button_pressed = false;

// Timers and their callbacks
static app_timer_t measurement_timer;
static app_timer_t tare_timer;
static void measurement_indication_cb(app_timer_t *timer, void *data);
static void measurement_advertising_cb(app_timer_t *timer, void *data);
static void tare_timer_cb(app_timer_t *timer, void *data);

static void measurement_indication_changed_cb(sl_bt_gatt_client_config_flag_t client_config);
static float get_mass(void);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  app_log("BTHome v2 scale\n");
  HX711_init(128);
  app_log("HX711_init done\n");
  HX711_tare(AVERAGE_COUNT);
  app_log("HX711_tare done\n");
  HX711_set_scale(DEFAULT_SCALE);
  app_log("HX711_set_scale done\n");
  HX711_power_down();
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
    sl_status_t sc = app_timer_start(&tare_timer,
                                     TARE_DELAY_MS,
                                     tare_timer_cb,
                                     NULL,
                                     false);
    app_assert_status(sc);
  }
  if (on_off_button_pressed) {
    on_off_button_pressed = false;
    // Just log the mass
    (void)get_mass();
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

  bthome_v2_bt_on_event(evt);

  // Handle stack events
  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      // Print boot message.
      app_log("Bluetooth stack booted: v%d.%d.%d-b%d\n",
              evt->data.evt_system_boot.major,
              evt->data.evt_system_boot.minor,
              evt->data.evt_system_boot.patch,
              evt->data.evt_system_boot.build);

      // Extract unique ID from BT Address.
      sc = sl_bt_system_get_identity_address(&address, &address_type);
      app_assert_status(sc);

      app_log("Bluetooth %s address: %02X:%02X:%02X:%02X:%02X:%02X\n",
              address_type ? "static random" : "public device",
              address.addr[5],
              address.addr[4],
              address.addr[3],
              address.addr[2],
              address.addr[1],
              address.addr[0]);

      app_assert_status(sc);
      
      sc = bthome_v2_init(device_name, false, NULL, false);
      app_assert_status(sc);

      bthome_v2_add_measurement_float(ID_MASS, get_mass());
      sc = bthome_v2_send_packet();
      app_assert_status(sc);

      sc = app_timer_start(&measurement_timer,
                           MEASUREMENT_INTERVAL_ADV_MS,
                           measurement_advertising_cb,
                           NULL,
                           true);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log("Connection opened\n");
      sc = app_timer_stop(&measurement_timer);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log("Connection closed\n");
      sc = app_timer_start(&measurement_timer,
                           MEASUREMENT_INTERVAL_ADV_MS,
                           measurement_advertising_cb,
                           NULL,
                           true);
      app_assert_status(sc);
      // Update advertising data
      measurement_advertising_cb(&measurement_timer, NULL);
      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:
      if (gattdb_mass == evt->data.evt_gatt_server_characteristic_status.characteristic) {
        // client characteristic configuration changed by remote GATT client
        if (sl_bt_gatt_server_client_config == (sl_bt_gatt_server_characteristic_status_flag_t)evt->data.evt_gatt_server_characteristic_status.status_flags) {
          measurement_indication_changed_cb(
            (sl_bt_gatt_client_config_flag_t)evt->data.evt_gatt_server_characteristic_status.client_config_flags);
        }
      }
      break;
    
    case sl_bt_evt_gatt_server_user_read_request_id:
      if (evt->data.evt_gatt_server_user_read_request.characteristic == gattdb_mass) {
        float mass = get_mass();
        int32_t mass_int = (int32_t)mass;
        sc = sl_bt_gatt_server_send_user_read_response(
            evt->data.evt_gatt_server_user_read_request.connection,
            evt->data.evt_gatt_server_user_read_request.characteristic,
            SL_STATUS_OK,
            sizeof(mass_int),
            (uint8_t *)&mass_int,
            NULL);
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
static void measurement_indication_changed_cb(sl_bt_gatt_client_config_flag_t client_config)
{
  sl_status_t sc;
  // Indication or notification enabled.
  if (sl_bt_gatt_disable != client_config) {
    // Start timer used for periodic indications.
    sc = app_timer_start(&measurement_timer,
                         MEASUREMENT_INTERVAL_IND_MS,
                         measurement_indication_cb,
                         NULL,
                         true);
    app_assert_status(sc);
    // Send first indication.
    measurement_indication_cb(&measurement_timer, NULL);
  }
  // Indications disabled.
  else {
    // Stop timer used for periodic indications.
    (void)app_timer_stop(&measurement_timer);
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

static void measurement_indication_cb(app_timer_t *timer, void *data)
{
  (void)data;
  (void)timer;
  int32_t mass_int = (int32_t)get_mass();
  sl_bt_gatt_server_notify_all(gattdb_mass, sizeof(mass_int), (uint8_t *)&mass_int);
}

static void measurement_advertising_cb(app_timer_t *timer, void *data)
{
  (void)data;
  (void)timer;
  float mass = get_mass();
  bthome_v2_reset_measurement();
  bthome_v2_add_measurement_float(ID_MASS, mass);
  bthome_v2_build_packet();
}

static void tare_timer_cb(app_timer_t *timer, void *data)
{
  (void)data;
  (void)timer;
  HX711_power_up();
  HX711_tare(AVERAGE_COUNT);
  HX711_power_down();
  app_log("tare done\n");
}

static float get_mass(void)
{
  HX711_power_up();
  float mass = HX711_get_mean_units(AVERAGE_COUNT);
  HX711_power_down();
  app_log("mass: %f\n", mass);
  return mass;
}
