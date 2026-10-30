

#if defined(CONFIG_MACH_MT6768) || defined(CONFIG_MACH_MT6765)
#include <helio-dvfsrc_v2.h>
#elif defined(CONFIG_MACH_MT6785) || defined(CONFIG_MACH_MT6885)
#include <helio-dvfsrc_v3.h>
#elif defined(CONFIG_MACH_MT6873)
#include <helio-dvfsrc_v3.h>
#elif defined(CONFIG_MACH_MT6853)
#include <helio-dvfsrc_v3.h>
#elif defined(CONFIG_MACH_MT6893)
#include <helio-dvfsrc_v3.h>
#elif defined(CONFIG_MACH_MT6833)
#include <helio-dvfsrc_v3.h>
#else
#include <helio-dvfsrc_v1.h>
#endif
