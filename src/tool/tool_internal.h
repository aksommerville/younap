#ifndef TOOL_INTERNAL_H
#define TOOL_INTERNAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include "opt/midi/midi.h"
#include "opt/fs/fs.h"
#include "opt/serial/serial.h"

extern struct g {
  const char *exename;
  const char *command;
  const char *srcpath;
  const char *dstpath;
} g;

#define TOOL_FOR_EACH_COMMAND \
  _(midmerge)
  
#define _(tag) int tool_main_##tag();
TOOL_FOR_EACH_COMMAND
#undef _

#endif
