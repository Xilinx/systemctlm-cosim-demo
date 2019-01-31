/*
 * Partial model of a TLM XGMII PHY.
 *
 * Copyright (c) 2019 Xilinx Inc.
 * Written by Edgar E. Iglesias.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __TLM_XGMII_PHY_H__
#define __TLM_XGMII_PHY_H__

#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <stdio.h>
#include "tlm-bridges/tlm2xgmii-bridge.h"
#include "tlm-bridges/xgmii2tlm-bridge.h"

class tlm_xgmii_phy
: public sc_core::sc_module
{
public:
	xgmii2tlm_bridge tx;
	tlm2xgmii_bridge rx;

	tlm_xgmii_phy(sc_core::sc_module_name name);

private:
};

tlm_xgmii_phy::tlm_xgmii_phy(sc_module_name name)
	: sc_module(name),
	  tx("tx"),
	  rx("rx")
{
}
#endif
