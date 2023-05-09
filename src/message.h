#ifndef _MESSAGE_H
#define _MESSAGE_H

/*
Messages are variable length with a minimum of one byte. Some messages use the MP (MSG_MP_MASK) to represent
a channel index, target, or boolean state allowing for a single byte message.
Most other messages have 2 additional bytes representing a uint16_t, with some messages having an additional 3 bytes.

--------------------------------------------------------------------------------
         0 (CTRL)        |        +1       |       +2        |        +3       |
--------------------------------------------------------------------------------
ID | R/W |   CMD |    MP |            ARG1 |            ARG2 |            ARG3 |
 1 |   0 |  0000 |    00 |        00000000 |        00000000 |        00000000 |
--------------------------------------------------------------------------------

ID = CTRL Magic Bit (used by STDIO comms to differentiate between log messages and commands)
R/W = Read/Write
RO = Read Only
WO = Write Only

Note:
    Ignoring GPIO pin limitations, swx has the capability to support up to 7 output channels (or 8 with reduced features).
    However, the current communication protocol can only support up to 4 channels (2-bit MSG_MP).
    I think the trade off in performance and simplicity wins over the edge case of supporting devices with more than 4 channels.
*/

#define MSG_ID (7)
#define MSG_ID_MASK (1 << MSG_ID)

// CMD Read/Write
#define MSG_MODE (6)
#define MSG_MODE_MASK (1 << MSG_MODE)
#define MSG_MODE_READ (0)
#define MSG_MODE_WRITE (1)

// Multi purpose (cmd dependant) - Channel index, command state, etc
#define MSG_MP (0)
#define MSG_MP_MASK (3 << MSG_MP)
#define MSG_MP_STATE_FALSE (0) // CMD State = false
#define MSG_MP_STATE_TRUE (1)  // CMD State = true
#define MSG_MP_CHANNEL_1 (0)   // Channel 1
#define MSG_MP_CHANNEL_2 (1)   // Channel 2
#define MSG_MP_CHANNEL_3 (2)   // Channel 3
#define MSG_MP_CHANNEL_4 (3)   // Channel 4

// Command
#define MSG_CMD (2)
#define MSG_CMD_MASK (15 << MSG_CMD)

#define MSG_CMD_STATUS (0)  // SWX Status - Version and channel count (RO)
#define MSG_CMD_PSU (1)     // Power Supply State (R/W)

#define MSG_CMD_AI_READ (2) // Read Audio Input (RO)

// All MSG_CMD_CH_ commands use MSG_MP as the channel index
#define MSG_CMD_CH_STATUS (8)   // Channel Status - See channel_status_t (RO)
#define MSG_CMD_CH_EN (9)       // Channel Enabled State (R/W) - Supports switch off delay
#define MSG_CMD_CH_POWER (10)    // Channel Power Level Percent (R/W)
#define MSG_CMD_CH_PARAM (11)    // Dynamic Channel Parameter (R/W)
#define MSG_CMD_CH_AI_SRC (12)   // Channel Audio Source (R/W)

#define MSG_CMD_CH_LL_PULSE (13) // Immediately pulse channel, bypassing pulse generation (R/W)
#define MSG_CMD_CH_LL_POWER (14) // Immediately set channel power, bypassing pulse generation (WO)

#define MSG_CMD_CH_PARAM_FLAGS (15) // Channel Parameter State Flags (R/W)

// ----------------------------------------------------------------------------------------------------

#define MSG_CTRL(mode, cmd, mp) ((mode) << MSG_MODE) | ((cmd) << MSG_CMD) | ((mp) << MSG_MP)
#define HL16(number) ((number) >> 8), ((number)&0xff)

#endif // _MESSAGES_H