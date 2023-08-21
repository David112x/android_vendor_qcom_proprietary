-include $(TOPDIR)/rules.mk
include ../shared/BuildConfigCommon.mk
include ../shared/BuildInfoLinux.mk

LDFLAGS += $(PTHREAD_LINK_FLAGS) $(LIBNL_LINK_FLAGS)
LIBS := -lnl-3 -lnl-genl-3

.DEFAULT_GOAL = all
PROG = host_manager_11ad

INCLUDE_CFLAGS +=                       \
    $(LIBNL_INCLUDE_FLAGS)              \
    -I .                                \
    -I access_layer_11ad                \
    -I access_layer_11ad/Unix           \
    -I ../External/JsonCpp/1.8.1/dist   \
    -I ../External/Asio/1.10.6/include  \
    -I ../External/SimpleWebServer/3.0.0\
    -I ../shared                        \

# SLPI Sensing test
#CPPFLAGS += -D_WIGIG_SLPI_SENSING_

all: $(PROG)

C_FILES = $(shell find . ../External -type f -name '*.c')
CPP_FILES = $(shell find . ../External -type f -name '*.cpp')
OBJ_FILES =  $(CPP_FILES:.cpp=.o)
OBJ_FILES += $(C_FILES:.c=.o)

# Suppress warning pragma in dependency files
NOISY_OBJ_FILES = WebHttpServer.o LogCollector/LogTxtParser.o
$(NOISY_OBJ_FILES): EXTRA_CPPFLAGS := -Wno-cpp -Wno-implicit-fallthrough


$(PROG): $(OBJ_FILES)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o : %.cpp
	$(CXX) $(CPPFLAGS) $(EXTRA_CPPFLAGS) $(INCLUDE_CFLAGS) -o $@ -c $<

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDE_CFLAGS) -o $@ -c $<

clean:
	rm -rf $(PROG)
	find . ../External -type f \( -name "*.d" -o -name "*.o" -o -name "*~" \) -delete

-include $(OBJ_FILES:%.o=%.d)
