-include $(TOPDIR)/rules.mk
include ../shared/BuildConfigCommon.mk
include ../shared/BuildInfoLinux.mk

.DEFAULT_GOAL = all
PROG = shell_11ad

INCLUDE_CFLAGS += \
	-I .\
	-I ../External/JsonCpp/1.8.1/dist \
	-I ../shared \

all: $(PROG)

C_FILES = $(shell find . ../External/JsonCpp -type f -name '*.c')
CPP_FILES = $(shell find . ../External/JsonCpp -type f -name '*.cpp')
OBJ_FILES =  $(CPP_FILES:.cpp=.o)
OBJ_FILES += $(C_FILES:.c=.o)

$(PROG): $(OBJ_FILES)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o : %.cpp
	$(CXX) $(CPPFLAGS) $(INCLUDE_CFLAGS) -o $@ -c $<

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDE_CFLAGS) -o $@ -c $<

clean:
	rm -rf $(PROG)
	find . ../External/JsonCpp -type f \( -name "*.d" -o -name "*.o" -o -name "*~" \) -delete

-include $(OBJ_FILES:%.o=%.d)
