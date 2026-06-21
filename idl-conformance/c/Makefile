# Build the IDL-conformance C roundtrip example.
#
# Requires the ZeroDDS C-API shared library to be built first:
#   cargo build -p zerodds-c-api --release   (run from the repo root)
#
# Override ZERODDS_ROOT if this example dir is moved out of the repo tree.

ZERODDS_ROOT ?= ../../..
INCDIR       := $(ZERODDS_ROOT)/crates/zerodds-c-api/include
LIBDIR       := $(ZERODDS_ROOT)/target/release
IDLC         := $(ZERODDS_ROOT)/target/debug/zerodds-idlc
FIXTURE      := sensor.idl

CFLAGS  := -std=c11 -Wall -I. -I$(INCDIR)
LDFLAGS := -L$(LIBDIR) -lzerodds

# macOS uses DYLD_LIBRARY_PATH; Linux uses LD_LIBRARY_PATH.
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  LDPATH_VAR := DYLD_LIBRARY_PATH
else
  LDPATH_VAR := LD_LIBRARY_PATH
endif

.PHONY: all gen run clean

all: roundtrip

# Regenerate the typed header from this example's local fixture (committed copy
# already present; this target lets you refresh it).
gen:
	$(IDLC) generate $(FIXTURE) --c -o . </dev/null

roundtrip: roundtrip.c sensor.h
	gcc $(CFLAGS) roundtrip.c $(LDFLAGS) -o roundtrip

run: roundtrip
	$(LDPATH_VAR)=$(LIBDIR) ./roundtrip

clean:
	rm -f roundtrip
