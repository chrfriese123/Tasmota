/*
  xsns_98_ens160.ino - ENS160 gas and air quality sensor support for Tasmota

  Copyright (C) 2021  Christoph Friese and Theo Arends

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_I2C
#ifdef USE_ENS160

/*********************************************************************************************\
 * ENS160 - Gas (TVOC - Total Volatile Organic Compounds) and Air Quality (CO2)
 *
 * Source: ScioSense
 *
 * I2C Address: 0x52 
\*********************************************************************************************/

#define XSNS_98             98
#define XI2C_57             57  // See I2CDEVICES.md

#define EVERYNSECONDS 5

#include "ScioSense_ENS160.h"

ScioSense_ENS160      ens160(ENS160_I2CADDR_0);
struct  {
  uint16_t  TVOC;
  uint16_t  eCO2;

  uint32_t  rHP0;
  uint32_t  rHP1;
  uint32_t  rHP2;
  uint32_t  rHP3;
  uint32_t  blHP0;
  uint32_t  blHP1;
  uint32_t  blHP2;
  uint32_t  blHP3;

  uint8_t ready = 0;
  uint8_t type = 0;;
  uint8_t tcnt = 0;
  uint8_t ecnt = 0;
} ENS160data;

/********************************************************************************************/

void ens160Detect(void)
{
  if (I2cActive(ENS160_I2CADDR_0)) { return; }

  if (ens160.begin()) {
    if (ens160.available()) {
        if (ens160.setMode(ENS160_OPMODE_STD) ) {
            ENS160data.type = 1;
            I2cSetActiveFound(ENS160_I2CADDR_0, "ENS160");
        }
    }
  }
}

void ens160Update(void)  // Perform every n second
{
  ENS160data.tcnt++;
  if (ENS160data.tcnt >= EVERYNSECONDS) {
    ENS160data.tcnt = 0;
    ENS160data.ready = 0;
    if (ens160.available()) {
        ens160.measure();
        ENS160data.TVOC = ens160.getTVOC();
        ENS160data.eCO2 = ens160.geteCO2();

        ENS160data.rHP0 = ens160.getHP0();
        ENS160data.blHP0 = ens160.getHP0BL();
        ENS160data.rHP1 = ens160.getHP1();
        ENS160data.blHP1 = ens160.getHP1BL();
        ENS160data.rHP2 = ens160.getHP2();
        ENS160data.blHP2 = ens160.getHP2BL();
        ENS160data.rHP3 = ens160.getHP3();
        ENS160data.blHP3 = ens160.getHP3BL();

        ENS160data.ready = 1;
        //if (TasmotaGlobal.global_update && (TasmotaGlobal.humidity > 0) && !isnan(TasmotaGlobal.temperature_celsius)) {
//          ens160.setEnvironmentalData((uint8_t)TasmotaGlobal.humidity, TasmotaGlobal.temperature_celsius);
        //}
        ENS160data.ecnt = 0;
      }
    } else {
      // failed, count up
      ENS160data.ecnt++;
      if (ENS160data.ecnt > 6) {
        // after 30 seconds, restart
        ens160.begin();
        if (ens160.available()) {
            ens160.setMode(ENS160_OPMODE_STD);
        }
      }
    }
}

const char HTTP_SNS_ens160[] PROGMEM =
  "{s}ENS160 " D_ECO2 "{m}%d " D_UNIT_PARTS_PER_MILLION "{e}"                // {s} = <tr><th>, {m} = </th><td>, {e} = </td></tr>
  "{s}ENS160 " D_TVOC "{m}%d " D_UNIT_PARTS_PER_BILLION "{e}";

void ens160Show(bool json)
{
  if (ENS160data.ready) {
    if (json) {
      ResponseAppend_P(PSTR(",\"ENS160\":{\"" D_JSON_ECO2 "\":%d,\"" D_JSON_TVOC "\":%d,\"R0\":%d,\"BL0\":%d,\"R1\":%d,\"BL1\":%d,\"R2\":%d,\"BL2\":%d,\"R3\":%d,\"BL3\":%d}"), ENS160data.eCO2, ENS160data.TVOC, ENS160data.rHP0, ENS160data.blHP0, ENS160data.rHP1, ENS160data.blHP1, ENS160data.rHP2, ENS160data.blHP2, ENS160data.rHP3, ENS160data.blHP3);
#ifdef USE_WEBSERVER
    } else {
      WSContentSend_PD(HTTP_SNS_ens160, ENS160data.eCO2, ENS160data.TVOC);
#endif
    }
  }
}

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

bool Xsns98(uint8_t function)
{
  if (!I2cEnabled(XI2C_57)) { return false; }

  bool result = false;

  if (FUNC_INIT == function) {
    ens160Detect();
  }
  else if (ENS160data.type) {
    switch (function) {
      case FUNC_EVERY_SECOND:
        ens160Update();
        break;
      case FUNC_JSON_APPEND:
        ens160Show(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_SENSOR:
        ens160Show(0);
        break;
#endif  // USE_WEBSERVER
    }
  }
  return result;
}

#endif  // USE_ENS160
#endif  // USE_I2C
