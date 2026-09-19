#include "tool_internal.h"

struct g g={0};

/* Main.
 */
 
int main(int argc,char **argv) {

  g.exename=argv[0];
  int argi=1;
  while (argi<argc) {
    const char *arg=argv[argi++];
    if (!memcmp(arg,"-o",2)) g.dstpath=arg+2;
    else if (!g.command) g.command=arg;
    else if (!g.srcpath) g.srcpath=arg;
  }
  if (!g.command) {
    fprintf(stderr,"Usage: %s COMMAND [-oOUTPUT] [INPUT]\n",g.exename);
    return 1;
  }
  
  int err;
  #define _(tag) if (!strcmp(g.command,#tag)) err=tool_main_##tag(); else
  TOOL_FOR_EACH_COMMAND
  #undef _
  { fprintf(stderr,"%s: Unknown command '%s'\n",g.exename,g.command); return 1; }
  if (err<0) return 1;
  
  return 0;
}
