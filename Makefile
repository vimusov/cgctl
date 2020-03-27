TARGET_MAIN := cgctl
TARGET_APPEND := $(TARGET_MAIN)-append
TARGET_START := $(TARGET_MAIN)-start
TARGET_STOP := $(TARGET_MAIN)-stop

BINDIR ?= /usr/bin
SRCDIR := src

CFLAGS += -O2 -D_GNU_SOURCE -std=c99 -Wall -Wextra -Werror
CFLAGS += -I$(SRCDIR)

COMMON_OBJS := $(SRCDIR)/freezer.o
COMMON_OBJS += $(SRCDIR)/cgroup.o
COMMON_OBJS += $(SRCDIR)/log.o
COMMON_OBJS += $(SRCDIR)/tasks.o
COMMON_OBJS += $(SRCDIR)/utils.o

MAIN_OBJS := $(COMMON_OBJS)
MAIN_OBJS += $(SRCDIR)/main.o

APPEND_OBJS := $(COMMON_OBJS)
APPEND_OBJS += $(SRCDIR)/append.o

START_OBJS := $(COMMON_OBJS)
START_OBJS += $(SRCDIR)/start.o
START_OBJS += $(SRCDIR)/privileges.o

STOP_OBJS := $(COMMON_OBJS)
STOP_OBJS += $(SRCDIR)/stop.o

all: $(TARGET_MAIN) $(TARGET_APPEND) $(TARGET_START) $(TARGET_STOP)

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

$(TARGET_MAIN): $(MAIN_OBJS)
	$(CC) -o $@ $(LDFLAGS) $(MAIN_OBJS)

# WARN: Утилиты cgctl-append и cgctl-start специально линкуем статически!
# Бывают случаи, когда динамические библиотеки, подгружаемые бинарником,
# запускают какие-то процессы. Здесь этого ни в коем случае нельзя
# допускать, поскольку утилита используется в upstart, который ведёт
# трассировку процесса и ориентируется по форкам, какой pid является
# главным процессом (опции expect fork, expect daemon).

$(TARGET_APPEND): $(APPEND_OBJS)
	$(CC) -o $@ -static $(LDFLAGS) $(APPEND_OBJS)

$(TARGET_START): $(START_OBJS)
	$(CC) -o $@ -static $(LDFLAGS) $(START_OBJS)

$(TARGET_STOP): $(STOP_OBJS)
	$(CC) -o $@ $(LDFLAGS) $(STOP_OBJS)

install:
	install -D --mode=0755 $(TARGET_MAIN)   $(DESTDIR)$(BINDIR)/$(TARGET_MAIN)
	install -D --mode=0755 $(TARGET_APPEND) $(DESTDIR)$(BINDIR)/$(TARGET_APPEND)
	install -D --mode=0755 $(TARGET_START)  $(DESTDIR)$(BINDIR)/$(TARGET_START)
	install -D --mode=0755 $(TARGET_STOP)   $(DESTDIR)$(BINDIR)/$(TARGET_STOP)

clean:
	-rm $(TARGET_MAIN) $(TARGET_APPEND) $(TARGET_START) $(TARGET_STOP) $(SRCDIR)/*.[oais] scan.log strace_out

indent:
	clang-format -i $(SRCDIR)/*.c $(SRCDIR)/*.h

check: clean
	pvs-studio-analyzer trace -- make
	pvs-studio-analyzer analyze -o scan.log
	plog-converter -t errorfile scan.log
