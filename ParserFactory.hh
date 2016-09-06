/* ParserFactory.hh - instantiate objects of type 'XXXparser'
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

#ifndef PARSERFACTORY_HH
#define PARSERFACTORY_HH

#include "XR25streamreader.hh"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

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

/** Use the REGISTER_TYPE(xxx) macro to add new parser types here.
 */
const ParserFactory::ctor_funcs_t ParserFactory::_ctor_funcs = {
    REGISTER_TYPE(Fenix3Parser),
};

#endif /* PARSERFACTORY_HH */
