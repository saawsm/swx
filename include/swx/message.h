/*
 * swx
 * Copyright (C) 2023 saawsm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _MESSAGE_H
#define _MESSAGE_H

#define READ_ONLY_ADDRESS_BOUNDARY (0x20)
#define MAX_STATE_MEM_SIZE (0x900)

#define REG_VERSION_w (0)

#define REG_CHANNEL_COUNT (2)  // Number of channels (readonly)
#define REG_CH_CAL_ENABLED (3) // (readonly)

#define REG_CHn_STATUS (5) // channel_status_t (readonly)
#define REG_CH1_STATUS (REG_CHn_STATUS + 0)
#define REG_CH2_STATUS (REG_CHn_STATUS + 1)
#define REG_CH3_STATUS (REG_CHn_STATUS + 2)
#define REG_CH4_STATUS (REG_CHn_STATUS + 3)

#define REG_CHn_CAL_VALUE_w (9) // uint16_t channel cal value (readonly)
#define REG_CH1_CAL_VALUE_w (REG_CHn_CAL_VALUE_w + 0)
#define REG_CH2_CAL_VALUE_w (REG_CHn_CAL_VALUE_w + 2)
#define REG_CH3_CAL_VALUE_w (REG_CHn_CAL_VALUE_w + 4)
#define REG_CH4_CAL_VALUE_w (REG_CHn_CAL_VALUE_w + 6)

// ------------------------ READONLY BOUNDARY END ----------------------------- 

#define REG_PSU_ENABLE (32) // Enable/disable PSU

#define REG_CH_GEN_ENABLE (33) // Channel pulse_gen enabled
#define REG_CH1_GEN_ENABLE_BIT (0)
#define REG_CH1_GEN_ENABLE_MASK (1 << REG_CH1_GEN_ENABLE_BIT)
#define REG_CH2_GEN_ENABLE_BIT (1)
#define REG_CH2_GEN_ENABLE_MASK (1 << REG_CH2_GEN_ENABLE_BIT)
#define REG_CH3_GEN_ENABLE_BIT (2)
#define REG_CH3_GEN_ENABLE_MASK (1 << REG_CH3_GEN_ENABLE_BIT)
#define REG_CH4_GEN_ENABLE_BIT (3)
#define REG_CH4_GEN_ENABLE_MASK (1 << REG_CH4_GEN_ENABLE_BIT)

#define REG_CHn_POWER_w (34) // uint16_t channel power level (scaled with PARAM_POWER)
#define REG_CH1_POWER_w (REG_CHn_POWER_w + 0)
#define REG_CH2_POWER_w (REG_CHn_POWER_w + 2)
#define REG_CH3_POWER_w (REG_CHn_POWER_w + 4)
#define REG_CH4_POWER_w (REG_CHn_POWER_w + 6)

// Status flags for when the param value has reached an extent (min/max). Flag bits should be reset when acknowledged.
#define REG_CHn_PARAM_FLAGS_w (50)
#define REG_CH1_PARAM_FLAGS_w (REG_CHn_PARAM_FLAGS_w + 0)
#define REG_CH2_PARAM_FLAGS_w (REG_CHn_PARAM_FLAGS_w + 2)
#define REG_CH3_PARAM_FLAGS_w (REG_CHn_PARAM_FLAGS_w + 4)
#define REG_CH4_PARAM_FLAGS_w (REG_CHn_PARAM_FLAGS_w + 6)

#define REG_CHn_AUDIO_SRC (66) // Channel audio source, override default pulse_gen. see analog_channel_t
#define REG_CH1_AUDIO_SRC (REG_CHn_AUDIO_SRC + 0)
#define REG_CH2_AUDIO_SRC (REG_CHn_AUDIO_SRC + 1)
#define REG_CH3_AUDIO_SRC (REG_CHn_AUDIO_SRC + 2)
#define REG_CH4_AUDIO_SRC (REG_CHn_AUDIO_SRC + 3)

// CMD register, clears on execute. CMD id in upper nibble, channel index in lower nibble.
#define REG_CMD (82)
#define REG_CMD_CH (15)
#define REG_CMD_CH_BIT (0)
#define REG_CMD_CH_MASK (REG_CMD_CH << REG_CMD_CH_BIT)
#define REG_CMD_ACTION (15)
#define REG_CMD_ACTION_BIT (4)
#define REG_CMD_ACTION_MASK (REG_CMD_ACTION << REG_CMD_ACTION_BIT)

#define CMD_PULSE (1)        // arg: pulse_width (uint16_t)
#define CMD_SET_POWER (2)    // arg: power level (uint16_t)
#define CMD_PARAM_UPDATE (3) // arg: param (param_t, uint8_t)
#define CMD_CH1 (0)
#define CMD_CH2 (1)
#define CMD_CH3 (2)
#define CMD_CH4 (3)

// TODO: Audio input Read
// TODO: Fetch cal value (points)

#define MAX_SEQ_COUNT (128)

#define REG_SEQ_PERIOD (90) // milliseconds (uint16_t)
#define REG_SEQ_INDEX (92)
#define REG_SEQ_COUNT (93) // sequence item count (uint8_t)
#define REG_SEQn (94)      // max MAX_SEQ_COUNT

#define MAX_ACTIONS (255) // Max supported action count is 255 since action value (uint16) must be able to fit start/end indices
#define ACTION_SIZE (5)   // size of action entry in bytes

// Action entries are stored sequentially and can be accessed using ACTION_SIZE * index + REG_An_...
#define REG_An_TYPE (224)         // action_type_t (uint8_t)
#define REG_An_CHANNEL_MASK (225) // action channel mask (uint8_t)
#define REG_An_PARAM_TARGET (226) // action parameter+target (uint8_t) upper nibble is parameter, lower nibble is target
#define REG_An_VALUE_w (227)      // action value (uint16_t)

#define MAX_TRIGS (32)
#define TRIG_SIZE (4) // size of trig entry in bytes

// Trig entries are stored sequentially and can be accessed using TRIG_SIZE * index + REG_TRIGn_...
#define REG_TRIGn_INPUT (1505)    // uint8_t, upper nibble: trigger_mask, lower nibble: trigger_invert_mask
#define REG_TRIGn_OUTPUT (1506)   // uint8_t, upper nibble: trigger_op_t, lower nibble: output_invert (bool)
#define REG_TRIGn_ACTION_w (1507) // uint16_t, upper byte: action_start_index, lower byte: action_end_index

#define REG_CHn_PARAM_w (1637) // Channel pulse generation parameters. see PARAM_TARGET_INDEX() for required offsets

#endif // _MESSAGE_H