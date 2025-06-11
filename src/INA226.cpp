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

#include "INA226.h"

constexpr uint32_t I2C_SPEED_HIGH = 400000UL;
constexpr uint32_t I2C_SPEED_LOW  = 100000UL;
constexpr uint8_t  MUX_SENSOR_OFFSET = 0x04;
constexpr float    POWER_LSB_SCALE = 25.0f;

INA226::INA226(board_typeDef &board, TwoWire *wire)
    : _address(STD_ADDR),
      _board(board),
      _wire(wire)
{
    _wire->begin();
    set_I2C_speed(I2C_SPEED_HIGH);
    // Set default calibration registers for each sensor
    for (int i = 0; i < NUM_SENS; i++) { 
        _sel_sensor((sensor_typeDef)i);
        _write_reg(CAL_REG, cal_reg[_board][i]); 
    }
}

INA226::INA226(uint8_t &addr, board_typeDef &board, TwoWire *wire)
    : _address(addr),
      _board(board),
      _wire(wire)
{
    _wire->begin();
    set_I2C_speed(400000UL);
    // Set the calibration register for each sensor
    for (int i = 0; i < NUM_SENS; i++) { 
        _sel_sensor((sensor_typeDef)i);
        _write_reg(CAL_REG, cal_reg[_board][i]); 
    }
}

void INA226::set_I2C_speed(uint16_t &speed) {
    _wire->setClock((speed == I2C_SPEED_HIGH) ? I2C_SPEED_HIGH : I2C_SPEED_LOW);
}

void INA226::set_addr(uint8_t &addr) { 
    _address = addr; 
}

float INA226::get_pwr(sensor_typeDef sensor) {
    if (_sel_sensor(sensor) != 0) {
        return -1.0f;
    }

    int32_t raw_value = _read_reg(PWR_REG);
    if (raw_value < 0) {
        return -1.0f;
    }

    return static_cast<float>(raw_value) * (lsb_val[_board][sensor] * POWER_LSB_SCALE);
}

int8_t INA226::_sel_sensor(sensor_typeDef sensor) {
    _wire->beginTransmission(MUX_ADDR);
    _wire->write(sensor + MUX_SENSOR_OFFSET);
    return _wire->endTransmission();
}

int8_t INA226::_write_reg(uint8_t &reg, uint16_t &val) {
    int8_t ret = 0;
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->write(val >> 8);
    _wire->write(val & 0xff);
    ret = _wire->endTransmission();
    return ret;
}

int32_t INA226::_read_reg(uint8_t reg) {
    _wire->beginTransmission(_address);
    _wire->write(reg);
    if (_wire->endTransmission() != 0) {
        return -1;
    }

    if (_wire->requestFrom(_address, static_cast<uint8_t>(2)) != 2) {
        return -1;
    }

    uint16_t val = _wire->read();
    val <<= 8;
    val |= _wire->read();

    return static_cast<int32_t>(val);
}

