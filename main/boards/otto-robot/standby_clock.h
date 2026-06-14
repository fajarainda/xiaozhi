#pragma once

// standby_clock.h  (VERSI RADIKAL v5 — BUMN Neural OS)
// Member baru: hour_label_, min_label_, colon_label_,
//              date_num_label_, sweep_line_

#include <lvgl.h>
#include <esp_timer.h>
#include <time.h>

class StandbyClock {
public:
    explicit StandbyClock(lv_obj_t* parent_obj, int width, int height);
    ~StandbyClock();

    StandbyClock(const StandbyClock&)            = delete;
    StandbyClock& operator=(const StandbyClock&) = delete;

    lv_obj_t* GetContainer() const { return container_; }

private:
    lv_obj_t*   container_       = nullptr;
    lv_obj_t*   time_label_      = nullptr;   // unused, kept for compat
    lv_obj_t*   hour_label_      = nullptr;   // HH (hijau)
    lv_obj_t*   min_label_       = nullptr;   // MM (putih)
    lv_obj_t*   colon_label_     = nullptr;   // : (biru, blink)
    lv_obj_t*   day_label_       = nullptr;   // nama hari
    lv_obj_t*   date_label_      = nullptr;   // "MEI / 2026"
    lv_obj_t*   date_num_label_  = nullptr;   // angka tanggal besar
    lv_obj_t*   sec_bar_         = nullptr;
    lv_obj_t*   sec_label_       = nullptr;
    lv_obj_t*   uptime_label_    = nullptr;
    lv_obj_t*   sweep_line_      = nullptr;   // animasi sweep
    lv_timer_t* tick_timer_      = nullptr;

    int width_;
    int height_;

    static void TickCb(lv_timer_t* timer);
    void        Tick();
};
