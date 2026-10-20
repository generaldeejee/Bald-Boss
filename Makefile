TARGET = elite_bald_man_club_psp
OBJS = src/main.o
INCDIR =
CFLAGS = -O2 -G0 -Wall -Wextra -Wno-unused-parameter -ffast-math
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
LIBDIR =
LIBS = -lpspgum -lpspgu -lpspaudio -lm
LDFLAGS =
BUILD_PRX = 1
PSP_EBOOT_TITLE = The Elite Bald Man Club
PSP_EBOOT_ICON = assets/ICON0.PNG
EXTRA_TARGETS = EBOOT.PBP
PSPSDK := $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
