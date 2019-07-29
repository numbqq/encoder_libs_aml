.PHONY: ALL BIN_LIB clean BINLIBCOPY

BIN = bin
LIB = lib

RM = rm -rf


ifneq ($(findstring $(AML)$(CNM),1),)
export CROSS_CC_PREFIX=aarch64-linux-gnu-
export CC=$(CROSS_CC_PREFIX)gcc
export CXX=$(CROSS_CC_PREFIX)g++
export AR=$(CROSS_CC_PREFIX)ar
endif


ifeq ($(AML), 1)

SUBDIRS = vpuapi \
		  amvenc_lib \
		  amvenc_test \


ALL: $(SUBDIRS) BINLIBCOPY


$(SUBDIRS): BIN_LIB
	make -C $@

BIN_LIB:
	mkdir -p $(BIN)
	mkdir -p $(LIB)

BINLIBCOPY:
	-cp ./vpuapi/libamvenc_api.so ./lib/
	-cp ./amvenc_lib/libvpcodec.so ./lib/
	-cp ./amvenc_test/aml_enc_test ./bin/

clean:
	$(RM) $(BIN) $(LIB)
	$(foreach i, $(SUBDIRS), make clean -C $i;)
	@echo clean all...

endif






ifeq ($(CNM), 1)


SUBDIRS = vpuapi \
		  test_sample \


ALL: $(SUBDIRS) BINLIBCOPY


$(SUBDIRS): BIN_LIB
	make -C $@

BIN_LIB:
	mkdir -p $(BIN)
	mkdir -p $(LIB)

BINLIBCOPY:
	-cp ./vpuapi/libamvenc_api.so ./lib/
	-cp ./test_sample/vp5_enc_test ./bin/


clean:
	$(RM) $(BIN) $(LIB)
	$(foreach i, $(SUBDIRS), make clean -C $i;)
	@echo clean all...

endif
