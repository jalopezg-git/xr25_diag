/* Parsers.cc - Parse Siemens Fenix/Renix diagnostic frames
 *
 * Copyright (C) Javier Lopez-Gomez, 2016
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "Parsers.hh"

/** Use the REGISTER_TYPE(xxx) macro to add new parser types here.
 */
const ParserFactory::ctor_funcs_t ParserFactory::_ctor_funcs = {
    REGISTER_TYPE(Fenix3Parser),
    REGISTER_TYPE(Fenix1Parser),
    REGISTER_TYPE(Fenix52BParser),
};

bool Fenix1Parser::parse_frame(const unsigned char c[], int length, XR25Frame &fra) {
  fra.program_vrsn = c[2];
  fra.calib_vrsn = c[3];
  fra.in_flags = (XR25InFlags)(remap_bit(c[4], 0x02, IN_PARKED) | remap_bit(c[4], 0x04, IN_AC_REQUEST) |
                               remap_bit(c[4], 0x08, IN_THROTTLE_0) | remap_bit(c[4], 0x10, IN_THROTTLE_1) |
                               remap_bit(c[4], 0x20, IN_AC_COMPRES));
  // fra.out_flags     not sent?;
  fra.map = 4 * c[5];
  fra.rpm = ({
    int tmp = (c[11] << 8) | c[10];
    tmp ? (0x00e4e1c0 / tmp) : 0;
  });
  fra.throttle = c[22] / 2.55;
  fra.fault_flags_1 = (XR25FaultFlags1)c[19];
  fra.eng_pinging = c[14];
  fra.injection_us = 2 * ((c[13] << 8) | c[12]);
  fra.advance = c[15];
  fra.fault_flags_0 = (XR25FaultFlags0)c[27];
  fra.fault_fugitive = c[26];
  fra.fault_flags_2 = (XR25FaultFlags2)c[18];
  fra.temp_water = (c[6] / 1.6) - 40;
  fra.temp_air = (c[7] / 1.6) - 40;
  fra.battvalue = (c[8] / 32.0) + 8;
  // fra.lambdavalue      = 6 * c[?];
  fra.idle_regulation = c[16] / 2.55;
  fra.idle_period = c[21];
  fra.eng_pinging_delay = c[28];
  fra.atmos_pressure = 4 * (~c[29] & 0xff);
  fra.spd_km_h = c[20];

  return (length > 29);
}

bool Fenix3Parser::parse_frame(const unsigned char c[], int length, XR25Frame &fra) {
  fra.program_vrsn = c[2];
  fra.calib_vrsn = c[3];
  fra.in_flags = (XR25InFlags)c[4];
  fra.out_flags = (XR25OutFlags)c[5];
  fra.map = 4 * c[6];
  fra.rpm = ({
    int tmp = (c[8] << 8) | c[7];
    tmp ? (0x00e4e1c0 / tmp) : 0;
  });
  fra.throttle = c[9] / 2.55;
  fra.fault_flags_1 = (XR25FaultFlags1)c[10];
  fra.eng_pinging = c[11];
  fra.injection_us = 2 * ((c[13] << 8) | c[12]);
  fra.advance = c[14];
  fra.fault_flags_0 = (XR25FaultFlags0)c[16];
  fra.fault_fugitive = c[17];
  fra.fault_flags_2 = (XR25FaultFlags2)c[18];
  fra.fault_flags_4 = (XR25FaultFlags4)c[19];
  fra.fault_flags_3 = (XR25FaultFlags3)c[20];
  fra.temp_water = (c[21] / 1.6) - 40;
  fra.temp_air = (c[22] / 1.6) - 40;
  fra.battvalue = (c[23] / 32.0) + 8;
  fra.lambdavalue = 6 * c[24];
  fra.idle_regulation = c[25] / 2.55;
  fra.idle_period = c[26];
  fra.eng_pinging_delay = c[27];
  fra.atmos_pressure = 4 * (~c[28] & 0xff);
  fra.afr_correction = c[30];
  fra.spd_km_h = c[34];

  return (length > 34);
}

bool Fenix52BParser::parse_frame(const unsigned char c[], int length, XR25Frame &fra) {
  fra.program_vrsn = c[2];
  fra.calib_vrsn = c[3];
  fra.in_flags = (XR25InFlags)(remap_bit(c[6], 0x80, IN_THROTTLE_0) | remap_bit(c[6], 0x40, IN_THROTTLE_1));
  fra.out_flags = (XR25OutFlags)(remap_bit(c[5], 0x80, OUT_LAMBDA_LOOP));
  fra.map = 4 * c[24];
  fra.rpm = ({
    int tmp = (c[20] << 8) | c[19];
    tmp ? (0x00e4e1c0 / tmp) : 0;
  });
  fra.throttle = c[25] / 2.55;
  // fra.fault_flags_1 = c[?];
  fra.eng_pinging = c[31];
  // fra.injection_us  = 2 * ((c[?] << 8) | c[?]);
  // fra.advance       = c[?];
  // fra.fault_flags_0 = c[?];
  // fra.fault_fugitive = c[?];
  // fra.fault_flags_2 = c[?];
  // fra.fault_flags_4 = c[?];
  // fra.fault_flags_3 = c[?];
  fra.temp_water = (c[27] / 1.6) - 40;
  fra.temp_air = (c[28] / 1.6) - 40;
  fra.battvalue = (c[29] / 32.0) + 8;
  fra.lambdavalue = 6 * c[26];
  // fra.idle_regulation = c[?] / 2.55;
  // fra.idle_period     = c[?];
  // fra.eng_pinging_delay = c[?];
  // fra.atmos_pressure  = 4 * (~c[?] & 0xff);
  // fra.afr_correction  = c[?];
  fra.spd_km_h = c[30];

  return (length == 52);
}
