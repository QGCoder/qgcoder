#pragma once
/* Stand-in for LinuxCNC's generated config.h, cut down to what the
 * interpreter needs when it is embedded rather than built in place. */
#include <cstring>
#include <strings.h>
#ifndef LINELEN
#define LINELEN 255
#endif
#ifndef EMC2_HOME
#define EMC2_HOME "."
#endif
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "embedded"
#endif
