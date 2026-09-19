all:
.SILENT:

ifeq (,$(EGG_SDK))
  EGG_SDK:=../egg2
endif
EGGDEV:=$(EGG_SDK)/out/eggdev

# If EGG_TARGETS was provided via environment, capture it.
# We're including config.mk for the tool's sake and that will clobber it.
PRE_EGG_TARGETS:=$(EGG_TARGETS)

# Build a single tool, for any extra build-time processing we need.
include $(EGG_SDK)/local/config.mk
PRECMD=echo "  $@" ; mkdir -p $(@D) ;
TOOLS_CFILES:=$(shell find src/tool $(EGG_SDK)/src/opt/midi $(EGG_SDK)/src/opt/fs $(EGG_SDK)/src/opt/serial -name '*.c')
TOOLS_OFILES:=$(patsubst $(EGG_SDK)/src/%.c,mid/tool/%.o,$(patsubst src/tool/%.c,mid/tool/%.o,$(TOOLS_CFILES)))
-include $(TOOLS_OFILES:.o=.d)
mid/tool/%.o:src/tool/%.c;$(PRECMD) $(eggdev_CC) -o$@ $< -I$(EGG_SDK)/src/
mid/tool/%.o:$(EGG_SDK)/src/%.c;$(PRECMD) $(eggdev_CC) -o$@ $< -I$(EGG_SDK)/src/
TOOL_EXE:=out/tool
$(TOOL_EXE):$(TOOLS_OFILES);$(PRECMD) $(eggdev_LD) -o$@ $^ $(EGG_SDK)/out/$(EGG_NATIVE_TARGET)/libeggrt-headless.a $(eggdev_LDPOST)
all:$(TOOL_EXE)

all:;$(EGGDEV) build
clean:;rm -rf mid out
run:;$(EGGDEV) run
web-run:all;$(EGGDEV) serve --htdocs=out/younap-web.zip --project=.
edit:;$(EGGDEV) serve \
  --htdocs=/data:src/data \
  --htdocs=EGG_SDK/src/web \
  --htdocs=EGG_SDK/src/editor \
  --htdocs=src/editor \
  --htdocs=/synth.wasm:EGG_SDK/out/web/synth.wasm \n  --htdocs=/build:out/younap-web.zip \
  --htdocs=/out:out \
  --writeable=src/data \
  --project=.
