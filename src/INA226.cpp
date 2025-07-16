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

INA226::INA226(const board_typeDef &board, TwoWire *wire)
    : _address(STD_ADDR),
      _board(board),
      _wire(wire)
{
    _wire->begin();
    set_I2C_speed(400000UL);
    for (int i = 0; i < NUM_SENS; i++) { 
        _sel_sensor((sensor_typeDef)i);
        _write_reg(CAL_REG, cal_reg[_board][i]); 
    }
}

INA226::INA226(const uint8_t &addr, const board_typeDef &board, TwoWire *wire)
    : _address(addr),
      _board(board),
      _wire(wire)
{
    _wire->begin();
    set_I2C_speed(400000UL);
    for (int i = 0; i < NUM_SENS; i++) { 
        _sel_sensor((sensor_typeDef)i);
        _write_reg(CAL_REG, cal_reg[_board][i]); 
    }
}

const void INA226::set_I2C_speed(const uint16_t &speed) {
    _wire->setClock((speed == 400000UL) ? 400000UL : 100000UL);
}

const void INA226::set_addr(const uint8_t &addr) { _address = addr; }

const float INA226::get_pwr(const sensor_typeDef &sensor) {
    _sel_sensor(sensor);
    float pwr = (float)_read_reg(PWR_REG) * (lsb_val[_board][sensor] * 25);
    return pwr;
}

void INA226::_sel_sensor(uint8_t sensor) {
    _wire->beginTransmission(MUX_ADDR);
#if defined(BOARD_ZCU106)
    _wire->write(sensor + 0x04);
#elif defined(BOARD_ZCU102)
    _wire->write(1 << sensor);
#else
    _wire->write(1 << sensor);
#endif
    _wire->endTransmission();
}

const int8_t INA226::_write_reg(const uint8_t &reg, const uint16_t &val) {
    int8_t ret = 0;
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->write(val >> 8);
    _wire->write(val & 0xff);
    ret = _wire->endTransmission();
    return ret;
}

int32_t INA226::_read_reg(const uint8_t &reg) {
    int8_t ret = 0;
    uint16_t val = 0;

    _wire->beginTransmission(_address);
    _wire->write(reg);
    ret = _wire->endTransmission();

    if (ret == 0) {
        if (_wire->requestFrom(_address, (uint8_t)2) == 2) {
            val = _wire->read();
            val <<= 8;
            val |= _wire->read();
        } else { ret = -1; }
    }
    return (ret == -1 ? (int32_t)ret : (int32_t)val);
}
