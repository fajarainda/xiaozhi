#pragma once

// otto_emoji_display.h  (VERSI MODIFIKASI)
// Path: main/boards/otto-robot/otto_emoji_display.h
//
// Perubahan dari versi asli:
//   + include "standby_clock.h"
//   + member private: standby_clock_
//   + method private: ShowStandbyClock() / HideStandbyClock()

#include "display/lcd_display.h"
#include "display/lvgl_display/lvgl_image.h"
#include "standby_clock.h"   // ← TAMBAHAN

#include <memory>

class OttoEmojiDisplay : public SpiLcdDisplay {
public:
    OttoEmojiDisplay(esp_lcd_panel_io_handle_t panel_io,
                     esp_lcd_panel_handle_t    panel,
                     int width, int height,
                     int offset_x, int offset_y,
                     bool mirror_x, bool mirror_y, bool swap_xy);

    void SetupUI()          override;
    void SetupPreviewImage();
    void InitializeOttoEmojis();
    void SetStatus(const char* status) override;
    void SetPreviewImage(std::unique_ptr<LvglImage> image) override;

private:
    // ── TAMBAHAN: jam standby ──────────────────────────────────────────────
    std::unique_ptr<StandbyClock> standby_clock_;

    // Tampilkan jam (sembunyikan emoji_box_, stop GIF)
    void ShowStandbyClock();

    // Hapus jam (kembalikan emoji_box_, restart GIF)
    void HideStandbyClock();
    // ──────────────────────────────────────────────────────────────────────
};
