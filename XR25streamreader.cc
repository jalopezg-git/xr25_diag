/* XR25StreamReader.cc - Parse Renault XR25 frame stream
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

#include "XR25streamreader.hh"

#include <condition_variable>
#include <mutex>
#include <tuple>
#include <type_traits>

/** Frame received handler.
 * @param parser The XR25FrameParser to use
 * @param c Translated frame (&quot;0xff 0xff&quot; replaced by &quot;0xff
 *     &quot;)
 * @param length Length of the frame in octets
 * @param fra Reference to the parsed frame
 */
void XR25StreamReader::frame_recv(XR25FrameParser &parser, const unsigned char c[], int length, XR25Frame &fra) {
  parser.parse_frame(c, length, fra);
  if (_post_parse)
    _post_parse(c, length, fra);
}

void XR25StreamReader::read_frames(XR25FrameParser &parser) {
  unsigned char frame[128] = {0xff, 0x00}, c, *p = &frame[1];
  XR25Frame fra;
  std::condition_variable term;
  std::mutex term_m;
  std::atomic_int count(0);
  // thread that updates _frames_per_sec once a second
  std::thread stat_thread([&term, &term_m, &count, this]() {
    std::unique_lock<std::mutex> lock(term_m);
    while (term.wait_for(lock, std::chrono::seconds(1)) == std::cv_status::timeout)
      this->_frames_per_sec = count.exchange(0);
  });

  // thread cancellation clean-up handler
  typedef std::tuple<std::condition_variable &, std::thread &> arg_tuple_t;
  auto args = std::make_tuple(std::ref(term), std::ref(stat_thread));
  pthread_cleanup_push(
      [](void *p) {
        auto args = *static_cast<arg_tuple_t *>(p);
        std::get<0>(args).notify_one();
        std::get<1>(args).join();
      },
      &args);

  while (!_in.eof()) {
    if ((c = _in.get()) == 0xff) {
      if ((c = _in.get()) == 0x00) { /* start of frame */
        if (_synchronized)
          frame_recv(parser, frame, p - frame, fra), count++;
        _synchronized = 1, p = &frame[1];
      } else if (c != 0xff) /* translate 'ff ff' to 'ff' */
        _in.unget();
    }

    if (_synchronized)
      static_cast<unsigned>(p - frame) < std::extent<decltype(frame)>::value ? *p++ = c : _synchronized = 0,
                                                                               _sync_err_count++;
  }
  pthread_cleanup_pop(1);
}
