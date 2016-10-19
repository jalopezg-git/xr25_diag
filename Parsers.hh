/* Parsers.hh - Parse Siemens Fenix/Renix diagnostic frames
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

#ifndef PARSERS_HH
#define PARSERS_HH

#include "XR25streamreader.hh"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

/// Parse Siemens Fenix1 frames.  This parser is not tested; if you test it, please report feedback!
class Fenix1Parser : public XR25FrameParser {
public:
  bool parse_frame(const unsigned char c[], int length, XR25Frame &fra) override;
};

/// Parse Siemens Fenix3 frames.
class Fenix3Parser : public XR25FrameParser {
public:
  bool parse_frame(const unsigned char c[], int length, XR25Frame &fra) override;
};

/// Parse Siemens Fenix 52-byte frames sent, e.g. by the ECU mounted on the Renault R21 2.0TXI.
/// This parser is incomplete; if you know the meaning of the remaining bytes, please contribute!
class Fenix52BParser : public XR25FrameParser {
public:
  bool parse_frame(const unsigned char c[], int length, XR25Frame &fra) override;
};

/// Helper class to construct a `FenixXyzParser` by name
class ParserFactory {
public:
  typedef std::shared_ptr<XR25FrameParser> parser_ptr_t;

private:
  typedef std::unordered_map<std::string, std::function<parser_ptr_t()>> ctor_funcs_t;
  static const ctor_funcs_t _ctor_funcs;

public:
  static parser_ptr_t create(const std::string &_typename) { return _ctor_funcs.at(_typename)(); }
  static const ctor_funcs_t &get_registered_types() { return _ctor_funcs; }
};

#define REGISTER_TYPE(_typename)                                                                                       \
  {                                                                                                                    \
    #_typename, []() { return std::make_shared<_typename>(); }                                                         \
  }

#endif /* PARSERS_HH */
