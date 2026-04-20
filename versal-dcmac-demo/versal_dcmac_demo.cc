/*
 * Top level of the Versal DCMAC FixedE mode CoSim example.
 *
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: MIT
 *
 * Overview
 * - Only Port 0 is enabled.
 * - The PHY is QEMU CoSim Remote-port's USER channels:
 *   - RX: channel 256
 *   - TX: channel 288 (= 256 + 32 = 256 + VERSAL_NUM_USER_PORTS)
 *   - To connect PHY to the QEMU user-mode network backend, add QEMU option:
 *     -netdev user,id=net4 -device remote-port-net,rp-adaptor0=/amba@0/cosim@0,rp-chan0=256,rp-chan1=288,netdev=net4
 *
 * This is based on Versal example design
 *   https://github.com/Xilinx/Vivado-Design-Tutorials/tree/2023.2/Device_Architecture_Tutorials/Versal/NoC_System_Designs/DCMAC_NoC
 * A design expected by DCMAC support of:
 *   https://github.com/Xilinx/linux-xlnx/blob/xilinx-v2025.2/drivers/net/ethernet/xilinx/xilinx_axienet_main.c#L4963
 *
 * See ./vivado_ref/README.md for detail
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include <functional>
#include <iostream>
#include <sstream>
#include <cstdio>

#include <systemc>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/tlm_quantumkeeper.h>

using namespace std;
using namespace sc_core;
using namespace sc_dt;
using namespace tlm;
using namespace tlm_utils;

#include "trace.h"
#include "soc/interconnect/iconnect.h"
#include "soc/xilinx/versal/xilinx-versal.h"
#include "soc/net/ethernet/amd/dcmac/dcmac-eth.h"
#include "soc/dma/amd/axidma/axidma.h"
#include "tests/test-modules/memory.h"
#include "utils/regapi.h"

#define HEX(V_) std::hex << std::showbase << std::nouppercase << (V_)

#define DBUG(M_) do { \
        std::ostringstream oss_;                                        \
        oss_ << M_;                                                     \
        SC_REPORT_INFO_VERB("Top", oss_.str().c_str(), sc_core::SC_DEBUG); \
    } while (0)

#ifndef DCMAC_BASE_ADDR
#define DCMAC_BASE_ADDR   (0xa4000000ULL)
#endif

#ifndef GT_CTL_BASE_ADDR
#define GT_CTL_BASE_ADDR  (0xa4130000ULL)
#endif

#ifndef GT_TXDP_BASE_ADDR
#define GT_TXDP_BASE_ADDR (0xa4140000ULL)
#endif

#ifndef GT_RXDP_BASE_ADDR
#define GT_RXDP_BASE_ADDR (0xa4150000ULL)
#endif

#ifndef GT_RSTS_DONE_BASE_ADDR
#define GT_RSTS_DONE_BASE_ADDR (0xa4160000ULL)
#endif

#ifndef GT_RSTS_BASE_ADDR
#define GT_RSTS_BASE_ADDR (0xa4170000ULL)
#endif

#ifndef DMA_BASE_ADDR
#define DMA_BASE_ADDR     (0xa4180000ULL)
#endif

#define DCMAC_REGS_SIZE  (1ULL << 20)
#define DCMAC_REGS_END   (DCMAC_BASE_ADDR + DCMAC_REGS_SIZE)
#define DCMAC_REGS_SPAN  (DCMAC_REGS_SIZE - 1)
#define DCMAC_REGS_DEVS  (7)

#define DMA_REGS_SIZE    (0x100)
#define DMA_REGS_SPAN    (DMA_REGS_SIZE - 1)
#define DMA_REGS_DEVS    (1)

#define GT_GPIO_DEVS     (5)

#define NR_MASTERS       (3)
#define NR_DEVICES       (DCMAC_REGS_DEVS + DMA_REGS_DEVS + GT_GPIO_DEVS + 1)

/*
 * A simple model of the AXIGPIO register slave interface to simulate
 * the dcmac_0_core_0_gt_reset_controller_0/_1 for responding to GPIO
 * outputs manipulated by Linux DCMAC driver via:
 *   Design Node                  Device Tree Prop       Driver Handle
 * ---------------------------------------------------------------------------
 *   axi_gpio_gt_ctl[31:0]        gt-ctrl-gpios          gds_gt_ctrl
 * - axi_gpio_tx_datapath[23:0]   gt-tx-dpath-gpios      gds_gt_tx_dpath
 * - axi_gpio_rx_datapath[23:0]   gt-rx-dpath-gpios      gds_gt_rx_dpath
 * - axi_reset_dyn[13:0]          gt-rsts-gpios          gds_gt_rsts
 * - axi_reset_done_dyn[23:0]     gt-tx-rst-done-gpios   gds_gt_tx_reset_done
 * - axi_reset_done_dyn[55:32]    gt-rx-rst-done-gpios   gds_gt_rx_reset_done
 *
 * Of all pins defined by the DCMAC example design, only pins manipulated
 * by driver are enumerated, and using names resembling that of driver's:
 *   https://github.com/Xilinx/linux-xlnx/blob/xilinx-v2025.2/drivers/net/ethernet/xilinx/xilinx_axienet.h#L797
 *
 */
REG32(GPIO_DATA, 0x00)      // https://docs.amd.com/r/en-US/pg144-axi-gpio
REG32(GPIO_TRI, 0x04)
REG32(GPIO2_DATA, 0x08)
REG32(GPIO2_TRI, 0x0c)

enum {  GT_GPIO_REGS_SPAN = 0x100 - 1 };

typedef regapi_block<uint32_t, (R_GPIO2_TRI + 1)> regapi_gt_gpio;

FIELD(GPIO_DATA,  DCMAC_GT_RESET_ALL,      0, 1)    /* gds_gt_ctrl */
FIELD(GPIO_DATA,  DCMAC_GT_TX_PRECURSOR,  12, 6)
FIELD(GPIO_DATA,  DCMAC_GT_TX_POSTCURSOR, 18, 6)
FIELD(GPIO_DATA,  DCMAC_GT_MAINCURSOR,    24, 7)

FIELD(GPIO_DATA,  DCMAC_GT_TXDPATH_RST,    0, 24)   /* gds_gt_tx_dpath */
FIELD(GPIO_DATA,  DCMAC_GT_RXDPATH_RST,    0, 24)   /* gds_gt_rx_dpath */

FIELD(GPIO_DATA,  DCMAC_GT_TX_CORE_RST,    0, 1)    /* gds_gt_rsts */
FIELD(GPIO_DATA,  DCMAC_GT_RX_CORE_RST,    1, 1)
FIELD(GPIO_DATA,  DCMAC_GT_TX_SERDES_RST,  2, 6)
FIELD(GPIO_DATA,  DCMAC_GT_RX_SERDES_RST,  8, 6)

/* Linux driver's DCMAC_GT_RESET_DONE_MASK == 0x0F */
FIELD(GPIO_DATA,  DCMAC_GT_RESET_DONE,  0, 4)   /* gds_gt_tx_reset_done */
FIELD(GPIO2_DATA, DCMAC_GT_RESET_DONE,  0, 4)   /* gds_gt_rx_reset_done */

static const regapi_info<uint32_t> dcmac_gpio_regsinfo[] = {
    {    .name = "GPIO_DATA",
         .addr = A_GPIO_DATA,
    }, { .name = "GPIO_TRI",
         .addr = A_GPIO_TRI,
    }, { .name = "GPIO2_DATA",
         .addr = A_GPIO2_DATA,
    }, { .name = "GPIO2_TRI",
         .addr = A_GPIO2_TRI,
    },

    {}
};

struct dcmac_gpio_regs : public regapi_gt_gpio
{
    typedef std::function<void(unsigned)> pre_reader;
    pre_reader pre_read;
    simple_target_socket<dcmac_gpio_regs> socket;

    dcmac_gpio_regs(sc_module_name name, pre_reader pr = nullptr) :
        regapi_gt_gpio(name, dcmac_gpio_regsinfo),
        pre_read(pr),
        socket("target_socket")
    {
        if (pr) {
            socket.register_b_transport(this, &dcmac_gpio_regs::pre_b_transport);
        } else {
            socket.register_b_transport(this, &dcmac_gpio_regs::reg_b_transport);
        }
    }

    void pre_b_transport(tlm_generic_payload& tr, sc_time& delay) {
        if (tr.is_read() && pre_read) {
            pre_read(tr.get_address() / 4);
        }
        reg_b_transport(tr, delay);
    }
};

struct Top;

SC_MODULE(dcmac_gt_gpios)
{
    dcmac_gpio_regs gt_ctrl_rb;
    dcmac_gpio_regs gt_txdp_rb;
    dcmac_gpio_regs gt_rxdp_rb;
    dcmac_gpio_regs gt_rsts_rb;
    dcmac_gpio_regs gt_rsts_done_rb;

    dcmac_gt_gpios(sc_module_name name) :
        sc_module(name),
        gt_ctrl_rb("axi_gpio_gt_ctl"),
        gt_txdp_rb("axi_gpio_tx_datapath"),
        gt_rxdp_rb("axi_gpio_rx_datapath"),
        gt_rsts_rb("axi_reset_dyn"),
        gt_rsts_done_rb("axi_reset_done_dyn",
                        [this](unsigned indx) {
                            this->gt_rsts_done_pre_read(indx); })
    {}

    void gt_rsts_done_pre_read(unsigned indx) {
        uint32_t dpath_bits;
        uint32_t rsts_bits;

        #define GT_EX(RB_, F_) ARRAY_FIELD_EX((RB_).regs, GPIO_DATA, F_)

        if (indx == R_GPIO_DATA) {
            /* Evaluate tx-reset-done */
            dpath_bits = GT_EX(gt_txdp_rb, DCMAC_GT_TXDPATH_RST);
            rsts_bits  = GT_EX(gt_rsts_rb, DCMAC_GT_TX_SERDES_RST) << 1;
            rsts_bits |= GT_EX(gt_rsts_rb, DCMAC_GT_TX_CORE_RST);
            goto eval;
        }

        if (indx == R_GPIO2_DATA) {
            /* Evaluate rx-reset-done */
            dpath_bits = GT_EX(gt_rxdp_rb, DCMAC_GT_RXDPATH_RST);
            rsts_bits  = GT_EX(gt_rsts_rb, DCMAC_GT_RX_SERDES_RST) << 1;
            rsts_bits |= GT_EX(gt_rsts_rb, DCMAC_GT_RX_CORE_RST);
            goto eval;
        }

        return; /* Nothing to update for reading other registers */

    eval:
        gt_rsts_done_rb.regs[indx] = 0;

        /* Done bits are set ONLY if all reset signals are 0 */
        if (GT_EX(gt_ctrl_rb, DCMAC_GT_RESET_ALL)) {
            return;
        }
        if (dpath_bits) {
            return;
        }
        if (rsts_bits) {
            return;
        }

        gt_rsts_done_rb.regs[indx] = R_GPIO_DATA_DCMAC_GT_RESET_DONE_MASK;
        #undef GT_EX
    }
};

SC_MODULE(Top)
{
    SC_HAS_PROCESS(Top);
    iconnect<NR_MASTERS, NR_DEVICES> bus;
    xilinx_versal versal;

    dcmac_gt_gpios eth_gt_ctl;
    amd_dcmac_eth eth;      // https://docs.amd.com/r/en-US/pg369-dcmac
    amd_axidma dma;         // https://docs.amd.com/r/en-US/pg021_axi_dma

    sc_clock clk;
    sc_signal<bool> rst;

    void pulse_reset() {
        /* Pulse the reset signal once on simulation start */
        rst.write(true);
        wait(100, SC_US);
        rst.write(false);
        wait(100, SC_US);
    }

    Top(sc_module_name name, const char *sk_descr, sc_time quantum) :
        sc_module(name),
        bus("bus"),
        versal("versal", sk_descr, remoteport_tlm_sync_untimed_ptr, true),
        eth_gt_ctl("dcmac_gt_ctl"),
        eth("dcmac_eth", true),
        dma("axidma"),
        clk("clk", sc_time(20, SC_US)),
        rst("rst")
    {
        m_qk.set_global_quantum(quantum);

        SC_THREAD(pulse_reset);
        versal.rst(rst);
        eth.rst(rst);
        dma.rst(rst);

        bus.memmap(DMA_BASE_ADDR, DMA_REGS_SPAN,
                   ADDRMODE_RELATIVE, -1, dma.regs_target_socket);

        bus.memmap(GT_CTL_BASE_ADDR, GT_GPIO_REGS_SPAN,
                   ADDRMODE_RELATIVE, -1, eth_gt_ctl.gt_ctrl_rb.socket);

        bus.memmap(GT_TXDP_BASE_ADDR, GT_GPIO_REGS_SPAN,
                   ADDRMODE_RELATIVE, -1, eth_gt_ctl.gt_txdp_rb.socket);

        bus.memmap(GT_RXDP_BASE_ADDR, GT_GPIO_REGS_SPAN,
                   ADDRMODE_RELATIVE, -1, eth_gt_ctl.gt_rxdp_rb.socket);

        bus.memmap(GT_RSTS_BASE_ADDR, GT_GPIO_REGS_SPAN,
                   ADDRMODE_RELATIVE, -1, eth_gt_ctl.gt_rsts_rb.socket);

        bus.memmap(GT_RSTS_DONE_BASE_ADDR, GT_GPIO_REGS_SPAN,
                   ADDRMODE_RELATIVE, -1, eth_gt_ctl.gt_rsts_done_rb.socket);

        // Connect dcmac's MMIO targets into a bus
        std::vector<amd_dcmac_eth::reg_spec> dcmac_regs
                                             = eth.get_regs(DCMAC_BASE_ADDR);
        for (amd_dcmac_eth::reg_spec& r : dcmac_regs) {
            bus.memmap(r.addr, (r.size - 1), ADDRMODE_RELATIVE, -1, r.sk);
        }

        // All other ranges go into Versal's system address space
        bus.memmap(0x0LL, UINT64_MAX - 1,
                   ADDRMODE_RELATIVE, -1, *(versal.s_axi_fpd));

        // Let other Versal initiators access the bus
        versal.m_axi_lpd->bind(*(bus.t_sk[0]));
        versal.m_axi_fpd->bind(*(bus.t_sk[1]));

        // DMA engine wire up.
        // Loop mm2s_control_stream to s2mm_status_stream, both optional.
        dma.dma_init_socket.bind(*(bus.t_sk[2]));
        dma.mm2s_control_stream_socket.bind(dma.s2mm_status_stream_socket);

        dma.mm2s_irq(versal.pl2ps_irq[0]);
        dma.s2mm_irq(versal.pl2ps_irq[1]);

        // Wire up DMA for dcmac port 0
        int port = 0;
        dma.mm2s_stream_socket.bind(eth.mac[port].mac_tx_socket);
        eth.mac[port].mac_rx_socket.bind(dma.s2mm_stream_socket);

        // Wire up phy for dcmac port 0
        versal.user_master[0]->bind(eth.mac[port].phy_rx_socket);
        eth.mac[port].phy_tx_socket.bind(*versal.user_slave[0]);

        // Loop back unused ETH ports to avoid 'port not bound' error
        for (unsigned pn = 1; pn < 6; pn++) {
            eth.mac[pn].mac_tx_socket.bind(eth.mac[pn].mac_rx_socket);
            eth.mac[pn].phy_tx_socket.bind(eth.mac[pn].phy_rx_socket);
        }

        versal.tie_off();
    }

private:
    tlm_utils::tlm_quantumkeeper m_qk;
};

void usage(const char *prog)
{
    cerr << "Usage: " << endl
         << "  " << prog << " socket-path sync-quantum-ns" << endl;
}

void unbuffered_output()
{
    // Force all outputs to be flushed often so the redirection to
    // a file can be monitored using tail -f
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);

    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    sc_report_handler::set_actions(SC_INFO,    SC_LOG | SC_DISPLAY);
    sc_report_handler::set_actions(SC_WARNING, SC_LOG | SC_DISPLAY);
    sc_report_handler::set_actions(SC_ERROR,   SC_LOG | SC_DISPLAY | SC_CACHE_REPORT);
    sc_report_handler::set_actions(SC_FATAL,   SC_LOG | SC_DISPLAY | SC_ABORT);

    if (std::getenv("DCMAC_DEMO_DEBUG")) {
        sc_report_handler::set_verbosity_level(SC_DEBUG);
    }
}

int sc_main(int argc, char* argv[])
{
    int rc = 0;
    Top *top;
    uint64_t sync_quantum;

    unbuffered_output();

    if (argc < 3) {
        sync_quantum = 10000;
    } else {
        sync_quantum = strtoull(argv[2], NULL, 10);
    }

    sc_set_time_resolution(1, SC_PS);

    top = new Top("Top", argv[1], sc_time((double) sync_quantum, SC_NS));

    // Even without sufficient args, elaborate the design to catch
    // wire-up errors before displaying usage.
    if (argc < 3) {
        sc_start(1, SC_PS);
        sc_stop();
        usage(argv[0]);
        rc = EXIT_FAILURE;
    } else {
        DBUG("Running");
        sc_start();
    }

    delete top;
    return rc;
}
