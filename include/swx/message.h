#ifndef _MESSAGE_H
#define _MESSAGE_H

#define REG_VERSION (0) // bytes 0 & 1
#define REG_VERSION_L (REG_VERSION)
#define REG_VERSION_H (REG_VERSION + 1)

#define REG_CHANNEL_COUNT (2)
#define REG_CH_CAL_ENABLED (3)

#define REG_CHnn_CAL_VALUE (8) // two bytes per
#define REG_CH1_CAL_VALUE (REG_CHnn_CAL_VALUE + 0)
#define REG_CH2_CAL_VALUE (REG_CHnn_CAL_VALUE + 2)
#define REG_CH3_CAL_VALUE (REG_CHnn_CAL_VALUE + 4)
#define REG_CH4_CAL_VALUE (REG_CHnn_CAL_VALUE + 6)

#define REG_CHn_STATUS (15)
#define REG_CH1_STATUS (REG_CHn_STATUS + 0)
#define REG_CH2_STATUS (REG_CHn_STATUS + 1)
#define REG_CH3_STATUS (REG_CHn_STATUS + 2)
#define REG_CH4_STATUS (REG_CHn_STATUS + 3)

// -----------------------------------------------------

#define REG_PSU_STATE (32)

#define REG_CHn_GEN_ENABLE (33)
#define REG_CH1_GEN_ENABLE (REG_CHn_GEN_ENABLE + 0)
#define REG_CH2_GEN_ENABLE (REG_CHn_GEN_ENABLE + 1)
#define REG_CH3_GEN_ENABLE (REG_CHn_GEN_ENABLE + 2)
#define REG_CH4_GEN_ENABLE (REG_CHn_GEN_ENABLE + 3)

#endif // _MESSAGE_H