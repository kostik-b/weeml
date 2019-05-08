# Copyright QUB 2019

.PHONY : clean uninstall

SOURCE_DIR    := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

all : libweeml.a peer_test max_size_test mt_test

C_SRCS = ikcp.c recv_reply.c snd_recv.c max_size_snd.c max_size_reply.c mt_reply.c mt_snd.c
CPP_SRCS = weeml.cc poller.cc  the_buffer.cc  wml.cc

SRC_DIRS = src test kcp_src

CC_OPTS = -g  -I$(SOURCE_DIR)/src -I$(SOURCE_DIR)/kcp_src #-DWEEML_DBG=1 -fPIC

CXX_OPTS := $(CC_OPTS) -std=c++11


# ----------------- macro for dependencies -------------
define depend_macro

%.o.d: $(1)
	@set -e; rm -f $$@; \
	$$(CC) -MM $$(CFLAGS) $$(CC_OPTS) $$< > $$@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $$@ : ,g' < $$@.$$$$ > $$@; \
	rm -f $$@.$$$$

endef

define depend_macro_cpp

%.o.d: $(1)
	@set -e; rm -f $$@; \
	$$(CXX) -MM $$(CXXFLAGS) $$(CXX_OPTS) $$< > $$@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $$@ : ,g' < $$@.$$$$ > $$@; \
	rm -f $$@.$$$$

endef

$(foreach DIR, $(SRC_DIRS), $(eval $(call depend_macro,       $(SOURCE_DIR)/$(DIR)/%.c)))
$(foreach DIR, $(SRC_DIRS), $(eval $(call depend_macro_cpp,   $(SOURCE_DIR)/$(DIR)/%.cc)))
# ------- end dependencies ------------------------------

include $(C_SRCS:%.c=%.o.d)
include $(CPP_SRCS:%.cc=%.o.d)

# ---------------- macro for the build pattern rules ----------
define build_macro

%.o : $(1)
	$${CC} $$< -o $$@ -c $${CFLAGS} $${CC_OPTS}

endef

define build_macro_cpp

%.o : $(1)
	$${CXX} $$< -o $$@ -c $${CXXFLAGS} $${CXX_OPTS} -D__FILE_NAME_ONLY__="\"$$(@:%.o=%) \""

endef

$(foreach DIR, $(SRC_DIRS), $(eval $(call build_macro, $(SOURCE_DIR)/$(DIR)/%.c)))
$(foreach DIR, $(SRC_DIRS), $(eval $(call build_macro_cpp, $(SOURCE_DIR)/$(DIR)/%.cc)))
#---------------- end build macro -----------------------------

peer_test : snd_recv.o recv_reply.o  libweeml.a
	$(CC) snd_recv.o  -o snd_recv -L$(SOURCE_DIR)/libev-4.25/install/lib libweeml.a -lpthread -lstdc++
	$(CC) recv_reply.o -o recv_reply  -L$(SOURCE_DIR)/libev-4.25/install/lib libweeml.a -lpthread -lstdc++

max_size_test : max_size_snd.o max_size_reply.o libweeml.a
	$(CC)  max_size_snd.o   -o max_size_snd    -L$(SOURCE_DIR)/libev-4.25/install/lib libweeml.a -lpthread -lstdc++
	$(CC)  max_size_reply.o -o max_size_reply  -L$(SOURCE_DIR)/libev-4.25/install/lib libweeml.a -lpthread -lstdc++

mt_test : mt_snd.o mt_reply.o  libweeml.a
	$(CC) mt_snd.o  -o mt_snd -L$(SOURCE_DIR)/libev-4.25/install/lib libweeml.a -lpthread -lstdc++
	$(CC) mt_reply.o -o mt_reply  -L$(SOURCE_DIR)/libev-4.25/install/lib libweeml.a -lpthread -lstdc++

libweeml.a : weeml.o ikcp.o wml.o the_buffer.o poller.o
	ar rcfv $@ $^


