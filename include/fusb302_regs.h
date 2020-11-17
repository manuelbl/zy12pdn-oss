//
// USB Power Delivery Sink Using FUSB302B
// Copyright (c) 2020 Manuel Bleichenbacher
//
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// FUSB302 registers
//

#ifndef _fusb302_regs_h_
#define _fusb302_regs_h_

#include <stdint.h>

//
// Macro adding bit mask operators to an enum class.
// The enum class is assumed to be derived from 'uint8_t.'
//
// Operators:
//  &     - bitwise and
//  |     - bitwise or
//  ~     - bitwise not
//  *     - conversion to 'uint8_t'
//
#define USB_PD_ADD_BITMAKS_OPS(ENUM)                                                                                   \
    constexpr ENUM operator|(ENUM lhs, ENUM rhs)                                                                       \
    {                                                                                                                  \
        return static_cast<ENUM>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));                               \
    }                                                                                                                  \
    constexpr ENUM operator&(ENUM lhs, ENUM rhs)                                                                       \
    {                                                                                                                  \
        return static_cast<ENUM>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));                               \
    }                                                                                                                  \
    constexpr ENUM operator~(ENUM mask) { return static_cast<ENUM>(~static_cast<uint8_t>(mask)); }                     \
    constexpr uint8_t operator*(ENUM mask) { return static_cast<uint8_t>(mask); }

namespace usb_pd {

/// FUSB302 register addresses
enum class reg : uint8_t {
    device_id = 0x01,
    switches0 = 0x02,
    switches1 = 0x03,
    measure = 0x04,
    slice = 0x05,
    control0 = 0x06,
    control1 = 0x07,
    control2 = 0x08,
    control3 = 0x09,
    mask = 0x0a,
    power = 0x0b,
    reset = 0x0c,
    ocpreg = 0x0d,
    maska = 0x0e,
    maskb = 0x0f,
    control4 = 0x10,
    status0a = 0x3c,
    status1a = 0x3d,
    interrupta = 0x3e,
    interruptb = 0x3f,
    status0 = 0x40,
    status1 = 0x41,
    interrupt = 0x42,
    fifos = 0x43
};

/// FUSB302 register SWITCHES0 values
enum class reg_switches0 : uint8_t {
    pu_en2 = 0x01 << 7,
    pu_en1 = 0x01 << 6,
    vconn_cc2 = 0x01 << 5,
    vconn_cc1 = 0x01 << 4,
    meas_cc2 = 0x01 << 3,
    meas_cc1 = 0x01 << 2,
    pdwn2 = 0x01 << 1,
    pdwn1 = 0x01 << 0,
    none = 0x00
};
USB_PD_ADD_BITMAKS_OPS(reg_switches0)

/// FUSB302 register SWITCHES1 values
enum class reg_switches1 : uint8_t {
    none = 0,
    powerrole = 0x01 << 7,
    specrev_mask = 0x03 << 5,
    specrev_rev_1_0 = 0x00 << 5,
    specrev_rev_2_0 = 0x01 << 5,
    datarole = 0x01 << 4,
    auto_crc = 0x01 << 2,
    txcc2 = 0x01 << 1,
    txcc1 = 0x01 << 0
};
USB_PD_ADD_BITMAKS_OPS(reg_switches1)

/// FUSB302 register MEASURE values
enum class reg_measure : uint8_t { meas_vbus = 0x01 << 6, meas_mdac_mask = 0x3f };
USB_PD_ADD_BITMAKS_OPS(reg_measure)

/// FUSB302 register SLICE values
enum class reg_slice : uint8_t {
    sdac_hys_mask = 0x03 << 6,
    sdac_hys_255mv = 0x03 << 6,
    sdac_hys_170mv = 0x02 << 6,
    sdac_hys_085mv = 0x01 << 6,
    sdac_hys_none = 0x00 << 6,
    sdac_mask = 0x3f << 0
};
USB_PD_ADD_BITMAKS_OPS(reg_slice)

/// FUSB302 register CONTROL0 values
enum class reg_control0 : uint8_t {
    tx_flush = 0x01 << 6,
    int_mask = 0x01 << 5,
    host_cur_mask = 0x03 << 2,
    host_cur_no = 0x00 << 2,
    host_cur_usb_def = 0x01 << 2,
    host_cur_1p5a = 0x02 << 2,
    host_cur_3p0a = 0x03 << 2,
    auto_pre = 0x01 << 1,
    tx_start = 0x01 << 0,
    none = 0
};
USB_PD_ADD_BITMAKS_OPS(reg_control0)

/// FUSB302 register CONTROL1 values
enum class reg_control1 : uint8_t {
    ensop2db = 0x01 << 6,
    ensop1db = 0x01 << 5,
    bist_mode2 = 0x01 << 4,
    rx_flush = 0x01 << 2,
    ensop2 = 0x01 << 1,
    ensop1 = 0x01 << 0
};
USB_PD_ADD_BITMAKS_OPS(reg_control1)

/// FUSB302 register CONTROL2 values
enum class reg_control2 : uint8_t {
    tog_save_pwr_mask = 0x03 << 6,
    tog_rd_only = 0x01 << 5,
    wake_en = 0x01 << 3,
    mode_mask = 0x03 << 1,
    mode_src_polling = 0x03 << 1,
    mode_snk_polling = 0x02 << 1,
    mode_drp_polling = 0x01 << 1,
    toggle = 0x01 << 0
};
USB_PD_ADD_BITMAKS_OPS(reg_control2)

/// FUSB302 register CONTROL3 values
enum class reg_control3 : uint8_t {
    send_hard_reset = 0x03 << 6,
    bist_tmode = 0x01 << 5,
    auto_hardreset = 0x01 << 4,
    auto_softreset = 0x01 << 3,
    n_retries_mask = 0x03 << 1,
    _3_retries = 0x03 << 1,
    _2_retries = 0x02 << 1,
    _1_retry = 0x01 << 1,
    _0_retries = 0x00 << 1,
    auto_retry = 0x01 << 0
};
USB_PD_ADD_BITMAKS_OPS(reg_control3)

/// FUSB302 register MASK values
enum class reg_mask : uint8_t {
    m_all = 0xff,
    m_vbusok = 0x01 << 7,
    m_activity = 0x01 << 6,
    m_comp_chng = 0x01 << 5,
    m_crc_chk = 0x01 << 4,
    m_alert = 0x01 << 3,
    m_wake = 0x01 << 2,
    m_collision = 0x01 << 1,
    m_bc_lvl = 0x01 << 0
};
USB_PD_ADD_BITMAKS_OPS(reg_mask)

/// FUSB302 register POWER values
enum class reg_power : uint8_t {
    pwr_mask = 0x0f << 0,
    pwr_int_osc = 0x01 << 3,
    pwr_receiver = 0x01 << 2,
    pwr_measure = 0x01 << 1,
    pwr_bandgap = 0x01 << 0
};
USB_PD_ADD_BITMAKS_OPS(reg_power)

/// FUSB302 register RESET values
enum class reg_reset : uint8_t { pd_reset = 0x01 << 1, sw_res = 0x01 << 0 };
USB_PD_ADD_BITMAKS_OPS(reg_reset)

/// FUSB302 register MASKA values
enum class reg_maska : uint8_t {
    m_all = 0xff,
    m_ocp_temp = 0x01 << 7,
    m_togdone = 0x01 << 6,
    m_softfail = 0x01 << 5,
    m_retry_fail = 0x01 << 4,
    m_hardsent = 0x01 << 3,
    m_txsent = 0x01 << 2,
    m_softrst = 0x01 << 1,
    m_hardrst = 0x01 << 0
};
USB_PD_ADD_BITMAKS_OPS(reg_maska)

/// FUSB302 register MASKB values
enum class reg_maskb : uint8_t { m_all = 0x01, m_gcrcsent = 0x01 << 0 };
USB_PD_ADD_BITMAKS_OPS(reg_maskb)

/// FUSB302 register STATUS1A values
enum class reg_status1a : uint8_t {
    togss_mask = 0x07 << 3,
    togss_toggle_running = 0x00 << 3,
    togss_src_on_cc1 = 0x01 << 3,
    togss_src_on_cc2 = 0x02 << 3,
    togss_snk_on_cc1 = 0x05 << 3,
    togss_snk_on_cc2 = 0x06 << 3,
    togss_auto_accessory = 0x07 << 3,

    rxsop2db = 0x01 << 2,
    rxsop1db = 0x01 << 1,
    rxsop = 0x01 << 0
};
USB_PD_ADD_BITMAKS_OPS(reg_status1a)

/// FUSB302 register INTERRUPTA values
enum class reg_interrupta : uint8_t {
    i_ocp_temp = 0x01 << 7,
    i_togdone = 0x01 << 6,
    i_softail = 0x01 << 5,
    i_retryfail = 0x01 << 4,
    i_hardsent = 0x01 << 3,
    i_txsent = 0x01 << 2,
    i_softrst = 0x01 << 1,
    i_hardrst = 0x01 << 0
};
USB_PD_ADD_BITMAKS_OPS(reg_interrupta)

/// FUSB302 register INTERRUPTB values
enum class reg_interruptb : uint8_t { i_gcrcsent = 0x01 << 0 };
USB_PD_ADD_BITMAKS_OPS(reg_interruptb)

/// FUSB302 register STATUS0 values
enum class reg_status0 : uint8_t {
    vbusok = 0x01 << 7,
    activity = 0x01 << 6,
    comp = 0x01 << 5,
    crc_chk = 0x01 << 4,
    alert_chk = 0x01 << 3,
    wake = 0x01 << 2,
    bc_lvl_mask = 0x03 << 0
};
USB_PD_ADD_BITMAKS_OPS(reg_status0)

/// FUSB302 register STATUS1 values
enum class reg_status1 : uint8_t {
    rxsop2_mask = 0x01 << 7,
    rxsop1 = 0x01 << 6,
    rx_empty = 0x01 << 5,
    rx_full = 0x01 << 4,
    tx_empty = 0x01 << 3,
    tx_full = 0x01 << 2,
    ovrtemp = 0x01 << 1,
    ocp = 0x01 << 0
};
USB_PD_ADD_BITMAKS_OPS(reg_status1)

/// FUSB302 register INTERRUPT values
enum class reg_interrupt : uint8_t {
    none = 0,
    i_vbusok = 0x01 << 7,
    i_activity = 0x01 << 6,
    i_comp_chng = 0x01 << 5,
    i_crc_chk = 0x01 << 4,
    i_alert = 0x01 << 3,
    i_wake = 0x01 << 2,
    i_collision = 0x01 << 1,
    i_bc_lvl = 0x01 << 0
};
USB_PD_ADD_BITMAKS_OPS(reg_interrupt)

} // namespace usb_pd

#endif
