# generic ubload make rules

V=1

CFLAGS          += -I . -I ../../common -I ../../lineedit
OBJS            += ../../common/config.o ../../common/led_basic.o
OBJS            += ../../common/fw_runner.o ../../common/cli.o
OBJS            += ../../common/timer.o ../../common/fw_flash.o
OBJS            += ../../lineedit/lineedit.o ../../common/xmodem.o
OBJS            += ../../common/system_log.o ../../common/u_log.o

OPENCM3_DIR      = ../../libopencm3

OOCD		?= openocd
OOCD_INTERFACE	?= stlink-v2

include ../../common/libopencm3.rules.mk
