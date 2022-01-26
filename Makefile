# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
MAGMA_KGSL = magma_kgsl_test
SOURCES += magma_kgsl_test.c
OBJS = $(SOURCES:.c=.o)
DEPS = $(SOURCES:.c=.d)
PKG_CONFIG ?= pkg-config
CFLAGS += -O0 -ggdb3
LDLIBS += $(PC_LIBS)
.PHONY: all clean
all: $(MAGMA_KGSL)
$(MAGMA_KGSL): $(OBJS)
clean:
	$(RM) $(MAGMA_KGSL)
	$(RM) $(OBJS) $(DEPS)
	$(RM) *.o *.d .version
$(MAGMA_KGSL):
	$(CC) $(CCFLAGS) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)
$(OBJS): %.o: %.c
	$(CC) $(CCFLAGS) $(CFLAGS) -c $< -o $@ -MMD
-include $(DEPS)
