// SPDX-License-Identifier: GPL-3.0-or-later
//
// Copyright © 2025 Christian Conti, Alessandro Varaldi
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the Licence, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef INA226_H
#define INA226_H

#include "Arduino.h"
#include "Wire.h"

// Default address of the TCA9548APWR multiplexer
#define MUX_ADDR 0x75
// Default address of INA226 current, voltage, power monitor
#define STD_ADDR 0x40

// INA226 registers addresses
#define CAL_REG  0x05
#define PWR_REG  0x03

// List of currently supported boards
typedef enum board {
    ZCU102,
    ZCU106,
    NUM_BOARDS    
} board_typeDef;

typedef enum sensor {
    PS,
    PL,
    NUM_SENS    
} sensor_typeDef;

// Value of default calibration register for PS & PL, based on Xilinx SCUI SW
// [ZCU102, ZCU106, ...]
// [[PS, PL], ...]
static const uint16_t cal_reg[NUM_BOARDS][2] = {{0x0D1B, 0x0800}, {0x0800, 0x0831}};
// LSB value obtained through datasheet
static const float lsb_val[NUM_SENS][2] = {{0.0003052, 0.00125}, {0.0005, 0.0012208}};

class INA226 {
public:
    // Constructor with default address
    explicit INA226(const board_typeDef &board, TwoWire *wire = &Wire);
    // Constructor with non-default address 
    explicit INA226(const uint8_t &addr, const board_typeDef &board, TwoWire *wire = &Wire);
    
    const float get_pwr(const sensor_typeDef &sensor);
    const void set_I2C_speed(const uint16_t &speed);
    const void set_addr(const uint8_t &addr);

private:

    uint8_t _address;
    board_typeDef _board;
    TwoWire * _wire;

    void _sel_sensor(const sensor_typeDef &sensor);
    const int8_t _write_reg(const uint8_t &reg, const uint16_t &val);
    int32_t _read_reg(const uint8_t &reg);
};

#endif // INA226_H
