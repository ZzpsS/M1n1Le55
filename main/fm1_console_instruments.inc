/* Native instrument structures, included by fm1_console.c.
 * A local page is a reference to the device UI, never a claim of screen sync.
 * Source evidence and protocol boundaries: docs/CONSOLE_STRUCTURE_SOURCES_INSTRUMENTS.md. */
enum {
    I_F_ASSIST = 1000, I_F_TYPE = 1010, I_F_ROOT = 1030, I_F_NOTE = 1050,
    I_F_OCT_DOWN = 1070, I_F_OCT_UP,
    I_X_PART = 1100, I_X_NOTE = 1120, I_X_PREV = 1140, I_X_NEXT,
    I_X_CH_DOWN, I_X_CH_UP,
    I_V_CC = 1200, I_V_CH_DOWN = 1400, I_V_CH_UP
};
static unsigned fomni_assist, x_part, x_subpage, x_track[2];
static unsigned x_channels[5] = {9, 10, 1, 2, 3};
static const char *const x_parts[] = {"909", "808", "303A", "303B", "BREAK"};
static const char *const chord_names[] = {"maj", "min", "7", "maj7", "min7", "dim", "aug", "sus4", "add9"};

static unsigned instruments_page_count(void)
{
    return profile == FW_FOMNI ? 5 : profile == FW_X0X || profile == FW_VA ? 8 : 0;
}

/* These are the real KNOB 1..4 assignments, without invented values or sliders. */
static void native_refs(const char *const names[4])
{
    for (unsigned i = 0; i < 4; ++i) {
        int x = (int)i * 264;
        lv_obj_t *p = m1_panel(panel, m1_wide(x), 398, m1_wide(256), 72, false);
        lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(p, lv_color_hex(M1_SURFACE), 0);
        label(names[i] && names[i][0] ? names[i] : "—", x + 12, 402, 232, &m1_font_20, M1_FG);
        char s[40];
        snprintf(s, sizeof(s), "KNOB %u · 本机调节", i + 1);
        label(names[i] && names[i][0] ? s : "未分配", x + 12, 437, 232, &m1_font_20, M1_MUTED);
    }
}

static void fomni_chord_input(void)
{
    for (unsigned i = 0; i < 9; ++i) {
        lv_obj_t *b = button(chord_names[i], i * 116, 202, 108, 44, I_F_TYPE + i, false);
        m1_selected(b, chord_type == i);
    }
    static const char *const roots[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    for (unsigned i = 0; i < 12; ++i)
        m1_enabled(button(roots[i], (i % 6) * 176, 256 + (i / 6) * 62, 168, 52,
                          I_F_ROOT + i, true), can_send());
    label("CH2 和弦输入 · 短触发 450 ms；本机 HOLD 可能保持声音。", 0, 371, 1048, &m1_font_20, M1_MUTED);
}

static void fomni_strum_input(void)
{
    for (unsigned i = 0; i < 12; ++i) {
        char s[32];
        snprintf(s, sizeof(s), "NOTE %u", note_base + i);
        m1_enabled(button(s, (i % 6) * 176, 208 + (i / 6) * 62, 168, 52,
                          I_F_NOTE + i, false), can_send());
    }
    button("输入八度 −", 0, 334, 248, 46, I_F_OCT_DOWN, false);
    button("输入八度 +", 264, 334, 248, 46, I_F_OCT_UP, false);
    label("CH1 按音高拨最近的弦；不改变本机 Octave。", 536, 337, 512, &m1_font_20, M1_MUTED);
}

static void fomni(void)
{
    static const char *const pages[] = {"Play", "Rhythm", "Sound", "Chords", "Setup"};
    static const char *const knobs[5][4] = {
        {"Voice 1", "Voice 2", "Sustain", "Chord level"},
        {"Rhythm", "Tempo", "Drums level", "Auto bass"},
        {"Reverb", "Space", "Width", "Chord rev"},
        {"Root", "Type", "Transpose", "Octave"},
        {"Tune", "MIDI out", "Lights", ""}
    };
    mode_buttons(pages, 5);
    if (mode == 0) {
        label("HOME · Play / MIDI 演奏辅助", 0, 157, 510, &m1_font_20, M1_BLUE);
        lv_obj_t *b = button("和弦输入", 528, 150, 248, 40, I_F_ASSIST, false);
        m1_selected(b, !fomni_assist);
        b = button("拨弦输入", 792, 150, 256, 40, I_F_ASSIST + 1, false);
        m1_selected(b, fomni_assist != 0);
        if (fomni_assist) fomni_strum_input(); else fomni_chord_input();
    } else if (mode == 1) {
        label("SEQ · Rhythm / 选择节奏、鼓音量和自动低音请用本机旋钮。", 0, 158, 1048, &m1_font_20, M1_BLUE);
        control(C_LOCAL, 0, "Tab5 外部时钟 BPM", 30, 300, bpm, true, NULL, 0, 214, 1048);
        m1_enabled(button(host.clock_enabled ? "停止时钟" : "启动时钟", 0, 328, 336, 54,
                          E_CLOCK, host.clock_enabled), can_send());
        label(host.clock_enabled ? "Tab5 正在发送时钟；本机应显示 EXT。" : "需外部同步时，先启动时钟，再开始节奏。",
              368, 337, 680, &m1_font_20, M1_MUTED);
    } else if (mode == 2) {
        label("FX · Sound / 本机效果路径", 0, 160, 1048, &m1_font_24, M1_BLUE);
        label("STRINGS  →  Reverb / Space / Width", 0, 224, 1048, &m1_font_24, M1_FG);
        label("CHORD + BASS  →  Chord rev", 0, 282, 1048, &m1_font_24, M1_FG);
        label("DRUMS  →  干声；这些效果设置请在 FM-1 调整。", 0, 340, 1048, &m1_font_20, M1_MUTED);
    } else if (mode == 3) {
        label("SEL · Chords / 本机黑键选和弦垫；下方是独立 MIDI 输入。", 0, 157, 1048, &m1_font_20, M1_BLUE);
        fomni_chord_input();
    } else {
        label("GLO · Setup / 本机设置", 0, 160, 1048, &m1_font_24, M1_BLUE);
        label("Tune  调音     MIDI out  输出开关     Lights  灯光", 0, 224, 1048, &m1_font_24, M1_FG);
        label("REC = Sync start    ARP = Chord hold    SAVE = 保存", 0, 282, 1048, &m1_font_20, M1_FG);
        label("这些按键和旋钮需在 FM-1 操作；此处不显示未读回的设置。", 0, 340, 1048, &m1_font_20, M1_MUTED);
    }
    native_refs(knobs[mode < 5 ? mode : 0]);
    transport_footer(false);
}

static unsigned x_pages(void)
{
    if (mode == 0) return x_part < 2 ? (x_part == 0 && x_track[0] == 0 ? 3 : 2) : 2;
    if (mode == 1) return x_part < 2 ? 1 : 2;
    if (mode == 3 || mode == 4 || mode == 7) return 2;
    return mode == 5 ? 4 : 1;
}

static void x_refs(void)
{
    static const char *const perform[] = {"Tempo", "Swing", "DJ filter", "Pump"};
    static const char *const tone[] = {"Cutoff", "Resonance", "Env mod", "Decay"};
    static const char *const voice[] = {"Accent", "Wave", "Drive", "Slide"};
    static const char *const loops[] = {"Loop A", "Loop B", "B chance", "Pitch"};
    static const char *const brsound[] = {"Filter", "Level", "", ""};
    static const char *const drum_seq[] = {"Length", "Rate", "", ""};
    static const char *const bass_seq[] = {"Length", "Rate", "Direction", "Transpose"};
    static const char *const keys[] = {"Free play", "", "", ""};
    static const char *const groove[] = {"Complexity", "Anchor", "Roll", "Fill"};
    static const char *const retrig[] = {"Retrig 2", "Retrig 3", "Retrig 4", "Retrig 8"};
    static const char *const generate[] = {"Density", "Accent", "Slide", "Octaves"};
    static const char *const scale[] = {"Root", "Scale", "Base", "Mutation"};
    static const char *const reverb[] = {"Decay", "Tone", "", ""};
    static const char *const delay[] = {"Time", "Feedback", "Tone", "Tape"};
    static const char *const mixer[] = {"Level", "Pan", "Rev", "Dly"};
    static const char *const master[] = {"Drive", "Compressor", "Pump", "DJ filter"};
    static const char *const song_a[] = {"Bar", "909 pattern", "808 pattern", "303A pattern"};
    static const char *const song_b[] = {"Bar", "303B pattern", "BREAK pattern", "Repeats"};
    static const char *const kit[] = {"Accent", "", "", ""};
    static const char *const bd2[] = {"Pitch", "", "", ""};
    static const char *const drum[2][11][4] = {
        {{"Tune", "Attack", "Decay", "P.Dpth"}, {"Tune", "Tone", "Snappy", "Decay"},
         {"Tune", "Decay", "Attack", ""}, {"Tune", "Decay", "Attack", ""}, {"Tune", "Decay", "Attack", ""},
         {"Tune", "", "", ""}, {"Tune", "Tail", "", ""}, {"Decay", "Tune", "", ""},
         {"Decay", "Tune", "", ""}, {"Tune", "Decay", "", ""}, {"Tune", "Decay", "", ""}},
        {{"Tone", "Decay", "Tune", "Attack"}, {"Tone", "Snappy", "Tune", "Decay"},
         {"Tune", "Decay", "Sound", ""}, {"Tune", "Decay", "Sound", ""}, {"Tune", "Decay", "Sound", ""},
         {"Tune", "Decay", "Sound", ""}, {"Tune", "Decay", "Attack", "Sound"}, {"Tune", "Decay", "", ""},
         {"Decay", "Tune", "", ""}, {"Decay", "Tune", "", ""}, {"Tune", "Decay", "", ""}}
    };
    const char *const *r = perform;
    if (mode == 0) {
        if (x_part < 2) r = x_subpage == x_pages() - 1 ? kit : x_subpage ? bd2 : drum[x_part][x_track[x_part]];
        else r = x_part < 4 ? (x_subpage ? voice : tone) : (x_subpage ? brsound : loops);
    } else if (mode == 1) r = x_part < 2 ? drum_seq : x_part < 4 ? (x_subpage ? keys : bass_seq) : (x_subpage ? retrig : groove);
    else if (mode == 3) r = x_subpage ? scale : generate;
    else if (mode == 4) r = x_subpage ? delay : reverb;
    else if (mode == 5) r = x_subpage == 3 ? master : mixer;
    else if (mode == 7) r = x_subpage ? song_b : song_a;
    native_refs(r);
}

static void x_audition(void)
{
    static const char *const drums[2][11] = {
        {"BD", "SD", "LT", "MT", "HT", "RS", "CP", "CH", "OH", "CR", "RD"},
        {"BD", "SD", "LT", "MT", "HT", "RS", "CP", "CB", "CY", "OH", "CH"}
    };
    unsigned count = x_part < 2 ? 11 : x_part < 4 ? 12 : 8;
    for (unsigned i = 0; i < count; ++i) {
        char text[32];
        if (x_part < 2) snprintf(text, sizeof(text), "%s", drums[x_part][i]);
        else if (x_part < 4) snprintf(text, sizeof(text), "NOTE %u", note_base + i);
        else snprintf(text, sizeof(text), "SLICE %u", i + 1);
        m1_enabled(button(text, (i % 6) * 176, 254 + (i / 6) * 52, 168, 44, I_X_NOTE + i, false), can_send());
    }
}

static void x0x(void)
{
    static const char *const pages[] = {"EDIT", "SEQ", "HOME", "TB-3PO", "FX", "MIX", "GLOBAL", "SONG"};
    static const char *const titles[] = {"声部声音 · EDIT", "声部步进 · SEQ", "Pattern 总览 · HOME", "303 生成器 · ARP", "效果 · FX", "混音 · LFO", "设置 · GLO", "Song · 再按 HOME"};
    mode_buttons(pages, 8);
    for (unsigned i = 0; i < 5; ++i) {
        lv_obj_t *b = button(x_parts[i], i * 212, 151, 200, 44, I_X_PART + i, false);
        m1_selected(b, x_part == i);
    }
    char caption[160];
    snprintf(caption, sizeof(caption), "%s  /  %u-%u  ·  本地结构对照", titles[mode < 8 ? mode : 0], x_subpage + 1, x_pages());
    label(caption, 0, 211, 700, &m1_font_20, M1_BLUE);
    m1_enabled(button("上一页", 728, 205, 152, 44, I_X_PREV, false), x_subpage > 0);
    m1_enabled(button("下一页", 896, 205, 152, 44, I_X_NEXT, false), x_subpage + 1 < x_pages());
    if (mode == 0) x_audition();
    else if (mode == 1) {
        label("白键：16 步；OCT −/+：切换第 1–64 步。", 0, 274, 1048, &m1_font_24, M1_FG);
        label(x_part < 2 ? "黑键选择鼓轨；按住步进再转旋钮写入 P-lock。" : x_part < 4 ? "303：音符 / 重音 / 滑音；FREE PLAY 只演奏、不写入。" : "BREAK：步进选择切片；GROOVE 与 RETRIG 决定节奏。",
              0, 334, 1048, &m1_font_20, M1_MUTED);
    } else if (mode == 2) {
        label("白键：Pattern 1–16 / 17–32\n黑键：五个声部静音", 0, 268, 512, &m1_font_24, M1_FG);
        label("Pattern / Mute 请在本机操作。", 0, 350, 512, &m1_font_20, M1_MUTED);
        control(C_LOCAL, 0, "Tab5 外部时钟 BPM", 30, 300, bpm, true, NULL, 536, 254, 512);
        m1_enabled(button(host.clock_enabled ? "停止时钟" : "启动时钟", 536, 350, 248, 44,
                          E_CLOCK, host.clock_enabled), can_send());
        label(host.clock_enabled ? "正在发送时钟" : "外部时钟已停止", 800, 360, 248, &m1_font_20, M1_MUTED);
    } else if (mode == 3) {
        label("GENERATE → SCALE  /  先在本机选择 303A 或 303B。", 0, 274, 1048, &m1_font_24, M1_FG);
        label("OCT+ 生成新线，OCT− 变奏；生成结果与写入操作需在本机完成。", 0, 334, 1048, &m1_font_20, M1_MUTED);
    } else if (mode == 4) {
        label(x_subpage ? "DELAY  /  Time → Feedback → Tone → Tape" : "REVERB  /  Decay → Tone", 0, 274, 1048, &m1_font_24, M1_FG);
        label("各声部的效果发送量在 MIX；SEL 打开完整参数列表。", 0, 334, 1048, &m1_font_20, M1_MUTED);
    } else if (mode == 5) {
        static const char *const sub[] = {"PARTS", "909 MIX", "808 MIX", "MASTER"};
        label(sub[x_subpage], 0, 274, 1048, &m1_font_28, M1_FG);
        label("PARTS 选择声部；鼓机 MIX 选择鼓轨；MASTER 为总线处理。", 0, 334, 1048, &m1_font_20, M1_MUTED);
    } else if (mode == 6) {
        label("MIDI → AUDIO → KEYS → PROJECT → SYSTEM", 0, 274, 1048, &m1_font_24, M1_FG);
        label("保存、清空、重置在本机完成。试听通道请按本机 MIDI 设置核对。", 0, 334, 1048, &m1_font_20, M1_MUTED);
    } else {
        label("每个 BAR 指定五声部的 Pattern、Mute 和重复次数。", 0, 274, 1048, &m1_font_24, M1_FG);
        label("HOME 从 Pattern 进入 Song；此处未读回 Song，不显示假步进。", 0, 334, 1048, &m1_font_20, M1_MUTED);
    }
    if (mode == 0) {
        snprintf(caption, sizeof(caption), "MIDI 试听 CH %u · 不改变本机选中声部；左右调整试听通道。", x_channels[x_part] + 1);
        label(caption, 56, 362, 936, &m1_font_20, M1_MUTED);
        button("−", 0, 352, 44, 44, I_X_CH_DOWN, false);
        button("+", 1004, 352, 44, 44, I_X_CH_UP, false);
    }
    x_refs();
    transport_footer(false);
}

static void va(void)
{
    static const char *const pages[] = {"HOME", "EDIT", "FX", "ENV", "LFO", "GLO", "ARP", "SEQ"};
    static const char *const captions[] = {
        "HOME：四旋钮 → 示波器 → 音色；再按 HOME 选择旋钮组。",
        "EDIT：当前音色的分组列表；FM / VA / 8-Bit 的内容各不相同。",
        "FX：当前音色的效果列表；打开 FX 不会切换旋钮组。",
        "ENV：当前音色的包络列表；HOME 返回上一层。",
        "LFO：当前音色的 LFO 列表；SELECT 选行，ALGORITHM 改值。",
        "GLO：整机设置；HOME 返回，SAVE 仅能在本机保存。",
        "ARP：Note Engine；本机 ARP 键打开，重复按可关闭。",
        "SEQ：Pattern / Flow；再次按 SEQ 切换 Pattern 设置。"
    };
    mode_buttons(pages, 8);
    static const char *const turns[] = {"SELECT −", "SELECT +", "ALGO −", "ALGO +", "PRESET −", "PRESET +"};
    static const unsigned turn_cc[] = {118, 119, 89, 90, 116, 117};
    for (unsigned i = 0; i < 6; ++i)
        m1_enabled(button(turns[i], i * 176, 153, 168, 48, I_V_CC + turn_cc[i], false), can_send());
    static const char *const keys[] = {"FX", "SEL", "ENV", "LFO", "EDIT", "GLO", "HOME", "SAVE 本机", "ARP", "SEQ", "PLAY", "REC", "OCT −", "OCT +"};
    static const unsigned key_cc[] = {102, 103, 104, 105, 106, 107, 108, 0, 110, 111, 112, 113, 114, 115};
    for (unsigned i = 0; i < 14; ++i)
        m1_enabled(button(keys[i], (i % 7) * 150, 214 + (i / 7) * 58, 144, 48,
                          I_V_CC + key_cc[i], false), key_cc[i] && can_send());
    label(captions[mode < 8 ? mode : 0], 0, 331, 1048, &m1_font_20, M1_BLUE);
    label("下方四旋钮操作本机当前 Knob Bank；显示发送目标，不是读回值。", 0, 361, 1048, &m1_font_20, M1_MUTED);
    for (unsigned i = 0; i < 4; ++i) {
        char name[20];snprintf(name, sizeof(name), "KNOB %u", i + 1);
        control(C_CC, 85 + i, name, 0, 127, cc_values[channel][85 + i], cc_known[channel][85 + i], NULL,
                i * 264, 390, 256);
    }
    button("通道 −", 0, 492, 160, 52, I_V_CH_DOWN, false);
    button("通道 +", 176, 492, 160, 52, I_V_CH_UP, false);
    char ch[40];snprintf(ch, sizeof(ch), "MIDI CH %u", channel + 1);
    label(ch, 360, 505, 208, &m1_font_20, M1_MUTED);
    button("音色文件", 576, 492, 224, 52, E_FILE, false);
    button("深入编辑", 816, 492, 232, 52, E_ADVANCED, false);
}

static void instruments_reset(void)
{
    fomni_assist = x_part = x_subpage = 0;
    x_track[0] = x_track[1] = 0;
    static const unsigned channels[] = {9, 10, 1, 2, 3};
    memcpy(x_channels, channels, sizeof(x_channels));
    chord_type = 0;
}

static void instruments_page_changed(void)
{
    x_subpage = 0;
}

static bool instruments_event(unsigned a)
{
    if (a < 1000 || a >= 1500) return false;
    if (fm1_console_dragging()) return true;
    if (profile == FW_FOMNI) {
        if (a >= I_F_ASSIST && a < I_F_ASSIST + 2) {fomni_assist = a - I_F_ASSIST;render();}
        else if (a >= I_F_TYPE && a < I_F_TYPE + 9) {chord_type = a - I_F_TYPE;render();}
        else if (a >= I_F_ROOT && a < I_F_ROOT + 12) {
            static const uint8_t intervals[9][4] = {{0,4,7,0},{0,3,7,0},{0,4,7,10},{0,4,7,11},{0,3,7,10},{0,3,6,0},{0,4,8,0},{0,5,7,0},{0,4,7,14}};
            unsigned t = chord_type < 9 ? chord_type : 0;
            unsigned count = t == 2 || t == 3 || t == 4 || t == 8 ? 4 : 3;
            uint8_t notes[4];
            for (unsigned i = 0; i < count; ++i) notes[i] = 48 + a - I_F_ROOT + intervals[t][i];
            accepted(can_send() && fm1_adapter_chord(notes, count, 450));
        } else if (a >= I_F_NOTE && a < I_F_NOTE + 12) midi(0x90, note_base + a - I_F_NOTE, velocity, 3, 0);
        else if (a == I_F_OCT_DOWN || a == I_F_OCT_UP) {
            if (a == I_F_OCT_DOWN && note_base >= 12) note_base -= 12;
            if (a == I_F_OCT_UP && note_base <= 104) note_base += 12;
            render();
        }
    } else if (profile == FW_X0X) {
        if (a >= I_X_PART && a < I_X_PART + 5) {x_part = a - I_X_PART;x_subpage = 0;render();}
        else if (a >= I_X_NOTE && a < I_X_NOTE + 12) {
            static const uint8_t notes[2][11] = {{36,38,41,45,50,37,39,42,46,49,51},{36,38,41,45,50,37,39,56,49,46,42}};
            unsigned i = a - I_X_NOTE, note = x_part < 2 && i < 11 ? notes[x_part][i] : x_part < 4 ? note_base + i : 36 + i;
            if (x_part < 2 && i < 11) {x_track[x_part] = i;x_subpage = 0;dirty = true;}
            midi(0x90 | x_channels[x_part], note, velocity, 3, 180);
        } else if (a == I_X_PREV || a == I_X_NEXT) {
            if (a == I_X_PREV && x_subpage) --x_subpage;
            if (a == I_X_NEXT && x_subpage + 1 < x_pages()) ++x_subpage;
            render();
        } else if (a == I_X_CH_DOWN || a == I_X_CH_UP) {
            if (a == I_X_CH_DOWN && x_channels[x_part]) --x_channels[x_part];
            if (a == I_X_CH_UP && x_channels[x_part] < 15) ++x_channels[x_part];
            render();
        }
    } else if (profile == FW_VA) {
        if (a > I_V_CC && a < I_V_CC + 128) {
            if (midi(0xb0 | channel, a - I_V_CC, 127, 3, 100)) {
                for (unsigned i = 85; i <= 88; ++i) cc_known[channel][i] = false;
                dirty = true;
            }
        } else if (a == I_V_CH_DOWN || a == I_V_CH_UP) {
            if (a == I_V_CH_DOWN && channel) --channel;
            if (a == I_V_CH_UP && channel < 15) ++channel;
            render();
        }
    }
    return true;
}
