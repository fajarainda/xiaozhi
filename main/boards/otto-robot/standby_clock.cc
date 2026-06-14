// standby_clock.cc  (VERSI RADIKAL v5 — BUMN Neural OS)
// Layout asimetris: jam besar kiri, tanggal kanan, grid bg, sweep line, accent bars
// Warna: Hijau BUMN #00DC64 + Biru #00A8FF

#include "standby_clock.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <cstring>
#include <cstdio>

#define TAG "StandbyClock"

// ─── Palet warna ─────────────────────────────────────────────────────────────
#define CLK_BG          lv_color_hex(0x020202)

#define CLK_GREEN       lv_color_hex(0xE0E0E0)   // silver
#define CLK_BLUE        lv_color_hex(0x00E5FF)   // cyan neon

#define CLK_GREEN_DIM   lv_color_hex(0x7A7A7A)
#define CLK_BLUE_DIM    lv_color_hex(0x3D7A8F)

#define CLK_STRIP_BG    lv_color_hex(0x101010)
#define CLK_BAR_BG      lv_color_hex(0x1A1A1A)

#define CLK_WHITE       lv_color_hex(0xFFFFFF)

// ─── Font ────────────────────────────────────────────────────────────────────
LV_FONT_DECLARE(orbitron_48);
LV_FONT_DECLARE(mono_16);
#define CLOCK_FONT_BIG   (&orbitron_48)
#define CLOCK_FONT_SMALL (&mono_16)

// ─── Helper: rect solid ──────────────────────────────────────────────────────
static lv_obj_t* make_rect(lv_obj_t* parent, int x, int y, int w, int h,
                             lv_color_t color, lv_opa_t opa, int radius = 0)
{
    lv_obj_t* o = lv_obj_create(parent);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, color, 0);
    lv_obj_set_style_bg_opa(o, opa, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

// ─── Helper: label absolut ───────────────────────────────────────────────────
static lv_obj_t* make_label(lv_obj_t* parent, int x, int y,
                              const lv_font_t* font, lv_color_t color,
                              int spc, const char* txt)
{
    lv_obj_t* l = lv_label_create(parent);
    lv_obj_set_pos(l, x, y);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    lv_obj_set_style_text_letter_space(l, spc, 0);
    lv_label_set_text(l, txt);
    return l;
}

// ─────────────────────────────────────────────────────────────────────────────

StandbyClock::StandbyClock(lv_obj_t* parent_obj, int width, int height)
    : width_(width), height_(height)
{
    const int W = width_;    // 240
    const int H = height_;   // 240

    // ── Container ─────────────────────────────────────────────────────────────
    container_ = lv_obj_create(parent_obj);
    lv_obj_set_pos(container_, 0, 0);
    lv_obj_set_size(container_, W, H);
    lv_obj_set_style_bg_color(container_, CLK_BG, 0);
    lv_obj_set_style_bg_opa(container_,   LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_,  0, 0);
    lv_obj_set_style_radius(container_,   0, 0);
    lv_obj_clear_flag(container_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_to_index(container_, -1);

    // ── Grid background — titik-titik 20x20px ────────────────────────────────
    // Simulasikan dengan garis-garis tipis horizontal dan vertikal
    for (int y = 20; y < H; y += 20) {
        lv_obj_t* l = make_rect(container_, 3, y, W-6, 1, CLK_GREEN, LV_OPA_10);
    }
    for (int x = 20; x < W-3; x += 20) {
        lv_obj_t* l = make_rect(container_, x, 0, 1, H, CLK_GREEN, LV_OPA_10);
    }

    // ── Accent bar kiri (hijau) ───────────────────────────────────────────────
    make_rect(container_, 0, 0, 3, H, CLK_GREEN, LV_OPA_60);

    // ── Accent bar kanan (biru) ───────────────────────────────────────────────
    make_rect(container_, W-3, 0, 3, H, CLK_BLUE, LV_OPA_60);

    // ── Status strip atas (y=0..28) ───────────────────────────────────────────
    make_rect(container_, 3, 0, W-6, 28, CLK_STRIP_BG, LV_OPA_COVER);
    // border bawah strip
    make_rect(container_, 3, 27, W-6, 1, CLK_GREEN, LV_OPA_20);

    // Dot indikator ONLINE
    make_rect(container_, 12, 11, 6, 6, CLK_GREEN, LV_OPA_COVER, 3);

    // Label ONLINE
    make_label(container_, 22, 8, CLOCK_FONT_SMALL, CLK_GREEN, 2, "ONLINE");

    // Label SYS-ID kanan
    make_label(container_, W-96, 8, CLOCK_FONT_SMALL, CLK_GREEN_DIM, 1, "ALEXA-001");

    // ── Nama HARI (y=34) ──────────────────────────────────────────────────────
    day_label_ = lv_label_create(container_);
    lv_obj_set_pos(day_label_, 12, 34);
    lv_obj_set_style_text_font(day_label_, CLOCK_FONT_SMALL, 0);
    lv_obj_set_style_text_color(day_label_, CLK_BLUE, 0);
    lv_obj_set_style_text_letter_space(day_label_, 5, 0);
    lv_label_set_text(day_label_, "-------");

    // ── JAM BESAR: HH (hijau) : (biru) MM (putih) ────────────────────────────
    // Font orbitron_48 ~54px tinggi → y=44..98
    // HH
    hour_label_ = lv_label_create(container_);
    lv_obj_set_pos(hour_label_, 8, 52);
    lv_obj_set_style_text_font(hour_label_, CLOCK_FONT_BIG, 0);
    lv_obj_set_style_text_color(hour_label_, CLK_GREEN, 0);
    lv_obj_set_style_text_letter_space(hour_label_, -1, 0);
    lv_label_set_text(hour_label_, "00");

    // Colon — posisi tengah, font kecil agar pas
    colon_label_ = lv_label_create(container_);
    lv_obj_set_pos(colon_label_, 88, 52);   // sedikit turun agar center visual
    lv_obj_set_style_text_font(colon_label_, CLOCK_FONT_BIG, 0);
    lv_obj_set_style_text_color(colon_label_, CLK_BLUE, 0);
    lv_label_set_text(colon_label_, ":");

    // MM
    min_label_ = lv_label_create(container_);
    lv_obj_set_pos(min_label_, 120, 52);
    lv_obj_set_style_text_font(min_label_, CLOCK_FONT_BIG, 0);
    lv_obj_set_style_text_color(min_label_, CLK_WHITE, 0);
    lv_obj_set_style_text_letter_space(min_label_, -1, 0);
    lv_label_set_text(min_label_, "00");

    // ── Tanggal — pojok kanan bawah area jam ─────────────────────────────────
    // Tanggal angka besar (y=100, font besar)
    date_num_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(date_num_label_, CLOCK_FONT_BIG, 0);
    lv_obj_set_style_text_color(date_num_label_, CLK_BLUE, 0);
    lv_obj_set_style_text_letter_space(date_num_label_, 0, 0);
    lv_label_set_text(date_num_label_, "27");
    lv_obj_align(date_num_label_, LV_ALIGN_TOP_RIGHT, -12, 100);

    // Bulan / Tahun kecil di bawah angka tanggal
    date_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(date_label_, CLOCK_FONT_SMALL, 0);
    lv_obj_set_style_text_color(date_label_, CLK_GREEN_DIM, 0);
    lv_obj_set_style_text_letter_space(date_label_, 1, 0);
    lv_label_set_text(date_label_, "MEI / 2026");
    lv_obj_align(date_label_, LV_ALIGN_TOP_RIGHT, -12, 142);

    // ── Divider tengah (y=158, 1px) ───────────────────────────────────────────
    make_rect(container_, 3, 158, W-5, 1, CLK_GREEN, LV_OPA_30);

    // ── SEC bar (y=164, h=6) ──────────────────────────────────────────────────
    const int bar_x = 12;
    const int bar_w = W - 12 - 64;   // sisakan ~44px kanan untuk angka
    const int bar_y = 164;

    make_label(container_, bar_x, bar_y - 1, CLOCK_FONT_SMALL, CLK_GREEN_DIM, 0, "SEC");

    // track
    make_rect(container_, bar_x + 30, bar_y + 2, bar_w, 6, CLK_BAR_BG, LV_OPA_COVER);
    // border track
    lv_obj_t* track_border = lv_obj_create(container_);
    lv_obj_set_pos(track_border, bar_x + 30, bar_y + 2);
    lv_obj_set_size(track_border, bar_w, 6);
    lv_obj_set_style_bg_opa(track_border, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(track_border, CLK_GREEN, 0);
    lv_obj_set_style_border_opa(track_border, LV_OPA_20, 0);
    lv_obj_set_style_border_width(track_border, 1, 0);
    lv_obj_set_style_radius(track_border, 0, 0);
    lv_obj_clear_flag(track_border, LV_OBJ_FLAG_SCROLLABLE);

    // bar isi
    sec_bar_ = lv_bar_create(container_);
    lv_bar_set_range(sec_bar_, 0, 59);
    lv_bar_set_value(sec_bar_, 0, LV_ANIM_OFF);
    lv_obj_set_pos(sec_bar_, bar_x + 30, bar_y + 2);
    lv_obj_set_size(sec_bar_, bar_w, 6);
    lv_obj_set_style_bg_color(sec_bar_, CLK_BAR_BG, 0);
    lv_obj_set_style_bg_opa(sec_bar_,   LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(sec_bar_, CLK_GREEN, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(sec_bar_,   LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(sec_bar_, 0, 0);
    lv_obj_set_style_radius(sec_bar_, 0, 0);
    lv_obj_set_style_radius(sec_bar_, 0, LV_PART_INDICATOR);

    // angka detik
    sec_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(sec_label_, CLOCK_FONT_SMALL, 0);
    lv_obj_set_style_text_color(sec_label_, CLK_GREEN, 0);
    lv_label_set_text(sec_label_, "00");
    lv_obj_set_pos(sec_label_, bar_x + 30 + bar_w + 6, bar_y - 1);

    // ── Info grid: ENGINEER | UPTIME (y=182) ──────────────────────────────────
    const int grid_y  = 182;
    const int grid_h  = 36;
    const int grid_x  = 12;
    const int cell_w  = (W - 12 - 12 - 6) / 2;   // ~105px tiap cell

    // Cell ENGINEER
    lv_obj_t* cell_eng = lv_obj_create(container_);
    lv_obj_set_pos(cell_eng, grid_x, grid_y);
    lv_obj_set_size(cell_eng, cell_w, grid_h);
    lv_obj_set_style_bg_opa(cell_eng, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(cell_eng, CLK_GREEN, 0);
    lv_obj_set_style_border_opa(cell_eng, LV_OPA_20, 0);
    lv_obj_set_style_border_width(cell_eng, 1, 0);
    lv_obj_set_style_radius(cell_eng, 0, 0);
    lv_obj_set_style_pad_all(cell_eng, 3, 0);
    lv_obj_clear_flag(cell_eng, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* eng_lbl = lv_label_create(cell_eng);
    lv_obj_set_style_text_font(eng_lbl,  CLOCK_FONT_SMALL, 0);
    lv_obj_set_style_text_color(eng_lbl, CLK_GREEN_DIM, 0);
    lv_obj_set_style_text_letter_space(eng_lbl, 1, 0);
    lv_label_set_text(eng_lbl, "OPERATOR");
    lv_obj_align(eng_lbl, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t* eng_val = lv_label_create(cell_eng);
    lv_obj_set_style_text_font(eng_val,  CLOCK_FONT_SMALL, 0);
    lv_obj_set_style_text_color(eng_val, CLK_GREEN, 0);
    lv_obj_set_style_text_letter_space(eng_val, 1, 0);
    lv_label_set_text(eng_val, "Fajar");
    lv_obj_align(eng_val, LV_ALIGN_BOTTOM_LEFT, 0, 3);

    // Cell UPTIME
    lv_obj_t* cell_up = lv_obj_create(container_);
    lv_obj_set_pos(cell_up, grid_x + cell_w + 6, grid_y);
    lv_obj_set_size(cell_up, cell_w, grid_h);
    lv_obj_set_style_bg_opa(cell_up, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(cell_up, CLK_BLUE, 0);
    lv_obj_set_style_border_opa(cell_up, LV_OPA_20, 0);
    lv_obj_set_style_border_width(cell_up, 1, 0);
    lv_obj_set_style_radius(cell_up, 0, 0);
    lv_obj_set_style_pad_all(cell_up, 3, 0);
    lv_obj_clear_flag(cell_up, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* up_lbl = lv_label_create(cell_up);
    lv_obj_set_style_text_font(up_lbl,  CLOCK_FONT_SMALL, 0);
    lv_obj_set_style_text_color(up_lbl, CLK_GREEN_DIM, 0);
    lv_obj_set_style_text_letter_space(up_lbl, 1, 0);
    lv_label_set_text(up_lbl, "UPTIME");
    lv_obj_align(up_lbl, LV_ALIGN_TOP_LEFT, 0, 0);

    uptime_label_ = lv_label_create(cell_up);
    lv_obj_set_style_text_font(uptime_label_,  CLOCK_FONT_SMALL, 0);
    lv_obj_set_style_text_color(uptime_label_, CLK_BLUE, 0);
    lv_obj_set_style_text_letter_space(uptime_label_, 0, 0);
    lv_label_set_text(uptime_label_, "00:00:00");
    lv_obj_align(uptime_label_, LV_ALIGN_BOTTOM_LEFT, 0, 3);

    // ── Footer strip bawah (y=218..240) ──────────────────────────────────────
    make_rect(container_, 3, 218, W-6, 22, CLK_STRIP_BG, LV_OPA_COVER);
    make_rect(container_, 3, 218, W-6, 1,  CLK_BLUE, LV_OPA_20);

    make_label(container_, 0, 224, CLOCK_FONT_SMALL, CLK_BLUE_DIM, 2, "PERUM JASA TIRTA I");
    lv_obj_t* ft = lv_obj_get_child(container_, -1);
    lv_obj_align(ft, LV_ALIGN_TOP_MID, 0, 224);

    // ── Sweep line (animasi via LVGL anim) ───────────────────────────────────
    sweep_line_ = make_rect(container_, 3, 0, W-6, 2, CLK_GREEN, LV_OPA_50);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, sweep_line_);
    lv_anim_set_exec_cb(&a, [](void* obj, int32_t v){
        lv_obj_set_y((lv_obj_t*)obj, (int)v);
    });
    lv_anim_set_values(&a, 0, H);
    lv_anim_set_time(&a, 4000);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_start(&a);

    // ── Tick timer ───────────────────────────────────────────────────────────
    Tick();
    tick_timer_ = lv_timer_create(TickCb, 1000, this);
    ESP_LOGI(TAG, "StandbyClock BUMN v5 created (%dx%d)", W, H);
}

StandbyClock::~StandbyClock() {
    if (tick_timer_) { lv_timer_del(tick_timer_); tick_timer_ = nullptr; }
    if (container_)  {
        lv_anim_del(sweep_line_, nullptr);
        lv_obj_del(container_);
        container_ = time_label_ = date_label_ = day_label_ = nullptr;
        hour_label_ = min_label_ = colon_label_ = nullptr;
        date_num_label_ = sec_bar_ = sec_label_ = uptime_label_ = nullptr;
        sweep_line_ = nullptr;
    }
    ESP_LOGI(TAG, "StandbyClock destroyed");
}

void StandbyClock::TickCb(lv_timer_t* timer) {
    auto* self = static_cast<StandbyClock*>(lv_timer_get_user_data(timer));
    if (self) self->Tick();
}

void StandbyClock::Tick() {
    if (!hour_label_ || !min_label_) return;

    time_t now; struct tm t;
    time(&now); localtime_r(&now, &t);

    static const char* const HARI[]  = {
        "MINGGU","SENIN","SELASA","RABU","KAMIS","JUMAT","SABTU"};
    static const char* const BULAN[] = {
        "JAN","FEB","MAR","APR","MEI","JUN",
        "JUL","AGU","SEP","OKT","NOV","DES"};

    // Jam
    char hh[4], mm[4];
    snprintf(hh, sizeof(hh), "%02d", t.tm_hour);
    snprintf(mm, sizeof(mm), "%02d", t.tm_min);
    lv_label_set_text(hour_label_, hh);
    lv_label_set_text(min_label_,  mm);

    // Colon blink
    if (colon_label_) {
        lv_obj_set_style_text_opa(colon_label_,
            (t.tm_sec % 2 == 0) ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    }

    // Hari
    if (day_label_) lv_label_set_text(day_label_, HARI[t.tm_wday]);

    // Tanggal angka
    if (date_num_label_) {
        char dn[4];
        snprintf(dn, sizeof(dn), "%02d", t.tm_mday);
        lv_label_set_text(date_num_label_, dn);
    }

    // Bulan / tahun
    if (date_label_) {
        char dmy[16];
        snprintf(dmy, sizeof(dmy), "%s / %04d", BULAN[t.tm_mon], t.tm_year + 1900);
        lv_label_set_text(date_label_, dmy);
    }

    // Detik
    if (sec_label_) {
        char sb[4];
        snprintf(sb, sizeof(sb), "%02d", t.tm_sec);
        lv_label_set_text(sec_label_, sb);
    }
    if (sec_bar_) lv_bar_set_value(sec_bar_, t.tm_sec, LV_ANIM_ON);

    // Uptime
    if (uptime_label_) {
        uint64_t us      = esp_timer_get_time();
        uint32_t total_s = (uint32_t)(us / 1000000ULL);
        int uh   = (int)(total_s / 3600);
        int um   = (int)((total_s % 3600) / 60);
        int usec = (int)(total_s % 60);
        char up[16];
        snprintf(up, sizeof(up), "%02d:%02d:%02d", uh, um, usec);
        lv_label_set_text(uptime_label_, up);
    }
}
