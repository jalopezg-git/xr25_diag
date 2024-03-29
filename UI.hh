/* UI.hh - xr25_diag user interface (gtkmm)
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

#ifndef UI_HH
#define UI_HH

#include "CairoGauge.hh"
#include "CairoTSPlot.hh"
#include "XR25streamreader.hh"

#include <gtkmm.h>
#include <mutex>
#include <pangomm/context.h>
#include <vector>

class UI {
private:
  Glib::RefPtr<Gtk::Application> _application;
  Glib::RefPtr<Gtk::Builder> _builder;
  XR25StreamReader _xr25reader;
  const XR25FrameParser &_fp;

  XR25Frame _last_recv;
  std::mutex _last_recv_mutex;

  Gtk::Label *_hb_sync_err, *_hb_fra_s;
  Gtk::Image *_hb_is_sync;
  Gtk::HeaderBar *_hb;
  Gtk::Notebook *_notebook;

  enum EntryWidgets {
    E_PROGRAM_VRSN = 0,
    E_CALIB_VRSN,
    E_MAP,
    E_RPM,
    E_THROTTLE,
    E_ENG_PINGING, /* 5  */
    E_INJECTION_US,
    E_ADVANCE,
    ETEMP_WATER,
    ETEMP_AIR,
    E_BATT_V, /* 10 */
    E_LAMBDA_V,
    E_IDLE_REGULATION,
    E_IDLE_PERIOD,
    E_ENG_PINGING_DELAY,
    E_ATMOS_PRESSURE, /* 15 */
    E_AFR_CORRECTION,
    E_SPD_KM_H,
    E_COUNT, // add new elements before this line
  };
  enum FlagWidgets {
    F_IN_AC_REQUEST = 0,
    F_IN_AC_COMPRES,
    F_IN_THROTTLE_0,
    F_IN_PARKED,
    F_IN_THROTTLE_1,
    F_OUT_PUMP_ENABLE, /* 5  */
    F_OUT_IDLE_REGULATION,
    F_OUT_WASTEGATE_REG,
    F_OUT_EGR_ENABLE,
    F_OUT_CHECK_ENGINE,
    F_FAULT_MAP, /* 10 */
    F_FAULT_SPD_SENSOR,
    F_FAULT_LAMBDA_TMP,
    F_FAULT_LAMBDA,
    F_FAULT_WATER_OPEN_C,
    F_FAULT_WATER_SHORT_C, /* 15 */
    F_FAULT_AIR_OPEN_C,
    F_FAULT_AIR_SHORT_C,
    F_FAULT_TPS_LOW,
    F_FAULT_TPS_HIGH,
    F_FAULT_F_WATER_OPEN_C, /* 20 */
    F_FAULT_F_WATER_SHORT_C,
    F_FAULT_F_AIR_OPEN_C,
    F_FAULT_F_AIR_SHORT_C,
    F_FAULT_F_TPS_LOW,
    F_FAULT_F_TPS_HIGH, /* 25 */
    F_FAULT_EEPROM_CHECKSUM,
    F_FAULT_PROG_CHECKSUM,
    F_FAULT_PUMP,
    F_FAULT_WASTEGATE,
    F_FAULT_EGR, /* 30 */
    F_FAULT_IDLE_REG,
    F_FAULT_INJECTORS,
    F_OUT_LAMBDA_LOOP,
    F_COUNT, // add new elements before this line
  };

  Gtk::Entry *_entry[E_COUNT];
  Gtk::Arrow *_flag[F_COUNT];

  std::vector<CairoGauge> _gauge = {
      {"RPM", [](void *p) { return static_cast<XR25Frame *>(p)->rpm; }, 7000, 500, 2},
      {"km/h", [](void *p) { return static_cast<XR25Frame *>(p)->spd_km_h; }, 240, 10, 2},
      {"Temp (C)", [](void *p) { return static_cast<XR25Frame *>(p)->temp_water; }, 120, 30, 1},
      {"Battery (V)", [](void *p) { return static_cast<XR25Frame *>(p)->battvalue; }, 18, 1, 2},
      {"MAP (mbar)", [](void *p) { return static_cast<XR25Frame *>(p)->map; }, 1020, 255, 1},
      {"Air Temp(C)", [](void *p) { return static_cast<XR25Frame *>(p)->temp_air; }, 90, 30, 1},
      {"Lambda (mV)", [](void *p) { return static_cast<XR25Frame *>(p)->lambdavalue; }, 1530, 255, 1},
  };
  std::vector<CairoTSPlot> _plot = {
      {"RPM", [](void *p, bool &is_alerted) { return static_cast<XR25Frame *>(p)->rpm; }, 0, 6000, 1500},
      {"MAP (mbar)", [](void *p, bool &is_alerted) { return static_cast<XR25Frame *>(p)->map; }, 0, 1020, 255},
      {"Throttle",
       [](void *p, bool &is_alerted) {
         is_alerted = static_cast<XR25Frame *>(p)->in_flags & IN_THROTTLE_0;
         return static_cast<XR25Frame *>(p)->throttle;
       },
       0, 100, 20},
      {"Lambda (mV)",
       [](void *p, bool &is_alerted) {
         is_alerted = ~static_cast<XR25Frame *>(p)->out_flags & OUT_LAMBDA_LOOP;
         return static_cast<XR25Frame *>(p)->lambdavalue;
       },
       0, 1020, 255},
      {"Battery (V)",
       [](void *p, bool &is_alerted) {
         is_alerted = static_cast<XR25Frame *>(p)->battvalue > 15;
         return static_cast<XR25Frame *>(p)->battvalue;
       },
       8, 16, 2},
      {"Temp (C)", [](void *p, bool &is_alerted) { return static_cast<XR25Frame *>(p)->temp_water; }, 0, 120, 30},
  };

  enum { GRID_DASHBOARD = 0, GRID_PLOTS, _GRID_COUNT };
  std::vector<Gdk::Rectangle> _g_rect[_GRID_COUNT] = {
      {
          // GRID_DASHBOARD
          {0, 0, 1, 2}, // RPM
          {2, 0, 1, 2}, // km/h
          {0, 2, 1, 2}, // Temp (C)
          {2, 2, 1, 2}, // Battery (V)
          {1, 0, 1, 1}, // MAP (mbar)
          {1, 2, 1, 2}, // Air Temp(C)
          {1, 1, 1, 1}, // Lambda (mV)
      },
      {
          // GRID_PLOTS
          {0, 0, 1, 1}, // RPM
          {0, 1, 1, 1}, // MAP (mbar)
          {0, 2, 1, 1}, // Throttle
          {1, 0, 1, 1}, // Lambda (mV)
          {1, 1, 1, 1}, // Battery (V)
          {1, 2, 1, 1}, // Temp (C)
      },
  };

  /** Attach a vector of widgets to a GtkGrid; the left, top, width and
   * height arguments for the attach() call are taken from @a _r vector.
   * @param _grid The GtkGrid to attach widgets to
   * @param _r A vector of Gdk::Rectangle that specifies the position of
   *     the widgets in the @a _vec vector
   * @param _vec The vector of widgets
   */
  template <class _T>
  inline void attach_widgets_to_grid(Gtk::Grid *_grid, std::vector<Gdk::Rectangle> &_r, std::vector<_T> &_vec) {
    for (size_t i = 0; i < _r.size(); ++i)
      _grid->attach(_vec[i], _r[i].get_x(), _r[i].get_y(), _r[i].get_width(), _r[i].get_height());
    _grid->show_all();
  }

  void update_page_diagnostic(XR25Frame &);
  void update_page_dashboard(XR25Frame &);
  void update_page_plots(XR25Frame &);
  /** Update current notebook page, see 'update_page_xxx()' member
   * functions; called UI_UPDATE_PAGE_HZ times per sec.
   */
  bool update_page();

  /** Update headerbar widgets; called UI_UPDATE_HEADER_HZ times per sec
   */
  bool update_header();

public:
  /// The update frequency for notebook pages in the main window
  static constexpr unsigned UI_UPDATE_PAGE_HZ = 16;
  /// The update frequency for widgets embedded in the window decoration
  static constexpr unsigned UI_UPDATE_HEADER_HZ = 1;

  UI(Glib::RefPtr<Gtk::Application> _a, Glib::RefPtr<Gtk::Builder> _b, std::istream &_is, const XR25FrameParser &_p);
  ~UI() {}

  void run();
};

#endif /* UI_HH */
