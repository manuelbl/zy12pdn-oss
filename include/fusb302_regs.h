//
// USB Power Delivery Sink Using FUSB302B
// Copyright (c) 2020 Manuel Bleichenbacher
//
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// FUSB302 registers
//

#pragma once

#include <stdint.h>

namespace usb_pd {

/// FUSB302 register addresses
enum reg : uint8_t {
    reg_device_id = 0x01,
    reg_switches0 = 0x02,
    reg_switches1 = 0x03,
    reg_measure = 0x04,
    reg_slice = 0x05,
    reg_control0 = 0x06,
    reg_control1 = 0x07,
    reg_control2 = 0x08,
    reg_control3 = 0x09,
    reg_mask = 0x0a,
    reg_power = 0x0b,
    reg_reset = 0x0c,
    reg_ocpreg = 0x0d,
    reg_maska = 0x0e,
    reg_maskb = 0x0f,
    reg_control4 = 0x10,
    reg_status0a = 0x3c,
    reg_status1a = 0x3d,
    reg_interrupta = 0x3e,
    reg_interruptb = 0x3f,
    reg_status0 = 0x40,
    reg_status1 = 0x41,
    reg_interrupt = 0x42,
    reg_fifos = 0x43
};

/// FUSB302 register SWITCHES0 values
enum switches0 : uint8_t {
    switches0_pu_en2 = 0x01 << 7,
    switches0_pu_en1 = 0x01 << 6,
    switches0_vconn_cc2 = 0x01 << 5,
    switches0_vconn_cc1 = 0x01 << 4,
    switches0_meas_cc2 = 0x01 << 3,
    switches0_meas_cc1 = 0x01 << 2,
    switches0_pdwn2 = 0x01 << 1,
    switches0_pdwn1 = 0x01 << 0,
    switches0_none = 0x00
};

/// FUSB302 register SWITCHES1 values
enum switches1 : uint8_t {
    switches1_none = 0,
    switches1_powerrole = 0x01 << 7,
    switches1_specrev_mask = 0x03 << 5,
    switches1_specrev_rev_1_0 = 0x00 << 5,
    switches1_specrev_rev_2_0 = 0x01 << 5,
    switches1_datarole = 0x01 << 4,
    switches1_auto_crc = 0x01 << 2,
    switches1_txcc2 = 0x01 << 1,
    switches1_txcc1 = 0x01 << 0
};

/// FUSB302 register MEASURE values
enum measure : uint8_t {
    measure_meas_vbus = 0x01 << 6,
    measure_meas_mdac_mask = 0x3f
};

/// FUSB302 register SLICE values
enum slice : uint8_t {
    slice_sdac_hys_mask = 0x03 << 6,
    slice_sdac_hys_255mv = 0x03 << 6,
    slice_sdac_hys_170mv = 0x02 << 6,
    slice_sdac_hys_085mv = 0x01 << 6,
    slice_sdac_hys_none = 0x00 << 6,
    slice_sdac_mask = 0x3f << 0
};

/// FUSB302 register CONTROL0 values
enum control0 : uint8_t {
    control0_tx_flush = 0x01 << 6,
    control0_int_mask = 0x01 << 5,
    control0_host_cur_mask = 0x03 << 2,
    control0_host_cur_no = 0x00 << 2,
    control0_host_cur_usb_def = 0x01 << 2,
    control0_host_cur_1p5a = 0x02 << 2,
    control0_host_cur_3p0a = 0x03 << 2,
    control0_auto_pre = 0x01 << 1,
    control0_tx_start = 0x01 << 0,
    control0_none = 0
};

/// FUSB302 register CONTROL1 values
enum control1 : uint8_t {
    control1_ensop2db = 0x01 << 6,
    control1_ensop1db = 0x01 << 5,
    control1_bist_mode2 = 0x01 << 4,
    control1_rx_flush = 0x01 << 2,
    control1_ensop2 = 0x01 << 1,
    control1_ensop1 = 0x01 << 0
};

/// FUSB302 register CONTROL2 values
enum control2 : uint8_t {
    control2_tog_save_pwr_mask = 0x03 << 6,
    control2_tog_rd_only = 0x01 << 5,
    control2_wake_en = 0x01 << 3,
    control2_mode_mask = 0x03 << 1,
    control2_mode_src_polling = 0x03 << 1,
    control2_mode_snk_polling = 0x02 << 1,
    control2_mode_drp_polling = 0x01 << 1,
    control2_toggle = 0x01 << 0
};

/// FUSB302 register CONTROL3 values
enum control3 : uint8_t {
    control3_send_hard_reset = 0x03 << 6,
    control3_bist_tmode = 0x01 << 5,
    control3_auto_hardreset = 0x01 << 4,
    control3_auto_softreset = 0x01 << 3,
    control3_n_retries_mask = 0x03 << 1,
    control3_3_retries = 0x03 << 1,
    control3_2_retries = 0x02 << 1,
    control3_1_retry = 0x01 << 1,
    control3_0_retries = 0x00 << 1,
    control3_auto_retry = 0x01 << 0
};

/// FUSB302 register MASK values
enum mask : uint8_t {
    mask_m_all = 0xff,
    mask_m_vbusok = 0x01 << 7,
    mask_m_activity = 0x01 << 6,
    mask_m_comp_chng = 0x01 << 5,
    mask_m_crc_chk = 0x01 << 4,
    mask_m_alert = 0x01 << 3,
    mask_m_wake = 0x01 << 2,
    mask_m_collision = 0x01 << 1,
    mask_m_bc_lvl = 0x01 << 0
};

/// FUSB302 register POWER values
enum power : uint8_t {
    power_pwr_mask = 0x0f << 0,
    power_pwr_all = 0x0f << 0,
    power_pwr_int_osc = 0x01 << 3,
    power_pwr_receiver = 0x01 << 2,
    power_pwr_measure = 0x01 << 1,
    power_pwr_bandgap = 0x01 << 0
};

/// FUSB302 register RESET values
enum reset : uint8_t {
    reset_pd_reset = 0x01 << 1,
    reset_sw_res = 0x01 << 0
};

/// FUSB302 register MASKA values
enum maska : uint8_t {
    maska_m_all = 0xff,
    maska_m_none = 0x00,
    maska_m_ocp_temp = 0x01 << 7,
    maska_m_togdone = 0x01 << 6,
    maska_m_softfail = 0x01 << 5,
    maska_m_retry_fail = 0x01 << 4,
    maska_m_hardsent = 0x01 << 3,
    maska_m_txsent = 0x01 << 2,
    maska_m_softrst = 0x01 << 1,
    maska_m_hardrst = 0x01 << 0
};

/// FUSB302 register MASKB values
enum maskb : uint8_t {
    maskb_m_all = 0x01,
    maskb_m_none = 0x00,
    maskb_m_gcrcsent = 0x01 << 0
};

/// FUSB302 register STATUS1A values
enum status1a : uint8_t {
    status1a_togss_mask = 0x07 << 3,
    status1a_togss_toggle_running = 0x00 << 3,
    status1a_togss_src_on_cc1 = 0x01 << 3,
    status1a_togss_src_on_cc2 = 0x02 << 3,
    status1a_togss_snk_on_cc1 = 0x05 << 3,
    status1a_togss_snk_on_cc2 = 0x06 << 3,
    status1a_togss_auto_accessory = 0x07 << 3,

    status1a_rxsop2db = 0x01 << 2,
    status1a_rxsop1db = 0x01 << 1,
    status1a_rxsop = 0x01 << 0
};

/// FUSB302 register INTERRUPTA values
enum interrupta : uint8_t {
    interrupta_i_ocp_temp = 0x01 << 7,
    interrupta_i_togdone = 0x01 << 6,
    interrupta_i_softfail = 0x01 << 5,
    interrupta_i_retryfail = 0x01 << 4,
    interrupta_i_hardsent = 0x01 << 3,
    interrupta_i_txsent = 0x01 << 2,
    interrupta_i_softrst = 0x01 << 1,
    interrupta_i_hardrst = 0x01 << 0
};

/// FUSB302 register INTERRUPTB values
enum interruptb : uint8_t {
    interruptb_i_gcrcsent = 0x01 << 0
};

/// FUSB302 register STATUS0 values
enum status0 : uint8_t {
    status0_vbusok = 0x01 << 7,
    status0_activity = 0x01 << 6,
    status0_comp = 0x01 << 5,
    status0_crc_chk = 0x01 << 4,
    status0_alert_chk = 0x01 << 3,
    status0_wake = 0x01 << 2,
    status0_bc_lvl_mask = 0x03 << 0
};

/// FUSB302 register STATUS1 values
enum status1 : uint8_t {
    status1_rxsop2_mask = 0x01 << 7,
    status1_rxsop1 = 0x01 << 6,
    status1_rx_empty = 0x01 << 5,
    status1_rx_full = 0x01 << 4,
    status1_tx_empty = 0x01 << 3,
    status1_tx_full = 0x01 << 2,
    status1_ovrtemp = 0x01 << 1,
    status1_ocp = 0x01 << 0
};

/// FUSB302 register INTERRUPT values
enum interrupt : uint8_t {
    interrupt_none = 0,
    interrupt_i_vbusok = 0x01 << 7,
    interrupt_i_activity = 0x01 << 6,
    interrupt_i_comp_chng = 0x01 << 5,
    interrupt_i_crc_chk = 0x01 << 4,
    interrupt_i_alert = 0x01 << 3,
    interrupt_i_wake = 0x01 << 2,
    interrupt_i_collision = 0x01 << 1,
    interrupt_i_bc_lvl = 0x01 << 0
};

/// Tokens used in FUSB302B FIFO
enum token : uint8_t {
    token_txon = 0xa1,
    token_sop1 = 0x12,
    token_sop2 = 0x13,
    token_sop3 = 0x1b,
    token_reset1 = 0x15,
    token_reset2 = 0x16,
    token_packsym = 0x80,
    token_jam_crc = 0xff,
    token_eop = 0x14,
    token_txoff = 0xfe
};

} // namespace usb_pd
