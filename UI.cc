/* UI.cc  - xr25_diag user interface (gtkmm)
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

#include "UI.hh"

UI::UI(Glib::RefPtr<Gtk::Application> _a, Glib::RefPtr<Gtk::Builder> _b, std::istream &_is, const XR25FrameParser &_p)
    : _application(_a), _builder(_b), _xr25reader(_is,
                                                  [this](const unsigned char c[], int l, XR25Frame &fra) {
                                                    this->_last_recv_mutex.lock();
                                                    this->_last_recv = fra;
                                                    this->_last_recv_mutex.unlock();

                                                    // call CairoTSPlots::sample() passing fra
                                                    auto _ts = std::chrono::steady_clock::now();
                                                    for (auto &i : _plot)
                                                      i.sample(&fra, _ts);
                                                  }),
      _fp(_p), _last_recv() {
  _builder->get_widget("mw_hb_sync_err", _hb_sync_err);
  _builder->get_widget("mw_hb_fra_s", _hb_fra_s);
  _builder->get_widget("mw_hb_is_sync", _hb_is_sync);
  _builder->get_widget("mw_hb", _hb);
  _builder->get_widget("mw_notebook", _notebook);

  for (int i = 0; i < E_COUNT; i++)
    _builder->get_widget("mw_e" + std::to_string(i), _entry[i]);
  for (int i = 0; i < F_COUNT; i++)
    _builder->get_widget("mw_f" + std::to_string(i), _flag[i]);
}

void UI::run() {
  Gtk::Grid *dash_grid, *plot_grid;

  _builder->get_widget("mw_dash_grid", dash_grid);
  _builder->get_widget("mw_plot_grid", plot_grid);
  attach_widgets_to_grid<CairoGauge>(dash_grid, _g_rect[GRID_DASHBOARD], _gauge);
  attach_widgets_to_grid<CairoTSPlot>(plot_grid, _g_rect[GRID_PLOTS], _plot);

  /* connect signals */
  Glib::signal_timeout().connect(sigc::mem_fun(*this, &UI::update_page), 1000 / UI_UPDATE_PAGE_HZ);
  Glib::signal_timeout().connect(sigc::mem_fun(*this, &UI::update_header), 1000 / UI_UPDATE_HEADER_HZ);

  Gtk::Button *about_button = nullptr;
  _builder->get_widget("mw_about_button", about_button);
  about_button->signal_clicked().connect([this]() {
    Gtk::AboutDialog *ad = nullptr;
    _builder->get_widget("about_dialog", ad);
    ad->set_version(XR25DIAG_VERSION);
    ad->run(), ad->hide();
  });

  Gtk::CheckButton *hud = nullptr;
  _builder->get_widget("mw_hud", hud);
  hud->signal_toggled().connect([hud, this]() {
    auto m = hud->get_active() ? Cairo::Matrix{1, 0, 0, -1, 0, 0} : Cairo::identity_matrix();
    for (auto &i : _gauge)
      i.set_transform_matrix(m);
    for (auto &i : _plot)
      i.set_transform_matrix(m);
  });

  _xr25reader.start(const_cast<XR25FrameParser &>(_fp));

  Gtk::Window *main_window = nullptr;
  _builder->get_widget("main_window", main_window);
  _application->run(*main_window);
}

bool UI::update_page() {
  sigc::bound_mem_functor1<void, UI, XR25Frame &> _fn[] = {
      sigc::mem_fun(*this, &UI::update_page_diagnostic),
      sigc::mem_fun(*this, &UI::update_page_plots),
      sigc::mem_fun(*this, &UI::update_page_dashboard),
  };

  _last_recv_mutex.lock();
  XR25Frame fra = _last_recv;
  _last_recv_mutex.unlock();

  _fn[_notebook->get_current_page()](fra);
  return TRUE;
}

bool UI::update_header() {
  _hb_sync_err->set_text(std::to_string(_xr25reader.get_sync_err_count()));
  _hb_fra_s->set_text(std::to_string(_xr25reader.get_frames_per_sec()));
  _hb_is_sync->set_from_icon_name(_xr25reader.is_synchronized() ? "gtk-yes" : "gtk-no", Gtk::ICON_SIZE_BUTTON);
  _hb->set_subtitle("Frame count: " + std::to_string(_xr25reader.get_fra_count()));

  return TRUE;
}

void UI::update_page_diagnostic(XR25Frame &fra) {
  auto update_entry = [&](unsigned index, auto data) { _entry[index]->set_text(std::to_string(data)); };
  auto update_flag = [&](unsigned index, auto data) {
    _flag[index]->set(data ? Gtk::ARROW_RIGHT : Gtk::ARROW_NONE, Gtk::SHADOW_OUT);
  };

  update_entry(E_PROGRAM_VRSN, fra.program_vrsn);
  update_entry(E_CALIB_VRSN, fra.calib_vrsn);
  update_entry(E_MAP, fra.map);
  update_entry(E_RPM, fra.rpm);
  update_entry(E_THROTTLE, fra.throttle);
  update_entry(E_ENG_PINGING, fra.eng_pinging);
  update_entry(E_INJECTION_US, fra.injection_us);
  update_entry(E_ADVANCE, fra.advance);
  update_entry(ETEMP_WATER, fra.temp_water);
  update_entry(ETEMP_AIR, fra.temp_air);
  update_entry(E_BATT_V, fra.battvalue);
  update_entry(E_LAMBDA_V, fra.lambdavalue);
  update_entry(E_IDLE_REGULATION, fra.idle_regulation);
  update_entry(E_IDLE_PERIOD, fra.idle_period);
  update_entry(E_ENG_PINGING_DELAY, fra.eng_pinging_delay);
  update_entry(E_ATMOS_PRESSURE, fra.atmos_pressure);
  update_entry(E_AFR_CORRECTION, fra.afr_correction);
  update_entry(E_SPD_KM_H, fra.spd_km_h);

  update_flag(F_IN_AC_REQUEST, fra.in_flags & IN_AC_REQUEST);
  update_flag(F_IN_AC_COMPRES, fra.in_flags & IN_AC_COMPRES);
  update_flag(F_IN_THROTTLE_0, fra.in_flags & IN_THROTTLE_0);
  update_flag(F_IN_PARKED, fra.in_flags & IN_PARKED);
  update_flag(F_IN_THROTTLE_1, fra.in_flags & IN_THROTTLE_1);
  update_flag(F_OUT_PUMP_ENABLE, fra.out_flags & OUT_PUMP_ENABLE);
  update_flag(F_OUT_IDLE_REGULATION, fra.out_flags & OUT_IDLE_REGULATION);
  update_flag(F_OUT_WASTEGATE_REG, fra.out_flags & OUT_WASTEGATE_REG);
  update_flag(F_OUT_EGR_ENABLE, fra.out_flags & OUT_EGR_ENABLE);
  update_flag(F_OUT_CHECK_ENGINE, fra.out_flags & OUT_CHECK_ENGINE);
  update_flag(F_OUT_LAMBDA_LOOP, fra.out_flags & OUT_LAMBDA_LOOP);
  update_flag(F_FAULT_MAP, fra.fault_flags_1 & FAULT_MAP);
  update_flag(F_FAULT_SPD_SENSOR, fra.fault_flags_1 & FAULT_SPD_SENSOR);
  update_flag(F_FAULT_LAMBDA_TMP, fra.fault_flags_1 & FAULT_LAMBDA_TMP);
  update_flag(F_FAULT_LAMBDA, fra.fault_flags_1 & FAULT_LAMBDA);
  update_flag(F_FAULT_WATER_OPEN_C, fra.fault_flags_0 & FAULT_WATER_OPEN_C);
  update_flag(F_FAULT_WATER_SHORT_C, fra.fault_flags_0 & FAULT_WATER_SHORT_C);
  update_flag(F_FAULT_AIR_OPEN_C, fra.fault_flags_0 & FAULT_AIR_OPEN_C);
  update_flag(F_FAULT_AIR_SHORT_C, fra.fault_flags_0 & FAULT_AIR_SHORT_C);
  update_flag(F_FAULT_TPS_LOW, fra.fault_flags_0 & FAULT_TPS_LOW);
  update_flag(F_FAULT_TPS_HIGH, fra.fault_flags_0 & FAULT_TPS_HIGH);
  update_flag(F_FAULT_F_WATER_OPEN_C, fra.fault_fugitive & FAULT_WATER_OPEN_C);
  update_flag(F_FAULT_F_WATER_SHORT_C, fra.fault_fugitive & FAULT_WATER_SHORT_C);
  update_flag(F_FAULT_F_AIR_OPEN_C, fra.fault_fugitive & FAULT_AIR_OPEN_C);
  update_flag(F_FAULT_F_AIR_SHORT_C, fra.fault_fugitive & FAULT_AIR_SHORT_C);
  update_flag(F_FAULT_F_TPS_LOW, fra.fault_fugitive & FAULT_TPS_LOW);
  update_flag(F_FAULT_F_TPS_HIGH, fra.fault_fugitive & FAULT_TPS_HIGH);
  update_flag(F_FAULT_EEPROM_CHECKSUM, fra.fault_flags_2 & FAULT_EEPROM_CHECKSUM);
  update_flag(F_FAULT_PROG_CHECKSUM, fra.fault_flags_2 & FAULT_PROG_CHECKSUM);
  update_flag(F_FAULT_PUMP, fra.fault_flags_4 & FAULT_PUMP);
  update_flag(F_FAULT_WASTEGATE, fra.fault_flags_4 & FAULT_WASTEGATE);
  update_flag(F_FAULT_EGR, fra.fault_flags_4 & FAULT_EGR);
  update_flag(F_FAULT_IDLE_REG, fra.fault_flags_4 & FAULT_IDLE_REG);
  update_flag(F_FAULT_INJECTORS, fra.fault_flags_3 & FAULT_INJECTORS);
}

void UI::update_page_dashboard(XR25Frame &fra) {
  for (auto &i : _gauge)
    i.update(&fra);
}

void UI::update_page_plots(XR25Frame &fra) {
  for (auto &i : _plot)
    i.update();
}
