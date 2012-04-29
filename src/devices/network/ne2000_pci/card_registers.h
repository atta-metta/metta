#pragma once

namespace ne2k_card
{

// @sa http://www.ti.com/lit/ds/symlink/dp8390d.pdf
// NE2000-compatible banked register set.
enum {
	COMMAND_BANK012_RW = 0x00, // available in all banks at the same offset

	// Bank 0 - primary
	CURRENT_LOCALDMA_ADDRESS0_BANK0_R = 0x01,
	CURRENT_LOCALDMA_ADDRESS1_BANK0_R = 0x02,

	PAGE_START_BANK0_W = 0x01,
	PAGE_STOP_BANK0_W = 0x02,

	BOUNDARY_POINTER_BANK0_RW = 0x03,

	TRANSMIT_STATUS_BANK0_R = 0x04,
	TRANSMIT_PAGE_START_ADDRESS_BANK0_W = 0x04,

	NUMBER_OF_COLLISIONS_BANK0_R = 0x05,
	FIFO_BANK0_R = 0x06,

	TRANSMIT_BYTE_COUNT0_BANK0_W = 0x05,
	TRANSMIT_BYTE_COUNT1_BANK0_W = 0x06,

	INTERRUPT_STATUS_BANK0_RW = 0x07,

	CURRENT_REMOTEDMA_ADDRESS0_BANK0_R = 0x08,
	CURRENT_REMOTEDMA_ADDRESS1_BANK0_R = 0x09,

	REMOTE_START_ADDRESS0_BANK0_W = 0x08,
	REMOTE_START_ADDRESS1_BANK0_W = 0x09,
	REMOTE_BYTE_COUNT0_BANK0_W = 0x0a,
	REMOTE_BYTE_COUNT1_BANK0_W = 0x0b,

	RECEIVE_STATUS_BANK0_R = 0x0c,

	FRAME_ALIGNMENT_ERRORS_TALLY_BANK0_R = 0x0d,
	CRC_ERRORS_TALLY_BANK0_R = 0x0e,
	MISSED_PACKETS_TALLY_BANK0_R = 0x0f,

	RECEIVE_CONFIGURATION_BANK0_W = 0x0c,
	TRANSMIT_CONFIGURATION_BANK0_W = 0x0d,
	DATA_CONFIGURATION_BANK0_W = 0x0e,

	INTERRUPT_MASK_BANK0_W = 0x0f,

	// Bank 1 - additional configuration
	PHYSICAL_ADDRESS0_BANK1_RW = 0x01,
	PHYSICAL_ADDRESS1_BANK1_RW = 0x02,
	PHYSICAL_ADDRESS2_BANK1_RW = 0x03,
	PHYSICAL_ADDRESS3_BANK1_RW = 0x04,
	PHYSICAL_ADDRESS4_BANK1_RW = 0x05,
	PHYSICAL_ADDRESS5_BANK1_RW = 0x06,

	CURRENT_PAGE_BANK1_RW = 0x07,

	MULTICAST_ADDRESS0_BANK1_RW = 0x08,
	MULTICAST_ADDRESS1_BANK1_RW = 0x09,
	MULTICAST_ADDRESS2_BANK1_RW = 0x0a,
	MULTICAST_ADDRESS3_BANK1_RW = 0x0b,
	MULTICAST_ADDRESS4_BANK1_RW = 0x0c,
	MULTICAST_ADDRESS5_BANK1_RW = 0x0d,
	MULTICAST_ADDRESS6_BANK1_RW = 0x0e,
	MULTICAST_ADDRESS7_BANK1_RW = 0x0f,

	// Bank 2 - diagnostic access
	PAGE_START_BANK2_R = 0x01,
	PAGE_STOP_BANK2_R = 0x02,

	CURRENT_LOCALDMA_ADDRESS0_BANK2_W = 0x01,
	CURRENT_LOCALDMA_ADDRESS1_BANK2_W = 0x02,

	REMOTE_NEXT_PACKET_POINTER_BANK2_RW = 0x03,

	TRANSMIT_PAGE_START_ADDRESS_BANK2_R = 0x04,

	LOCAL_NEXT_PACKET_POINTER_BANK2_RW = 0x05,

	ADDRESS_COUNTER_UPPER_BANK2_RW = 0x06,
	ADDRESS_COUNTER_LOWER_BANK2_RW = 0x07,

	RECEIVE_CONFIGURATION_BANK2_R = 0x0c,
	TRANSMIT_CONFIGURATION_BANK2_R = 0x0d,
	DATA_CONFIGURATION_BANK2_R = 0x0e,
	INTERRUPT_MASK_BANK2_R = 0x0f
};

// Bit masks of individual registers
enum {
	COMMAND_STOP = 1 << 0, // D0
	COMMAND_START = 1 << 1, // D1
	COMMAND_TRANSMIT_PACKET = 1 << 2, // D2
	COMMAND_REMOTEDMA_COMMAND = 7 << 3, // D3, D4, D5
	COMMAND_BANK_SELECT = 3 << 6 // D6, D7
};

enum {
	INTERRUPT_STATUS_PACKET_RECEIVED = 1 << 0,
	INTERRUPT_STATUS_PACKET_TRANSMITTED = 1 << 1,
	INTERRUPT_STATUS_RECEIVE_ERROR = 1 << 2,
	INTERRUPT_STATUS_TRANSMIT_ERROR = 1 << 3,
	INTERRUPT_STATUS_OVERWRITE_WARNING = 1 << 4,
	INTERRUPT_STATUS_COUNTER_OVERFLOW = 1 << 5,
	INTERRUPT_STATUS_REMOTEDMA_COMPLETE = 1 << 6,
	INTERRUPT_STATUS_RESET_STATUS = 1 << 7 // Does not generate an INT!
};

enum {
	INTERRUPT_MASK_PACKET_RECEIVED_ENABLE = 1 << 0,
	INTERRUPT_MASK_PACKET_TRANSMITTED_ENABLE = 1 << 1,
	INTERRUPT_MASK_RECEIVE_ERROR_ENABLE = 1 << 2,
	INTERRUPT_MASK_TRANSMIT_ERROR_ENABLE = 1 << 3,
	INTERRUPT_MASK_OVERWRITE_WARNING_ENABLE = 1 << 4,
	INTERRUPT_MASK_COUNTER_OVERFLOW_ENABLE = 1 << 5,
	INTERRUPT_MASK_REMOTEDMA_COMPLETE_ENABLE = 1 << 6,
};

enum {
	DATA_CONFIGURATION_WORD_TRANSFER_SELECT = 1 << 0,
	DATA_CONFIGURATION_BIG_ENDIAN_SELECT = 1 << 1,
	DATA_CONFIGURATION_LONG_ADDRESS_SELECT = 1 << 2,
	DATA_CONFIGURATION_NON_LOOPBACK_SELECT = 1 << 3,
	DATA_CONFIGURATION_AUTO_INITIALIZE_REMOTE = 1 << 4,
	DATA_CONFIGURATION_FIFO_THRESHOLD_SELECT = 3 << 5, // D5, D6
};

enum {
	TRANSMIT_CONFIGURATION_INHIBIT_CRC = 1 << 0,
	TRANSMIT_CONFIGURATION_ENCODED_LOOPBACK_CONTROL = 3 << 1, // D1, D2
	TRANSMIT_CONFIGURATION_AUTO_TRANSMIT_DISABLE = 1 << 3,
	TRANSMIT_CONFIGURATION_COLLISION_OFFSET_ENABLE = 1 << 4
};

enum {
	TRANSMIT_STATUS_PACKET_TRANSMITTED = 1 << 0,
	// D1 is reserved
	TRANSMIT_STATUS_COLLIDED = 1 << 2,
	TRANSMIT_STATUS_ABORTED = 1 << 3,
	TRANSMIT_STATUS_CARRIER_SENSE_LOST = 1 << 4,
	TRANSMIT_STATUS_FIFO_UNDERRUN = 1 << 5,
	TRANSMIT_STATUS_COLLISION_DETECT_HEARTBEAT_FAIL = 1 << 6,
	TRANSMIT_STATUS_OUT_OF_WINDOW_COLLISION = 1 << 7
};

enum {
	RECEIVE_CONFIGURATION_SAVE_ERRORED_PACKETS = 1 << 0,
	RECEIVE_CONFIGURATION_ACCEPT_RUNT_PACKETS = 1 << 1,
	RECEIVE_CONFIGURATION_ACCEPT_BROADCAST = 1 << 2,
	RECEIVE_CONFIGURATION_ACCEPT_MULTICAST = 1 << 3,
	RECEIVE_CONFIGURATION_PROMISCUOUS_PHYSICAL = 1 << 4,
	RECEIVE_CONFIGURATION_MONITOR_MODE = 1 << 5
};

enum {
	RECEIVE_STATUS_PACKET_RECEIVED_INTACT = 1 << 0,
	RECEIVE_STATUS_CRC_ERROR = 1 << 1,
	RECEIVE_STATUS_FRAME_ALIGNMENT_ERROR = 1 << 2,
	RECEIVE_STATUS_FIFO_OVERRUN = 1 << 3,
	RECEIVE_STATUS_MISSED_PACKET = 1 << 4,
	RECEIVE_STATUS_PACKED_WAS_MULTICAST = 1 << 5,
	RECEIVE_STATUS_DISABLED = 1 << 6,
	RECEIVE_STATUS_DEFERRING = 1 << 7
};

// Some predefined values for register fields.

enum {
	COMMAND_REMOTEDMA_READ = 1 << 3,
	COMMAND_REMOTEDMA_WRITE = 2 << 3,
	COMMAND_REMOTEDMA_SEND_PACKET = 3 << 3,
	COMMAND_REMOTEDMA_ABORT = 4 << 3,

	COMMAND_BANK0 = 0 << 6,
	COMMAND_BANK1 = 1 << 6,
	COMMAND_BANK2 = 2 << 6
};

enum {
	DATA_CONFIGURATION_FIFO_THRESHOLD_1WORD = 0 << 5,
	DATA_CONFIGURATION_FIFO_THRESHOLD_2WORDS = 1 << 5,
	DATA_CONFIGURATION_FIFO_THRESHOLD_4WORDS = 2 << 5,
	DATA_CONFIGURATION_FIFO_THRESHOLD_6WORDS = 3 << 5
};

enum {
	TRANSMIT_CONFIGURATION_NORMAL_OPERATION = 0 << 1,
	TRANSMIT_CONFIGURATION_INTERNAL_LOOPBACK = 1 << 1,
	TRANSMIT_CONFIGURATION_EXTERNAL_LOOPBACK1 = 2 << 1,
	TRANSMIT_CONFIGURATION_EXTERNAL_LOOPBACK2 = 3 << 1
};

} // namespace ne2k_card




















