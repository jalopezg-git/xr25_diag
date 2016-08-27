/* CairoGauge.cc - Gtk::DrawingArea-based analog gauge widget
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

#include "CairoGauge.hh"

bool CairoGauge::on_draw(const Cairo::RefPtr<Cairo::Context> &context) {
  const int width = get_allocation().get_width(), height = get_allocation().get_height(),
            radius = std::min(width, height) / 2;
  Cairo::TextExtents TE;

  // setup
  context->set_antialias(Cairo::ANTIALIAS_SUBPIXEL);
  context->translate(width / 2, height / 2);
  context->transform(_transform_matrix);
  context->set_line_cap(Cairo::LINE_CAP_ROUND);
  context->set_line_width(2);
  context->set_font_size(CAIROGAUGE_FONT_SIZE);
  Gdk::Cairo::set_source_rgba(context, _face_rgba);

  // draw face
  context->arc_negative(0, 0, 0.8 * radius, M_PI_4, 3 * M_PI_4);
  context->stroke();
  if (_tick_step != 0)
    for (double i = 0, _r1 = 0.78 * radius, _r2 = 0.82 * radius, _r3 = 0.9 * radius; i <= _value_max; i += _tick_step) {
      double angle = angle_of(i);
      std::string label = std::to_string(i);

      context->move_to(_r1 * cos(angle), _r1 * sin(angle));
      context->line_to(_r2 * cos(angle), _r2 * sin(angle));
      context->stroke();

      context->get_text_extents(label, TE);
      context->move_to((_r3 * cos(angle)) - TE.width / 2, _r3 * sin(angle));
      context->show_text(label);
    }
  context->get_text_extents(_text, TE);
  context->move_to(-TE.width / 2, 0.7 * radius);
  context->show_text(_text);

  // draw needle
  double angle = angle_of(_value);
  context->set_line_width(3);
  context->set_source_rgba(1, 0.2, 0.2, 1);
  context->move_to(0, 0);
  context->line_to(0.76 * radius * cos(angle), 0.76 * radius * sin(angle));
  context->stroke();
  context->arc(0, 0, 0.03 * radius, 0, 2 * M_PI);
  context->fill();

  return TRUE;
}
