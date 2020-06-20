USE_HANDLERSOCKET = no

DEPENDENCIES = sqlite3 libconfig libcurl dbus-1

FLAGS = -g -rdynamic -O3 -MMD $(shell pkg-config --cflags $(DEPENDENCIES)) -I$(shell pg_config --includedir)
LIBS = -lstdc++ -lpthread -lbsd -lcurses -lircclient -lpq -lmysqlclient -lmemcached -lmosquitto \$(shell pkg-config --libs $(DEPENDENCIES))

OBJECTS = \
    Dump.o \
    StringTools.o NetworkTools.o DStarTools.o \
    G2Link.o DExtraLink.o DCSLink.o CCSLink.o \
    GateContext.o GateSession.o GateRouter.o GateManager.o \
    SQLiteStore.o MySQLStore.o StoreFactory.o DDBClient.o DDBModule.o \
    DataCache.o MonitorClient.o RP2CEmulator.o RP2CDirectory.o RP2CModule.o \
    LiveLogClient.o CCSModule.o \
    MasterModule.o \
    ZoneModule.o \
    DBusCore.o GateService.o \
    ReportTools.o BorderGate.o

PREFIX = /opt/BorderGate

ifeq ( , $(wildcard /usr/include/bsd/libutil.h))
	FLAGS += -DUSE_LIBBSD_02
endif

ifeq ($(USE_HANDLERSOCKET), yes)
	FLAGS += -DUSE_HANDLERSOCKET
	OBJECTS += HSStore.o
	LIBS += -lhsclient
endif

CC = gcc
CXX = g++

CFLAGS += $(FLAGS)
CXXFLAGS += $(FLAGS)

all: build

build: $(OBJECTS)
	$(CC) $(OBJECTS) $(FLAGS) $(LIBS) -o bordergate

clean:
	rm -f $(OBJECTS) bordergate
	rm *.d

config:
	install -m 640 -o root -g bordergate BorderGate.conf $(PREFIX)
	install -m 640 -o root -g bordergate DStarMonitor.cnf $(PREFIX)
	install -m 640 -o root -g bordergate DDB-Local.cnf $(PREFIX)
	install -m 640 -o root -g bordergate DDB-Global.cnf $(PREFIX)

install:
	install -o root -g root bordergate $(PREFIX)
	install -o root -g root initiate.sh $(PREFIX)
	install -o root -g root reconnect.sh $(PREFIX)
	install -o root -g root restart.sh $(PREFIX)
	install -o root -g root visible.sh $(PREFIX)

install-system:
	install -o root -g root bordergate-daemon $(PREFIX)
	install -m 644 -o root -g root bordergate-monit $(PREFIX)
	install -m 644 -o root -g root bordergate-dbus.conf $(PREFIX)
	ln -s $(PREFIX)/bordergate-daemon /etc/init.d/bordergate
	ln -s $(PREFIX)/bordergate-monit /etc/monit/monitrc.d/bordergate
	ln -s $(PREFIX)/bordergate-dbus.conf /etc/dbus-1/system.d/bordergate.conf
	update-rc.d bordergate defaults
	service dbus restart

install-data:
	install -m 660 -o bordergate -g bordergate Data/*.dat $(PREFIX)/Data

install-all: config install install-system install-data

.PHONY: all build clean install install-system install-data install-all config
