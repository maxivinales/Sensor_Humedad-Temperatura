#ifndef _SHT_40_
#define _SHT_40_
#include <sht4x.h>
#include "config.h"
void sht40_launch();
#ifndef APP_CPU_NUM
#define APP_CPU_NUM PRO_CPU_NUM
#endif
#endif