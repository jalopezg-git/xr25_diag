/* CairoTSPlot.cc - a time-series plot widget
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

#include "CairoTSPlot.hh"

const Gdk::RGBA CairoTSPlot::RGBA_DEFAULT{"#2e7db3"};
const Gdk::RGBA CairoTSPlot::RGBA_ALERT{"#cc0d29"};

void CairoTSPlot::draw_background(void) {
  const int width = get_allocation().get_width(), height = get_allocation().get_height(), y_0 = height - MARGIN_BOTTOM;
  Cairo::TextExtents TE;

  _background = get_window()->create_similar_surface(Cairo::CONTENT_COLOR_ALPHA, width, height);
  auto context = Cairo::Context::create(_background);

  context->set_antialias(Cairo::ANTIALIAS_SUBPIXEL);
  context->set_line_cap(Cairo::LINE_CAP_ROUND);
  context->set_line_width(1);

  // background
  context->set_source_rgba(1, 1, 1, 1);
  context->rectangle(MARGIN_LEFT, MARGIN_TOP, width - MARGIN_LEFT - MARGIN_RIGHT, height - MARGIN_TOP - MARGIN_BOTTOM);
  context->fill();
  context->translate(-0.5f, -0.5f); // avoid AA blur

  // vertical axis scale and borders
  if (_tick_step != 0) {
    for (double i = _value_min; i <= _value_max; i += _tick_step) {
      double _y = yoffset_of(i);
      std::string label = std::to_string(static_cast<int>(i));

      if (i == _value_min || i == _value_max)
        context->set_source_rgba(0.70, 0.71, 0.70, 1);
      else
        context->set_source_rgba(0.89, 0.89, 0.89, 1);
      context->move_to(MARGIN_LEFT, y_0 - _y);
      context->line_to(width - MARGIN_RIGHT + 4, y_0 - _y);
      context->stroke();

      Gdk::Cairo::set_source_rgba(context, _text_rgba);
      context->get_text_extents(label, TE);
      context->move_to(width - MARGIN_RIGHT + 6, y_0 - _y + (TE.height / 2));
      context->show_text(label);
    }
  }
  context->set_source_rgba(0.70, 0.71, 0.70, 1);
  context->move_to(MARGIN_LEFT, MARGIN_TOP);
  context->line_to(MARGIN_LEFT, height - MARGIN_BOTTOM);
  context->move_to(width - MARGIN_RIGHT, MARGIN_TOP);
  context->line_to(width - MARGIN_RIGHT, height - MARGIN_BOTTOM);
  context->stroke();

  // draw the _text string
  Gdk::Cairo::set_source_rgba(context, _text_rgba);
  context->set_font_size(CAIROTSPLOT_FONT_SIZE);
  context->get_text_extents(_text, TE);
  context->move_to((width - TE.width) / 2, MARGIN_TOP / 2);
  context->show_text(_text);
}

bool CairoTSPlot::on_draw(const Cairo::RefPtr<Cairo::Context> &context) {
  const int width = get_allocation().get_width(), height = get_allocation().get_height(),
            y_0 = (height / 2) - MARGIN_BOTTOM, x_offset = (width / 2) - MARGIN_RIGHT;
  const double x_step = (width - MARGIN_LEFT - MARGIN_RIGHT) / static_cast<double>(NUM_POINTS);
  ssize_t data_tail = _data_head.load() - 1, is_alert_region;
  Cairo::TextExtents TE;

  context->set_antialias(Cairo::ANTIALIAS_SUBPIXEL);
  context->translate((width / 2) - 0.5f, (height / 2) - 0.5f);
  context->transform(_transform_matrix);
  context->set_line_cap(Cairo::LINE_CAP_ROUND);
  context->set_line_join(Cairo::LINE_JOIN_ROUND);

  if (!_background)
    draw_background();
  context->set_source(_background, -(width / 2) - 0.5f, // avoid AA blur
                      -(height / 2) - 0.5f);
  context->paint();
  context->set_line_width(1);

  // horizontal axis scale
  auto timepoint = std::chrono::steady_clock::now();
  for (unsigned i = 0; i < NUM_POINTS; ++i) {
    struct value_struct &_s = _circbuf_get(_data, data_tail - i);
    if (_s.has_timepoint) {
      std::chrono::duration<double> diff = timepoint - _s.timepoint;
      std::string label = std::to_string(static_cast<int>(diff.count())) + "s";

      context->set_source_rgba(0.89, 0.89, 0.89, 1);
      context->move_to(x_offset - (x_step * i), y_0 - _data_height);
      context->line_to(x_offset - (x_step * i), y_0 + 4);
      context->stroke();

      Gdk::Cairo::set_source_rgba(context, _text_rgba);
      context->get_text_extents(label, TE);
      context->move_to(x_offset - (x_step * i) - (TE.width / 2), y_0 + 11);
      context->show_text(label);
    }
  }

  // draw plot
  Gdk::Cairo::set_source_rgba(context, (is_alert_region = _circbuf_get(_data, data_tail).is_alerted) ? RGBA_ALERT
                                                                                                     : RGBA_DEFAULT);
  context->set_line_width(2);
  context->move_to(x_offset, y_0 - yoffset_of(_circbuf_get(_data, data_tail).value));
  for (unsigned i = 1; i < NUM_POINTS; ++i) {
    struct value_struct &val = _circbuf_get(_data, data_tail - i);
    if (val.value == HUGE_VAL)
      break;

    context->line_to(x_offset - (x_step * i), y_0 - yoffset_of(val.value));
    if (is_alert_region != val.is_alerted) { // set a different color for alerted region
      context->stroke();
      Gdk::Cairo::set_source_rgba(context, (is_alert_region = val.is_alerted) ? RGBA_ALERT : RGBA_DEFAULT);
      context->move_to(x_offset - (x_step * i), y_0 - yoffset_of(val.value));
    }
  }
  context->stroke();
  return TRUE;
}
