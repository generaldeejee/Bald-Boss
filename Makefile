TARGET = baldboss_psp
OBJS = src/main.o
INCDIR =
CFLAGS = -O2 -G0 -Wall -Wextra -Wno-unused-parameter -ffast-math
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
LIBDIR =
LIBS = -lpspgum -lpspgu -lm
LDFLAGS =
BUILD_PRX = 1
PSP_EBOOT_TITLE = Bald Boss 3D - PSP Edition
PSP_EBOOT_ICON = assets/ICON0.PNG
EXTRA_TARGETS = EBOOT.PBP
PSPSDK := $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
