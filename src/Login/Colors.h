#pragma once
#include "imgui.h"

// ============================================================================
// LOGIN PAGE PALETTE
// ============================================================================
static inline ImU32 LC_Accent()       { return IM_COL32(180,  30,  30, 255); }
static inline ImU32 LC_AccentDark()   { return IM_COL32(120,  20,  20, 255); }
static inline ImU32 LC_AccentBright() { return IM_COL32(220,  40,  40, 255); }
static inline ImU32 LC_AccentSoft()   { return IM_COL32(180,  30,  30,  30); }
static inline ImU32 LC_AccentGlow()   { return IM_COL32(180,  30,  30,  90); }
static inline ImU32 LC_AccentLine()   { return IM_COL32(180,  30,  30, 178); }

static inline ImU32 LC_LeftFrom()     { return IM_COL32(26,   6,  13, 255); }
static inline ImU32 LC_LeftTo()       { return IM_COL32(10,  10,  12, 255); }
static inline ImU32 LC_LeftGlow()     { return IM_COL32(180,  30,  30,  18); }

static inline ImU32 LC_Bg()           { return IM_COL32(5,    5,   6, 255); }
static inline ImU32 LC_Panel()        { return IM_COL32(10,  10,  12, 255); }
static inline ImU32 LC_Panel2()       { return IM_COL32(13,  13,  16, 255); }
static inline ImU32 LC_Panel2Hov()    { return IM_COL32(18,  18,  22, 255); }
static inline ImU32 LC_Panel2Active() { return IM_COL32(22,  22,  26, 255); }

static inline ImU32 LC_Line()         { return IM_COL32(28,  28,  32, 255); }
static inline ImU32 LC_LineSoft()     { return IM_COL32(38,  38,  44, 255); }
static inline ImU32 LC_LineAccent()   { return IM_COL32(180,  30,  30, 100); }

static inline ImU32 LC_Text()         { return IM_COL32(255, 255, 255, 255); }
static inline ImU32 LC_TextSec()      { return IM_COL32(160, 160, 170, 255); }
static inline ImU32 LC_TextDim()      { return IM_COL32(106, 106, 114, 255); }

static inline ImU32 LC_Success()      { return IM_COL32( 52, 199,  89, 255); }
static inline ImU32 LC_SuccessSoft()  { return IM_COL32( 52, 199,  89,  20); }
static inline ImU32 LC_SuccessLine()  { return IM_COL32( 52, 199,  89,  76); }
static inline ImU32 LC_Warning()      { return IM_COL32(245, 158,  11, 255); }
static inline ImU32 LC_Danger()       { return IM_COL32(239,  68,  68, 255); }
static inline ImU32 LC_DangerSoft()   { return IM_COL32(239,  68,  68,  30); }

static inline ImU32 LC_Shadow()       { return IM_COL32(  0,   0,   0, 140); }
