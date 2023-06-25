#ifndef _MESSAGE_H
#define _MESSAGE_H

#define REG_VERSION (0)                 // bytes 0 & 1 (readonly)
#define REG_VERSION_L (REG_VERSION)     // (readonly)
#define REG_VERSION_H (REG_VERSION + 1) // (readonly)

#define REG_CHANNEL_COUNT (2)           // Number of channels (readonly)
#define REG_CH_CAL_ENABLED (3)          // (readonly)

#define REG_CHnn_CAL_VALUE (8)          // uint16_t channel cal value (readonly)
#define REG_CH1_CAL_VALUE (REG_CHnn_CAL_VALUE + 0)
#define REG_CH2_CAL_VALUE (REG_CHnn_CAL_VALUE + 2)
#define REG_CH3_CAL_VALUE (REG_CHnn_CAL_VALUE + 4)
#define REG_CH4_CAL_VALUE (REG_CHnn_CAL_VALUE + 6)

#define REG_CHn_STATUS (15) // channel_status_t (readonly)
#define REG_CH1_STATUS (REG_CHn_STATUS + 0)
#define REG_CH2_STATUS (REG_CHn_STATUS + 1)
#define REG_CH3_STATUS (REG_CHn_STATUS + 2)
#define REG_CH4_STATUS (REG_CHn_STATUS + 3)

#define REG_PSU_STATE (19) // PSU enabled state (readonly)

// -----------------------------------------------------

#define REG_PSU_ENABLE (32)    // Enable/disable PSU

#define REG_CH_GEN_ENABLE (33) // Channel pulse_gen enabled
#define REG_CH1_GEN_ENABLE_BIT (0)
#define REG_CH1_GEN_ENABLE_MASK (1 << REG_CH1_GEN_ENABLE_BIT)
#define REG_CH2_GEN_ENABLE_BIT (1)
#define REG_CH2_GEN_ENABLE_MASK (1 << REG_CH2_GEN_ENABLE_BIT)
#define REG_CH3_GEN_ENABLE_BIT (2)
#define REG_CH3_GEN_ENABLE_MASK (1 << REG_CH3_GEN_ENABLE_BIT)
#define REG_CH4_GEN_ENABLE_BIT (3)
#define REG_CH4_GEN_ENABLE_MASK (1 << REG_CH4_GEN_ENABLE_BIT)

#define REG_CHnn_POWER (34) // uint16_t channel power level (scaled with PARAM_POWER)
#define REG_CH1_POWER (REG_CHnn_POWER + 0)
#define REG_CH2_POWER (REG_CHnn_POWER + 2)
#define REG_CH3_POWER (REG_CHnn_POWER + 4)
#define REG_CH4_POWER (REG_CHnn_POWER + 6)

#define REG_CHn_AUDIO_SRC (42) // Channel audio source, override default pulse_gen. see analog_channel_t
#define REG_CH1_AUDIO_SRC (REG_CHn_AUDIO_SRC + 0)
#define REG_CH2_AUDIO_SRC (REG_CHn_AUDIO_SRC + 1)
#define REG_CH3_AUDIO_SRC (REG_CHn_AUDIO_SRC + 2)
#define REG_CH4_AUDIO_SRC (REG_CHn_AUDIO_SRC + 3)

#define REG_CHnn_PARAM (152) // Channel pulse generation parameters. see PARAM_TARGET_INDEX() for required offsets

#endif                       // _MESSAGE_H