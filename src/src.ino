/*
SPDX-License-Identifier: GPL-3.0-or-later

Copyright © 2025 Christian Conti, Alessandro Varaldi
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the Licence, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "INA226.h"

float pwr_ps = 0;
float pwr_pl = 0;

volatile byte trigger = LOW; 

INA226 *ina; 

#ifdef EXT_TRIGGER
  constexpr uint8_t TRIGGER_PIN = 2;
  bool lastTrigState = LOW;
  bool logging = false;
#endif

void setup() {
  Serial.begin(2000000);
  pinMode(LED_BUILTIN, OUTPUT);

  #ifdef EXT_TRIGGER
    pinMode(TRIGGER_PIN, INPUT);
  #endif
  
  #if defined(BOARD_ZCU106)
    ina = new INA226(ZCU106); 
  #elif defined(BOARD_ZCU102)
    ina = new INA226(ZCU102);
  #else  
    digitalWrite(LED_BUILTIN, HIGH); 
  #endif

  delay(1000);

}

void loop() { 

  #ifdef EXT_TRIGGER
    const bool trigState = digitalRead(TRIGGER_PIN);
    if (trigState == HIGH && lastTrigState == LOW) {
        logging = !logging;
        Serial.println(logging ? "#START" : "#STOP");
    }
    lastTrigState = trigState;

    if (!logging) {
        delay(1);
        return;
    }
  #endif

  pwr_ps = ina->get_pwr(PS);
  pwr_pl = ina->get_pwr(PL);

  Serial.print(micros());
  Serial.print('\t');
  Serial.print(pwr_ps, 5);
  Serial.print('\t');
  Serial.println(pwr_pl, 5);
}