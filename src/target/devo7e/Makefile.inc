SCREENSIZE := 128x64x1
FILESYSTEM := devo7e
CRC_OFFSET       := 8192
OPTIMIZE_DFU     := 1
DFU_ARGS         := -c 7 -b 0x08003000

include $(SDIR)/target/common/devo/Makefile.inc

