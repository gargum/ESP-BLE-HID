/**
 * @file Steno.h
 * @brief PloverHID stenotype feature
 */
 
#ifndef STENO_H
#define STENO_H

#include <stdint.h>
#include "HIDTypes.h"
#include "../../drivers/Log/Log.h"
#include "../../drivers/Event/Types.h"
#include "../../drivers/Transport/Transport.h"

#define STENO_ID 0x07

// Plover HID protocol uses a 4-byte report
typedef struct {
    uint8_t reportId;
    uint8_t keys0;  // Byte 0: S- T- K- P- W- H- R-
    uint8_t keys1;  // Byte 1: A- O- * -E -U -F -R-
    uint8_t keys2;  // Byte 2: -P -B -L -G -T -S -D-
    uint8_t keys3;  // Byte 3: -Z #1 #2 #3 #4 #5 #6
} StenoReport;

static const uint8_t _stenoReportDescriptor[] = {
    // Plover Steno HID Report
    USAGE_PAGE(1),      0xFF, 0x00,              // Vendor-defined usage page
    USAGE(1),           0x01,                    // Vendor usage 1
    COLLECTION(1),      0x01,                    // Application
    REPORT_ID(1),       STENO_ID,
    
    // 4-byte steno key bitmap
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x01,
    REPORT_SIZE(1),     0x01,
    REPORT_COUNT(1),    0x20,                    // 32 bits = 4 bytes
    
    USAGE_MINIMUM(1),   0x00,
    USAGE_MAXIMUM(1),   0x1F,
    HIDINPUT(1),        0x02,                    // Data,Var,Abs
    
    END_COLLECTION(0)
};

// Plover HID key positions (bit positions in the 4-byte report)
enum class StenoKeys : uint8_t {
    // Byte 0
    STN_S1  = 0,    // S-
    STN_S2  = 1,    // S-
    STN_T   = 2,    // T-
    STN_K   = 3,    // K-
    STN_P   = 4,    // P-
    STN_W   = 5,    // W-
    STN_H   = 6,    // H-
    STN_R   = 7,    // R-
    
    // Byte 1
    STN_A   = 8,    // A-
    STN_O   = 9,    // O-
    STN_STAR = 10,  // *
    STN_E   = 11,   // -E
    STN_U   = 12,   // -U
    STN_F   = 13,   // -F
    STN_R2  = 14,   // -R
    
    // Byte 2
    STN_P2  = 15,   // -P
    STN_B   = 16,   // -B
    STN_L   = 17,   // -L
    STN_G   = 18,   // -G
    STN_T2  = 19,   // -T
    STN_S   = 20,   // -S
    STN_D   = 21,   // -D
    
    // Byte 3
    STN_Z   = 22,   // -Z
    STN_NUM1 = 23,  // #1
    STN_NUM2 = 24,  // #2
    STN_NUM3 = 25,  // #3
    STN_NUM4 = 26,  // #4
    STN_NUM5 = 27,  // #5
    STN_NUM6 = 28,  // #6
    STN_FN   = 29   // Function key
};

// Create strong types for Steno keys
MK(StenoKey, SL_S1,    STN_S1);
MK(StenoKey, SL_S2,    STN_S2);
MK(StenoKey, SL_T,     STN_T);
MK(StenoKey, SL_K,     STN_K);
MK(StenoKey, SL_P,     STN_P);
MK(StenoKey, SL_W,     STN_W);
MK(StenoKey, SL_H,     STN_H);
MK(StenoKey, SL_R,     STN_R);
MK(StenoKey, SL_A,     STN_A);
MK(StenoKey, SL_O,     STN_O);
MK(StenoKey, SL_ST1,   STN_STAR);
MK(StenoKey, SL_ST2,   STN_STAR);
MK(StenoKey, SR_ST3,   STN_STAR);
MK(StenoKey, SR_ST4,   STN_STAR);
MK(StenoKey, SR_E,     STN_E);
MK(StenoKey, SR_U,     STN_U);
MK(StenoKey, SR_F,     STN_F);
MK(StenoKey, SR_R,     STN_R2);
MK(StenoKey, SR_P,     STN_P2);
MK(StenoKey, SR_B,     STN_B);
MK(StenoKey, SR_L,     STN_L);
MK(StenoKey, SR_G,     STN_G);
MK(StenoKey, SR_T,     STN_T2);
MK(StenoKey, SR_S,     STN_S);
MK(StenoKey, SR_D,     STN_D);
MK(StenoKey, SR_Z,     STN_Z);
MK(StenoKey, SL_1,     STN_NUM1);
MK(StenoKey, SL_2,     STN_NUM2);
MK(StenoKey, SL_3,     STN_NUM3);
MK(StenoKey, SR_4,     STN_NUM4);
MK(StenoKey, SR_5,     STN_NUM5);
MK(StenoKey, SR_6,     STN_NUM6);
MK(StenoKey, SL_FN,    STN_FN);
MK(StenoKey, SR_FN,    STN_FN);

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
