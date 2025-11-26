/**
 * @file Steno.h
 * @brief PloverHID stenotype feature
 */
 
#ifndef STENO_H
#define STENO_H

#include <stdint.h>
#include "HIDTypes.h"
#include "../../drivers/Software/Log/Log.h"
#include "../../drivers/Software/Event/Types.h"
#include "../../drivers/Software/Transport/Transport.h"

#define STENO_ID 0x50

// Plover HID protocol uses a 4-byte report
typedef struct {
    uint8_t reportId;
    uint8_t keys[8];  // 64 bits for 64 keys
} StenoReport;

static const uint8_t _stenoReportDescriptor[] = {
    USAGE_PAGE(2),      0x50, 0xFF,              
    USAGE(2),           0x56, 0x4C,              
    COLLECTION(1),      0x02,                    
    REPORT_ID(1),       STENO_ID,          
    LOGICAL_MAXIMUM(1), 0x01,                    
    REPORT_SIZE(1),     0x01,                    
    REPORT_COUNT(1),    0x40,                    
    USAGE_PAGE(1),      0x0A,                    
    USAGE_MINIMUM(1),   0x00,                    
    USAGE_MAXIMUM(1),   0x3F,                   
    HIDINPUT(1),        0x02,                    
    END_COLLECTION(0)                            
};

// Plover HID key positions (bit positions in the 8-byte report)
enum class StenoKeys : uint8_t {
    // Standard Ward Stone Ireland keys (22 keys)
    STN_S1  = 0,    // S1-
    STN_TL  = 1,    // T-
    STN_PL  = 2,    // P-
    STN_HL  = 3,    // H-
    STN_ST1 = 4,    // *1 (left star)
    STN_FR  = 5,    // -F
    STN_PR  = 6,    // -P
    STN_LR  = 7,    // -L
    STN_TR  = 8,    // -T
    STN_DR  = 9,    // -D
    STN_S2  = 10,   // S2- (extra)
    STN_KL  = 11,   // K-
    STN_WL  = 12,   // W-
    STN_RL  = 13,   // R-
    STN_ST2 = 14,   // *2 (extra)
    STN_RR  = 15,   // -R
    STN_BR  = 16,   // -B
    STN_GR  = 17,   // -G
    STN_SR  = 18,   // -S
    STN_ZR  = 19,   // -Z
    STN_A   = 20,   // A-
    STN_O   = 21,   // O-
    STN_ST3 = 22,   // *3 (extra)
    STN_E   = 23,   // -E
    STN_U   = 24,   // -U
    STN_ST4 = 25,   // *4 (extra)
    STN_PWR = 26,   // #1
    STN_NUM2 = 27,  // #2 (extra)
    STN_NUM3 = 28,  // #3 (extra)
    STN_NUM4 = 29,  // #4 (extra)
    STN_NUM5 = 30,  // #5 (extra)
    STN_NUM6 = 31,  // #6 (extra)
    
    // Extra steno keys and X keys (PloverHID added 32 more keys seemingly just to spite me)
    STN_NUM7 = 32,  // #7 (extra)
    STN_NUM8 = 33,  // #8 (extra)
    STN_NUM9 = 34,  // #9 (extra)
    STN_NUMA = 35,  // #A (extra)
    STN_NUMB = 36,  // #B (extra)
    STN_NUMC = 37,  // #C (extra)
    
    // X keys X1-X26
    STN_X1 = 38,
    STN_X2 = 39,
    STN_X3 = 40,
    STN_X4 = 41,
    STN_X5 = 42,
    STN_X6 = 43,
    STN_X7 = 44,
    STN_X8 = 45,
    STN_X9 = 46,
    STN_X10 = 47,
    STN_X11 = 48,
    STN_X12 = 49,
    STN_X13 = 50,    
    STN_X14 = 51,
    STN_X15 = 52,
    STN_X16 = 53,
    STN_X17 = 54,
    STN_X18 = 55,
    STN_X19 = 56,
    STN_X20 = 57,
    STN_X21 = 58,
    STN_X22 = 59,
    STN_X23 = 60,
    STN_X24 = 61,
    STN_X25 = 62,
    STN_X26 = 63
};

// Create strong types for Steno keys
MK(StenoKey, SL_S1,    STN_S1);
MK(StenoKey, SL_S2,    STN_S2);
MK(StenoKey, SL_T,     STN_TL);
MK(StenoKey, SL_K,     STN_KL);
MK(StenoKey, SL_P,     STN_PL);
MK(StenoKey, SL_W,     STN_WL);
MK(StenoKey, SL_H,     STN_HL);
MK(StenoKey, SL_R,     STN_RL);
MK(StenoKey, SL_A,     STN_A);
MK(StenoKey, SL_O,     STN_O);
MK(StenoKey, SL_ST1,   STN_ST1);
MK(StenoKey, SL_ST2,   STN_ST2);
MK(StenoKey, SR_ST3,   STN_ST3);
MK(StenoKey, SR_ST4,   STN_ST4);
MK(StenoKey, SR_E,     STN_E);
MK(StenoKey, SR_U,     STN_U);
MK(StenoKey, SR_F,     STN_FR);
MK(StenoKey, SR_R,     STN_RR);
MK(StenoKey, SR_P,     STN_PR);
MK(StenoKey, SR_B,     STN_BR);
MK(StenoKey, SR_L,     STN_LR);
MK(StenoKey, SR_G,     STN_GR);
MK(StenoKey, SR_T,     STN_TR);
MK(StenoKey, SR_S,     STN_SR);
MK(StenoKey, SR_D,     STN_DR);
MK(StenoKey, SR_Z,     STN_ZR);
MK(StenoKey, SL_N2,    STN_NUM2);
MK(StenoKey, SL_N3,    STN_NUM3);
MK(StenoKey, SL_N4,    STN_NUM4);
MK(StenoKey, SL_N5,    STN_NUM5);
MK(StenoKey, SL_N6,    STN_NUM6);
MK(StenoKey, SR_N7,    STN_NUM7);
MK(StenoKey, SR_N8,    STN_NUM8);
MK(StenoKey, SR_N9,    STN_NUM9);
MK(StenoKey, SR_NA,    STN_NUMA);
MK(StenoKey, SR_NB,    STN_NUMB);
MK(StenoKey, SR_NC,    STN_NUMC);
MK(StenoKey, SL_X1,    STN_X1);
MK(StenoKey, SL_X2,    STN_X2);
MK(StenoKey, SL_X3,    STN_X3);
MK(StenoKey, SL_X4,    STN_X4);
MK(StenoKey, SL_X5,    STN_X5);
MK(StenoKey, SL_X6,    STN_X6);
MK(StenoKey, SL_X7,    STN_X7);
MK(StenoKey, SL_X8,    STN_X8);
MK(StenoKey, SL_X9,    STN_X9);
MK(StenoKey, SL_XA,    STN_X10);
MK(StenoKey, SL_XB,    STN_X11);
MK(StenoKey, SL_XC,    STN_X12);
MK(StenoKey, SL_XD,    STN_X13);
MK(StenoKey, SR_X1,    STN_X14);
MK(StenoKey, SR_X2,    STN_X15);
MK(StenoKey, SR_X3,    STN_X16);
MK(StenoKey, SR_X4,    STN_X17);
MK(StenoKey, SR_X5,    STN_X18);
MK(StenoKey, SR_X6,    STN_X19);
MK(StenoKey, SR_X7,    STN_X20);
MK(StenoKey, SR_X8,    STN_X21);
MK(StenoKey, SR_X9,    STN_X22);
MK(StenoKey, SR_XA,    STN_X23);
MK(StenoKey, SR_XB,    STN_X24);
MK(StenoKey, SR_XC,    STN_X25);
MK(StenoKey, SR_XD,    STN_X26);
MK(StenoKey, SL_PWR,   STN_PWR);
MK(StenoKey, SR_PWR,   STN_PWR);

class SQUIDSTENO {
private:
    Transport*  transport;
    StenoReport _stenoReport;
    uint32_t    _delay_ms = 7;
    
    void updateStenoKey(StenoKey stenoKey, bool pressed);
    
public:
    SQUIDSTENO();
    ~SQUIDSTENO();
    
    void   begin(Transport* transport, uint32_t delay_ms = 7);
    void   onConnect();
    void   onDisconnect();
    
    // Steno methods
    size_t press(StenoKey stenoKey);
    size_t release(StenoKey stenoKey);
    void   releaseAll();
    void   stenoStroke(const StenoKey* keys, size_t count);
    void   sendStenoReport();
};

#endif
