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

  unsigned char in_flags;
#define IN_AC_REQUEST 0x02
#define IN_AC_COMPRES 0x04
#define IN_THROTTLE_0 0x08
#define IN_NOT_PARKED 0x10
#define IN_THROTTLE_1 0x20

  unsigned char out_flags;
#define OUT_PUMP_ENABLE 0x01
#define OUT_IDLE_REGULATION 0x02
#define OUT_WASTEGATE_REG 0x04
#define OUT_EGR_ENABLE 0x20
#define OUT_CHECK_ENGINE 0x80

  int map;
  int rpm;
  int throttle;

  unsigned char fault_flags_1;
#define FAULT_MAP 0x04
#define FAULT_SPD_SENSOR 0x10
#define FAULT_LAMBDA_TMP 0x20
#define FAULT_LAMBDA 0x80

  unsigned char eng_pinging;
  int injection_us;
  int advance;

  unsigned char fault_flags_0;
#define FAULT_WATER_OPEN_C 0x01
#define FAULT_WATER_SHORT_C 0x02
#define FAULT_AIR_OPEN_C 0x04
#define FAULT_AIR_SHORT_C 0x08
#define FAULT_TPS_LOW 0x40
#define FAULT_TPS_HIGH 0x80

  unsigned char fault_fugitive;

  unsigned char fault_flags_2;
#define FAULT_EEPROM_CHECKSUM 0x20
#define FAULT_PROG_CHECKSUM 0x80

  unsigned char fault_flags_4;
#define FAULT_NOT_PUMP 0x01
#define FAULT_NOT_WASTEGATE 0x04
#define FAULT_NOT_EGR 0x08
#define FAULT_NOT_IDLE_REG 0x20

  unsigned char fault_flags_3;
#define FAULT_INJECTORS 0x10

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

#define ARRAY_SIZE(_a) (unsigned int)(sizeof(_a) / sizeof(_a[0]))

class XR25StreamReader {
private:
  typedef std::function<void(const unsigned char[], int, XR25Frame &)> post_parse_t;

  std::istream &_in;
  std::atomic_bool _synchronized;
  std::atomic_int _sync_err_count, _frames_per_sec;
  post_parse_t _post_parse;
  std::unique_ptr<std::thread> _thrd;

  void frame_recv(XR25FrameParser &parser, const unsigned char[], int, XR25Frame &);
  void read_frames(XR25FrameParser &parser);

public:
  XR25StreamReader(std::istream &s, post_parse_t p = nullptr)
      : _in(s), _synchronized(0), _sync_err_count(0), _frames_per_sec(0), _post_parse(p), _thrd(nullptr) {}
  ~XR25StreamReader() { stop(); }

  bool is_synchronized() { return _synchronized.load(); }
  int get_sync_err_count() { return _sync_err_count.load(); }
  int get_frames_per_sec() { return _frames_per_sec.load(); }

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
