
#ifndef __CLK_FMETER_H
#define __CLK_FMETER_H

#if defined(CONFIG_COMMON_CLK_MT6885)
#include "clk-mt6885-fmeter.h"
#elif defined(CONFIG_COMMON_CLK_MT6873)
#include "clk-mt6873-fmeter.h"
#elif defined(CONFIG_COMMON_CLK_MT6853)
#include "clk-mt6853-fmeter.h"
#elif defined(CONFIG_COMMON_CLK_MT6833)
#include "clk-mt6833-fmeter.h"
#endif

#endif
