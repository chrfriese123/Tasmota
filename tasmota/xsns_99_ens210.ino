/*
  xsns_99_ens210.ino - ENS210 gas and air quality sensor support for Tasmota

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
#ifdef USE_ENS210

/*********************************************************************************************\
 * ENS210 - Temperature & Humidity sensor
 *
 * Source: ScioSense
 *
 * I2C Address: 0x43 
\*********************************************************************************************/

#define XSNS_99             99
#define XI2C_58             58  // See I2CDEVICES.md

#define ENS210_I2CADDR      0x43
#define EVERYNSECONDS 5

#include "ScioSense_ENS210.h"

ScioSense_ENS210      ens210;
struct  {
  uint16_t  temperature;
  uint16_t  humidity;

  uint8_t ready = 0;
  uint8_t type = 0;;
  uint8_t tcnt = 0;
  uint8_t ecnt = 0;
} ENS210data;

/********************************************************************************************/

void ens210Detect(void)
{
  if (I2cActive(ENS210_I2CADDR)) { return; }

  if (ens210.begin()) {
    if (ens210.available()) {
        ens210.setSingleMode(false);
        ENS210data.type = 1;
        I2cSetActiveFound(ENS210_I2CADDR, "ENS210");
    }
  }
}

void ens210Update(void)  // Perform every n second
{
    ENS210data.tcnt++;
    if (ENS210data.tcnt >= EVERYNSECONDS) {
        ENS210data.tcnt = 0;
        ENS210data.ready = 0;
        if (ens210.available()) {
            ens210.measure();
            
            ENS210data.temperature = ens210.getTempCelsius();
            ENS210data.humidity = ens210.getHumidityPercent();
            ENS210data.ready = 1;
            ENS210data.ecnt = 0;
        } else {
            // failed, count up
            ENS210data.ecnt++;
            if (ENS210data.ecnt > 6) {
                // after 30 seconds, restart
                ens210.begin();
                if (ens210.available()) {
                    ens210.setSingleMode(false);
                }
            }
        }  
    }
}

void ens210Show(bool json)
{
  if (ENS210data.ready) {
    TempHumDewShow(json, (0 == TasmotaGlobal.tele_period), "ENS210", ENS210data.temperature, ENS210data.humidity);
  }
}

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

bool Xsns99(uint8_t function)
{
  if (!I2cEnabled(XI2C_58)) { return false; }

  bool result = false;

  if (FUNC_INIT == function) {
    ens210Detect();
  }
  else if (ENS210data.type) {
    switch (function) {
      case FUNC_EVERY_SECOND:
        ens210Update();
        break;
      case FUNC_JSON_APPEND:
        ens210Show(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_SENSOR:
        ens210Show(0);
        break;
#endif  // USE_WEBSERVER
    }
  }
  return result;
}

#endif  // USE_ENS210
#endif  // USE_I2C
