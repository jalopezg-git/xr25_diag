/* XR25streamreader.hh - Parse Renault XR25 frame stream
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

#ifndef XR25STREAMREADER_HH
#define XR25STREAMREADER_HH

#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <pthread.h>
#include <thread>

enum XR25InFlags : unsigned char {
  IN_AC_REQUEST = 0x02,
  IN_AC_COMPRES = 0x04,
  IN_THROTTLE_0 = 0x08,
  IN_PARKED = 0x10,
  IN_THROTTLE_1 = 0x20,
};

enum XR25OutFlags : unsigned char {
  OUT_PUMP_ENABLE = 0x01,
  OUT_IDLE_REGULATION = 0x02,
  OUT_WASTEGATE_REG = 0x04,
  OUT_LAMBDA_LOOP = 0x08,
  OUT_EGR_ENABLE = 0x20,
  OUT_CHECK_ENGINE = 0x80,
};

enum XR25FaultFlags0 : unsigned char {
  FAULT_WATER_OPEN_C = 0x01,
  FAULT_WATER_SHORT_C = 0x02,
  FAULT_AIR_OPEN_C = 0x04,
  FAULT_AIR_SHORT_C = 0x08,
  FAULT_TPS_LOW = 0x40,
  FAULT_TPS_HIGH = 0x80,
};

enum XR25FaultFlags1 : unsigned char {
  FAULT_MAP = 0x04,
  FAULT_SPD_SENSOR = 0x10,
  FAULT_LAMBDA_TMP = 0x20,
  FAULT_LAMBDA = 0x80,
};

enum XR25FaultFlags2 : unsigned char {
  FAULT_EEPROM_CHECKSUM = 0x20,
  FAULT_PROG_CHECKSUM = 0x80,
};

enum XR25FaultFlags3 : unsigned char {
  FAULT_INJECTORS = 0x10,
};

enum XR25FaultFlags4 : unsigned char {
  FAULT_PUMP = 0x01,
  FAULT_WASTEGATE = 0x04,
  FAULT_EGR = 0x08,
  FAULT_IDLE_REG = 0x20,
};

/* XR25 frames start with 0xff 0x00; 0xff ocurrences in the frame sent on the
 * wire as 0xff 0xff.
 *
 * 0xff 0x00 0xaa 0xbb 0xcc 0xdd  .  .  .  0xff 0x00  .  .  .
 * |-------|   |   |    |    |---- byte 3  |-------|
 *  header1    |   |    |--------- byte 2   header2
 *             |   |-------------- byte 1
 *             |------------------ byte 0
 */
struct XR25Frame {
  unsigned char program_vrsn;
  unsigned char calib_vrsn;

  XR25InFlags in_flags;
  XR25OutFlags out_flags;

  int map;
  int rpm;
  int throttle;

  XR25FaultFlags1 fault_flags_1;

  unsigned char eng_pinging;
  int injection_us;
  int advance;

  XR25FaultFlags0 fault_flags_0;

  unsigned char fault_fugitive;

  XR25FaultFlags2 fault_flags_2;
  XR25FaultFlags4 fault_flags_4;
  XR25FaultFlags3 fault_flags_3;

  float temp_water;
  float temp_air;
  float battvalue;
  float lambdavalue;
  int idle_regulation;
  int idle_period;
  unsigned char eng_pinging_delay;
  int atmos_pressure;
  unsigned char afr_correction;
  int spd_km_h;
};

/* Equivalent to '(x & bit1) ? bit2 : 0' but this is faster;
 * borrowed from include/linux/mman.h: _calc_vm_trans.
 */
#define remap_bit(x, bit1, bit2)                                                                                       \
  ((bit1) <= (bit2) ? ((x) & (bit1)) * ((bit2) / (bit1)) : ((x) & (bit1)) / ((bit1) / (bit2)))

class XR25FrameParser {
public:
  /** Parses a frame and return a 'struct XR25Frame'.
   * @param c Translated frame (&quot;0xff 0xff&quot; converted to 0xff)
   * @param length Length in octets
   * @param fra Reference to 'struct XR25Frame'; implementors write here
   * @return true if the frame @a c was parsed
   */
  virtual bool parse_frame(const unsigned char c[], int length, XR25Frame &fra) = 0;
};

class XR25StreamReader {
private:
  typedef std::function<void(const unsigned char[], int, XR25Frame &)> post_parse_t;

  std::istream &_in;
  std::atomic_bool _synchronized;
  std::atomic_int _sync_err_count, _frames_per_sec, _fra_count;
  post_parse_t _post_parse;
  std::unique_ptr<std::thread> _thrd;

  void frame_recv(XR25FrameParser &parser, const unsigned char[], int, XR25Frame &);
  void read_frames(XR25FrameParser &parser);

public:
  XR25StreamReader(std::istream &s, post_parse_t p = nullptr)
      : _in(s), _synchronized(0), _sync_err_count(0), _frames_per_sec(0), _fra_count(0), _post_parse(p),
        _thrd(nullptr) {}
  ~XR25StreamReader() { stop(); }

  bool is_synchronized() { return _synchronized.load(); }
  int get_sync_err_count() { return _sync_err_count.load(); }
  int get_frames_per_sec() { return _frames_per_sec.load(); }
  int get_fra_count() { return _fra_count.load(); }

  /** Read frames non-blocking; call stop() to cancel thread
   * @param parser The XR25FrameParser to use
   */
  void start(XR25FrameParser &parser) {
    if (!_thrd)
      _thrd = std::make_unique<std::thread>([&parser, this]() { this->read_frames(parser); });
  }

  /** Stop internal thread; see start()
   */
  inline void stop() {
    if (_thrd) {
      pthread_cancel(_thrd->native_handle());
      _thrd->join();
    }
  }
};

#endif /* XR25STREAMREADER_HH */
