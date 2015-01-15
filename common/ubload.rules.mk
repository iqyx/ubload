# generic ubload make rules

V=1

CFLAGS          += -I . -I ../../common -I ../../lineedit
OBJS            += ../../common/config.o ../../common/led_basic.o
OBJS            += ../../common/fw_runner.o ../../common/cli.o
OBJS            += ../../common/timer.o
OBJS            += ../../lineedit/lineedit.o

OPENCM3_DIR      = ../../libopencm3

OOCD		?= openocd
OOCD_INTERFACE	?= stlink-v2

include ../../common/libopencm3.rules.mk
