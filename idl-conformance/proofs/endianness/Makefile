# Endianness × XCDR-version wire proof.
#
#   make goldens    -> ZeroDDS Rust reference encodes all 6 samples in all 4
#                      representations into goldens/ (24 files).
#   make roundtrip  -> encode+decode each repr; asserts codec self-consistency.
#   make vendors    -> on $(ORACLE_HOST): generate+build+run the CycloneDDS and
#                      Fast DDS oracles, pull their goldens into
#                      vendor-cyclone/ and vendor-fastcdr/.
#   make compare    -> print the repr × vendor MATCH/DIFF table + diff windows.

ORACLE_HOST ?= root@codepit
ORACLE_DIR  ?= /tmp/oracle-endian

.PHONY: goldens roundtrip vendors compare all clean

goldens:
	cd harness-rust && cargo run --quiet --bin endian_proof ENCODE

roundtrip:
	cd harness-rust && cargo run --quiet --bin endian_proof ROUNDTRIP

compare:
	python3 compare.py

# The vendor leg is captured in vendor-src/ (drivers) + the committed goldens.
# Re-running it needs a host with CycloneDDS, Fast DDS and fastddsgen; see
# README "Reproduce" and vendor-src/VENDOR-VERSIONS.txt.
vendors:
	@echo "Vendor oracles run on $(ORACLE_HOST):$(ORACLE_DIR)."
	@echo "Drivers: vendor-src/fastdds_oracle.cpp, vendor-src/cyclone_*_oracle.c"
	@echo "Committed goldens already present; see README to rebuild."

all: goldens roundtrip compare

clean:
	rm -rf harness-rust/target internal-tmp/*.txt
