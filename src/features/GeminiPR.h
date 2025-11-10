//  ----------------------------------------
// | GeminiPR Stenotype Feature - Constants |
//  ----------------------------------------

#include "HIDTypes.h"

#define GEMINIPR_ID   0x06

typedef struct {
  uint8_t byte0;  // Fn  to #6
  uint8_t byte1;  // S1- to H-
  uint8_t byte2;  // R-  to res2
  uint8_t byte3;  // pwr to -R
  uint8_t byte4;  // -P  to -D
  uint8_t byte5;  // #7  to -Z
} GeminiPRReport;

static const uint8_t _geminiPRReportDescriptor[] = {
  // ------------------------------------------------- GeminiPR Steno Protocol
  USAGE_PAGE(1),      0xFF, 0x00,                USAGE(1),           0x01,             
  COLLECTION(1),      0x01,                      REPORT_ID(1),       GEMINIPR_ID,      
  // 6-byte GeminiPR packet (48 bits)
  LOGICAL_MINIMUM(1), 0x00,                      LOGICAL_MAXIMUM(1), 0x01,             
  REPORT_SIZE(1),     0x01,                      REPORT_COUNT(1),    0x30,             
  USAGE(1),           0x01,                      HIDINPUT(1),        0x02,             
  END_COLLECTION(0),   
};

// GeminiPR steno codes

enum GeminiPRKeys {
  // Byte 0
  GEMINI_FN    = 0x80,
  GEMINI_NUM1  = 0x40,
//GEMINI_NUM2  = 0x20, <-
//GEMINI_NUM3  = 0x10, <-   None of these 5 actually exist for some reason.
//GEMINI_NUM4  = 0x08, <-   I've added them as comments to mourn what could have been.
//GEMINI_NUM5  = 0x04, <-
//GEMINI_NUM6  = 0x02, <-

  // Byte 1
  GEMINI_S1    = 0x80,
  GEMINI_S2    = 0x40,
  GEMINI_T     = 0x20,
  GEMINI_K     = 0x10,
  GEMINI_P     = 0x08,
  GEMINI_W     = 0x04,
  GEMINI_H     = 0x02,
  
  // Byte 2
  GEMINI_R     = 0x80,
  GEMINI_A     = 0x40,
  GEMINI_O     = 0x20,
  GEMINI_STAR1 = 0x10,
  GEMINI_STAR2 = 0x08,
  GEMINI_RES1  = 0x04,
  GEMINI_RES2  = 0x02,
  
  // Byte 3
  GEMINI_PWR   = 0x80,
  GEMINI_STAR3 = 0x40,
  GEMINI_STAR4 = 0x20,
  GEMINI_E     = 0x10,
  GEMINI_U     = 0x08,
  GEMINI_F     = 0x04,
  GEMINI_R2    = 0x02,
  
  // Byte 4
  GEMINI_P2    = 0x80,
  GEMINI_B     = 0x40,
  GEMINI_L     = 0x20,
  GEMINI_G     = 0x10,
  GEMINI_T2    = 0x08,
  GEMINI_S     = 0x04,
  GEMINI_D     = 0x02,
  
  // Byte 5
  GEMINI_NUM7  = 0x80,
  GEMINI_NUM8  = 0x40,
  GEMINI_NUM9  = 0x20,
  GEMINI_NUM10 = 0x10,
  GEMINI_NUM11 = 0x08,
  GEMINI_NUM12 = 0x04,
  GEMINI_Z     = 0x02
};

using StenoKey = int32_t;

const StenoKey ST_FN    = GEMINI_FN;
const StenoKey ST_1     = GEMINI_NUM1;
const StenoKey ST_S1    = GEMINI_S1;
const StenoKey ST_S2    = GEMINI_S2;
const StenoKey ST_T     = GEMINI_T;
const StenoKey ST_K     = GEMINI_K;
const StenoKey ST_P     = GEMINI_P;
const StenoKey ST_W     = GEMINI_W;
const StenoKey ST_H     = GEMINI_H;
const StenoKey ST_R     = GEMINI_R;
const StenoKey ST_A     = GEMINI_A;
const StenoKey ST_O     = GEMINI_O;
const StenoKey ST_ST1   = GEMINI_STAR1;
const StenoKey ST_ST2   = GEMINI_STAR2;
const StenoKey ST_RS1   = GEMINI_RES1;
const StenoKey ST_RS2   = GEMINI_RES2;
const StenoKey ST_PWR   = GEMINI_PWR;
const StenoKey ST_ST3   = GEMINI_STAR3;
const StenoKey ST_ST4   = GEMINI_STAR4;
const StenoKey ST_E     = GEMINI_E;
const StenoKey ST_U     = GEMINI_U;
const StenoKey ST_F     = GEMINI_F;
const StenoKey ST_R2    = GEMINI_R2;
const StenoKey ST_P2    = GEMINI_P2;
const StenoKey ST_B     = GEMINI_B;
const StenoKey ST_L     = GEMINI_L;
const StenoKey ST_G     = GEMINI_G;
const StenoKey ST_T2    = GEMINI_T2;
const StenoKey ST_S     = GEMINI_S;
const StenoKey ST_D     = GEMINI_D;
const StenoKey ST_7     = GEMINI_NUM7;
const StenoKey ST_8     = GEMINI_NUM8;
const StenoKey ST_9     = GEMINI_NUM9;
const StenoKey ST_10    = GEMINI_NUM10;
const StenoKey ST_11    = GEMINI_NUM11;
const StenoKey ST_12    = GEMINI_NUM12;
const StenoKey ST_Z     = GEMINI_Z;
