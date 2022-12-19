#-------------------------------------------------------------------------------
#-                                                                            --
#-       This software is confidential and proprietary and may be used        --
#-        only as expressly authorized by a licensing agreement from          --
#-                                                                            --
#-                            Verisilicon.                                    --
#-                                                                            --
#-                   (C) COPYRIGHT 2020 VERISILICON.                          --
#-                            ALL RIGHTS RESERVED                             --
#-                                                                            --
#-                 The entire notice above must be reproduced                 --
#-                  on all copies and should not be removed.                  --
#-                                                                            --
#-------------------------------------------------------------------------------
#-
#--  Abstract : Makefile for encoder testbench
#--
#-------------------------------------------------------------------------------

# default
ENV ?= pci
target ?=

include build/globaldefs
include build/globalrules

ifeq ($(VCLE),y)
ifeq  ($(SYSH264),y)
	SYSHEVC=n
	SYSH264=y
	SYSAV1=n
	SYSVP9=n
else
	SYSHEVC=y
	SYSH264=n
	SYSAV1=n
	SYSVP9=n
endif
else
	SYSHEVC=y
	SYSH264=y
	SYSAV1=y
	SYSVP9=y
endif

#Here are rules for building codes and generating executable
help:
	@echo "---------------------------------------"
	@echo ""
	@echo "Hantro VC8000 Encoder Software Makefile"
	@echo ""
	@echo " Usage:"
	@echo "  make [target] ENV=[environment] target=[configure] [variables]"
	@echo ""
	@echo "    NOTE! Make sure to do 'make libclean' between compiling to different targets!"
	@echo ""
	@echo " Available targets:"
	@echo "  video           - VC8000E encoder binary for full video formats (not include JPEG)"
	@echo "  hevc            - VC8000E encoder binary for HEVC"
	@echo "  h264            - VC8000E encoder binary for H264"
	@echo "  av1             - VC8000E encoder binary for AV1"
	@echo "  vp9             - VC8000E encoder binary for VP9"
	@echo "  jpeg            - VC8000E encoder binary for JPEG"
#	@echo "  camstab         - VC8000E Standalone Stabilization (c-model only)"
	@echo "  libenc          - VC8000E encoder libh2enc.a"
	@echo "  clean           - Clean libh2enc.a and all excutable"
	@echo "  env             - Print environment variables"
	@echo "  help            - This help text"
	@echo " (Following targets are for C-model only)"
	@echo "  libsys          - VC8000E c-model lib, which required before target with ENV=system"
	@echo "  libva           - VC8000E libs for vaapi, which not included EWL"
	@echo "  libclean        - Clean C-model lib"
	@echo "  cfgclean        - Clean C-model lib configure only, use to change target"
	@echo " for projects defined with build ID, get projects configure by following commands"
	@echo "  hwcfg_ls        - list all projects"
	@echo "  hwcfg_mk feature=<Vendor:Project> - Generate enc_cfg.h for target Vender_Project"
	@echo "  hwcfg_rm target=<Vender_Project>  - Remove enc_cfg.h in c-model for target Name"
	@echo " for projects defined with fuse registers, get projects information by following command"
	@echo "  cfglist         - List all configures supported"
	@echo "  cfgsys_mk target=<ProjectName>    - Generate enc_cfg.h for target ProjectName"
	@echo "  cfgsys_rm target=<Name>      - Remove enc_cfg.h in c-model for target Name"
	@echo ""
	@echo " Available environments:"
	@echo "  system (default)    - system model in x86 linux. must specify target=ProjectName"
#	@echo "  eval                - system model for evaluation with frame limit"
#	@echo "  arm_linux           - hardware in arm aarch64 linux"
#	@echo "  arm_pclinux         - system model in arm aarch64 linux"
#	@echo "  x86_pclinux         - system model in x86 linux (same as system)"
	@echo "  freertos            - no sepcial cpu, just build with gcc in Linux"
	@echo "  pci                 - vsi fpga verification board in x86 linux"
	@echo ""
	@echo " Examples of C-Model targets:"
	@echo "  default        - VC8000E6.2 base configure"
	@echo "  DEV            - VC8000E developing configure with latest features"
	@echo "  V60_DEV        - VC8000LE developing configure with latest features"
	@echo " Configuration variables to compile C-Model"
	@echo "  VCLE=$(strip $(VCLE)) [y/n]   -- select VCLE or VCE lib of system model. When VCLE=y, need to specify"
	@echo "                   which format is supported (default is HEVC) because hevc and"
	@echo "                   h264 cannot be supported in one system librry. use SYSH264=y to"
	@echo "                   compile h264 lib; use SYSHEVC=y for hevc lib (default)."
	@echo "                   When target has prefix \"V60_\", it will enable \"VCLE=y\"."
	@echo ""
	@echo " General Configuration variables"
	@echo "  TRACE         = $(strip $(TRACE)) -- enables trace (for system model only) [n/y]"
	@echo "  DEBUG         = $(strip $(DEBUG)) -- enables debug symobles [n/y]"
	@echo "  LOGMSG        = $(strip $(LOGMSG)) -- enables software logs [y/n]"
	@echo "    Beside control log message in command line of test bench, it can be selected by following ENV"
	@echo "    * VCNEC_LOG_OUTPUT: output target (0:stdout | 1: two files TRACE/CHECK | 2: files per thread)"
	@echo "    * VCENC_LOG_LEVEL: output level   (0:stdout | 1:FATAL | 2:ERROR | 3:WARN | 4:INFO | 5:DEBUG | 6:ALL)"
	@echo "    * VCENC_LOG_TRACE: bitmap bit 0~6 to control which module will be logged:"
	@echo "           BIT  Range     Notes"
	@echo "             0: API      encoder API trace"
	@echo "             1: REGS     registers dump"
	@echo "             2: EWL  	wrapper layer API trace"
	@echo "             3: RC       Rate Control trace"
	@echo "             4: MEM      Allocated EWL memory summary"
	@echo "             5: CML      (TODO) Command line detail"
	@echo "             6: PERF     (TODO) Performance Profiling"
	@echo "       e.g. enable API and EWL by 'export VCENC_LOG_TRACE=5 #b101'"
#	@echo "    * VCENC_LOG_CHECK: enable CHECK, (TODO) bitmap bit0~4: RECON|QUALITY|VBV|RC|FEATURE"
	@echo ""
	@echo " To override environment variables give them on command line, e.g."
	@echo ""
	@echo "  make hevc ENV=system DEBUG=y"
	@echo ""

BINHOME=bin/$(ENV)

$(BINHOME):
	@echo "creating... $(BINHOME)"
	@test -d $@ || mkdir -p $@

.PHONY: video hevc h264 av1 vp9 jpeg

video: $(BINHOME) libenc
	make $(ENV) -C linux_reference/test/video
	mv linux_reference/test/video/video_testenc $(BINHOME)/video_testenc$(target)

hevc: $(BINHOME) libenc
	make $(ENV) -C vcenc_test
	mv vcenc_test/vc_test_enc $(BINHOME)/vc_test_enc$(target)

h264: $(BINHOME) libenc
	make $(ENV) -C linux_reference/test/h264
	mv linux_reference/test/h264/h264_testenc $(BINHOME)/h264_testenc$(target)

av1: $(BINHOME) libenc
	make $(ENV) -C linux_reference/test/av1
	mv linux_reference/test/av1/av1_testenc $(BINHOME)/av1_testenc$(target)

vp9: $(BINHOME) libenc
	make $(ENV) -C linux_reference/test/vp9
	mv linux_reference/test/vp9/vp9_testenc $(BINHOME)/vp9_testenc$(target)

jpeg: $(BINHOME) libenc
	make $(ENV) -C linux_reference/test/jpeg
	mv linux_reference/test/jpeg/jpeg_testenc $(BINHOME)/jpeg_testenc$(target)

camstab: $(BINHOME)
	make $(ENV) -C linux_reference INCLUDE_VS=y
	make $(ENV) -C linux_reference/test/camstab INCLUDE_VS=y
	mv linux_reference/test/camstab/videostabtest $(BINHOME)/videostabtest$(target)

libsys: libenc
	make system -C ../$(SYSTEM)/models HEVC=$(SYSHEVC) H264=$(SYSH264) AV1=$(SYSAV1) VP9=$(SYSVP9)

libenc:
	make $(ENV) -C linux_reference

libva:
	make $(ENV) -C linux_reference USE_SAFESTRING=n LIBVA=y
	make vah26x -C ../system LIBVA=y

shlib:
	make videolib -C ../system SHARELIB=y
	cp ../$(ENV)/models/ench2_asic_model.so lib/ench2_asic_model_h26x_${target}.so

libclean:
	make clean -C ../$(SYSTEM)/models

cfgclean:
	rm ../$(SYSTEM)/models/enc_core.o ../$(SYSTEM)/models/ench2_asic_model.a

cfglist:
	make hwcfglist -C ../$(SYSTEM)/

cfgsys_mk:
	test -f ../$(SYSTEM)/models/ewl/$(target)/enc_cfg.h || make hwcfg -C ../$(SYSTEM)/ target=$(target)

cfgsys_rm:
	test ! -f ../$(SYSTEM)/models/ewl/$(target)/enc_cfg.h || rm -rf ../$(SYSTEM)/models/ewl/$(target)

hwcfg_rm: cfgsys_rm

hwcfg_ls:
	make hwcfg_ls -C ../$(SYSTEM)/

hwcfg_mk:
	test -f ../$(SYSTEM)/models/ewl/${feature}/enc_cfg.h || make hwcfg_mk -C ../$(SYSTEM)/ feature=${feature}

clean:
	rm $(BINHOME)/* -rf
	make clean -C linux_reference
	test ! -f linux_reference/test/video/Makefile || make clean -C linux_reference/test/video
	test ! -f vcenc_test/Makefile || make clean -C vcenc_test
	test ! -f linux_reference/test/h264/Makefile || make clean -C linux_reference/test/h264
	test ! -f linux_reference/test/av1/Makefile || make clean -C linux_reference/test/av1
	test ! -f linux_reference/test/vp9/Makefile || make clean -C linux_reference/test/vp9
	test ! -f linux_reference/test/jpeg/Makefile || make clean -C linux_reference/test/jpeg

all: video jpeg

