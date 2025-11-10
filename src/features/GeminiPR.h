//  ----------------------------------------
// | GeminiPR Stenotype Feature - Constants |
//  ----------------------------------------

typedef struct {
  uint8_t byte0;  // Fn  to #6
  uint8_t byte1;  // S1- to H-
  uint8_t byte2;  // R-  to res2
  uint8_t byte3;  // pwr to -R
  uint8_t byte4;  // -P  to -D
  uint8_t byte5;  // #7  to -Z
} GeminiPRReport;

// GeminiPR steno codes - UNIQUE VALUES FOR EACH KEY
enum GeminiPRKeys {
  // Byte 0
  GEMINI_FN    = 0x0100,
  GEMINI_NUM1  = 0x0101,
  
  // Byte 1  
  GEMINI_S1    = 0x0200,
  GEMINI_S2    = 0x0201,
  GEMINI_T     = 0x0202,
  GEMINI_K     = 0x0203,
  GEMINI_P     = 0x0204,
  GEMINI_W     = 0x0205,
  GEMINI_H     = 0x0206,
  
  // Byte 2
  GEMINI_R     = 0x0300,
  GEMINI_A     = 0x0301,
  GEMINI_O     = 0x0302,
  GEMINI_STAR1 = 0x0303,
  GEMINI_STAR2 = 0x0304,
  GEMINI_RES1  = 0x0305,
  GEMINI_RES2  = 0x0306,
  
  // Byte 3
  GEMINI_PWR   = 0x0400,
  GEMINI_STAR3 = 0x0401,
  GEMINI_STAR4 = 0x0402,
  GEMINI_E     = 0x0403,
  GEMINI_U     = 0x0404,
  GEMINI_F     = 0x0405,
  GEMINI_R2    = 0x0406,
  
  // Byte 4
  GEMINI_P2    = 0x0500,
  GEMINI_B     = 0x0501,
  GEMINI_L     = 0x0502,
  GEMINI_G     = 0x0503,
  GEMINI_T2    = 0x0504,
  GEMINI_S     = 0x0505,
  GEMINI_D     = 0x0506,
  
  // Byte 5
  GEMINI_NUM7  = 0x0600,
  GEMINI_NUM8  = 0x0601,
  GEMINI_NUM9  = 0x0602,
  GEMINI_NUM10 = 0x0603,
  GEMINI_NUM11 = 0x0604,
  GEMINI_NUM12 = 0x0605,
  GEMINI_Z     = 0x0606
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
