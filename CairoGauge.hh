/* CairoGauge.hh - Gtk::DrawingArea-based analog gauge widget
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

#ifndef CAIROGAUGE_HH
#define CAIROGAUGE_HH

#include <cairomm/context.h>
#include <cmath>
#include <functional>
#include <gtkmm.h>
#include <string>

#define CAIROGAUGE_FONT_SIZE 14

class CairoGauge : public Gtk::DrawingArea {
protected:
  typedef std::function<double(void *)> sample_fn_t;

  std::string _text;
  sample_fn_t _sample_fn;
  double _value, _value_max, _tick_step;
  Gdk::RGBA _face_rgba;

  bool on_draw(const Cairo::RefPtr<Cairo::Context> &context) override;

public:
  /** Construct a CairoGauge object
   * @param text Text rendered below the gauge
   * @param fn std::function<double(void *)> that returns the next value
   * @param _M Maximum value of any sample
   * @param step Draw ticks using @a step increments
   */
  CairoGauge(std::string text, sample_fn_t fn, double _M, double step = 0)
      : _text(text), _sample_fn(fn), _value(0), _value_max(_M), _tick_step(step),
        _face_rgba(get_style_context()->get_color(Gtk::STATE_FLAG_NORMAL)) {}
  virtual ~CairoGauge() {}

  /** Call the @a fn function (constructor argument) and update gauge with
   * the returned value.
   */
  void update(void *arg) {
    auto v = _sample_fn(arg);
    if (v != _value) {
      _value = v;
      get_window()->invalidate_rect(Gdk::Rectangle(0, 0, get_allocation().get_width(), get_allocation().get_height()),
                                    FALSE);
    }
  }

protected:
  double angle_of(double value) { return 5 * M_PI_4 - (value / _value_max * 3 * M_PI_2); }
};

#endif /* CAIROGAUGE_HH */
