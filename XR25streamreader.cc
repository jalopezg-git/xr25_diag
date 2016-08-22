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

void XR25StreamReader::frame_recv(XR25FrameParser &parser, const unsigned char c[], int length, XR25Frame &fra) {
  parser.parse_frame(c, length, fra);
  if (_post_parse)
    _post_parse(c, length, fra);
}

void XR25StreamReader::read_frames(XR25FrameParser &parser) {
  unsigned char frame[128] = {0xff, 0x00}, c, *p = &frame[1];
  XR25Frame fra;
  while (!_in.eof()) {
    if ((c = _in.get()) == 0xff) {
      if ((c = _in.get()) == 0x00) { /* start of frame */
        if (_synchronized)
          frame_recv(parser, frame, p - frame, fra);
        _synchronized = 1, p = &frame[1];
      } else if (c != 0xff) /* translate 'ff ff' to 'ff' */
        _in.unget();
    }

    if (_synchronized)
      (p - frame) < ARRAY_SIZE(frame) ? *p++ = c : _synchronized = 0;
  }
}
