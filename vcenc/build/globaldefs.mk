#                   (C) COPYRIGHT 2014 VERISILICON
#                            ALL RIGHTS RESERVED
#
#File: globaldefs
#

#########################################################
#  Section: OS environment
#########################################################
# 1. Build SW in 32bit instruction on 64bit OS
#    Default:  64bit OS built 64bit instructions,
#               32bit OS built 32bit instructions
#    USE_32BIT=y to build 32bit instruction on 64bit OS
USE_32BIT=n

# 2. EWL to use POLLING_ISR or IRQ to communicate with driver
#    POLLING_ISR = y: don't use IRQs
ifeq (pci,$(findstring pci, $(MAKECMDGOALS)))
POLLING_ISR = y
else
POLLING_ISR = n
endif
POLLING_ISR = y
# 3. Assemble C Code
# ASM = y: Enable Intel SSE/AVX2
ASM = n

# 4. Profiling
# PROFILE = y: Enable -pg for profiling function timecost
PROFILE = n

#########################################################
#  Section: Debug/Verification purpose environment section
#               This section should be disabled in production
#########################################################
# 1. Comment/uncomment the following line to disable/enable debugging
DEBUG = n

# 2. Enable C-Model to have tracefile generated
#     Used for HW verification purpose
TRACE = y

# 3. Include API extension and test ID for internal testing
#     SW has capability for specific test case ID
INTERNAL_TEST = y

# 4. To add coverage check of code usage and data range
DAT_COVERAGE = n
USE_COVERAGE = n

# 5. Control API trace log printed to STDOUT(screen) or log file(api.trc)
# default to STDOUT
APITRC_STDOUT = y

# 6. Log Message
# default is enable
LOGMSG = y

#########################################################
#  Section: SW/HW functions section
#           Features control correspond with HW IP
#########################################################

# define the feature set, feature set will determine which build ID feature file
# will be used when compile ewl_common.c
FEATURES=

# 1. Comment/uncomment the following lines to define which control codes to
#    To include in the library build
INCLUDE_HEVC = y
ifeq ($(AV1),y)
  INCLUDE_AV1 = y
endif
ifeq ($(VP9),y)
  INCLUDE_VP9 = y
endif
INCLUDE_JPEG = n
INCLUDE_VS = n

# 2. Input line buffer low latency
#    Set PCIE FPGA Verification env
#    Test hardware handshake mode of input line buffer
PCIE_FPGA_VERI_LINEBUF=n

# 3. Configure encoder SW/testbench support cache&shaper or not
#    System should have L2-Cache IP integrated
SUPPORT_CACHE = n
SUPPORT_INPUT_CACHE = n
SUPPORT_REF_CACHE = n

# 4. Referene frame read/write 1KB burst size
RECON_REF_1KB_BURST_RW = n

# 5. Configure encoder SW/testbench support DEC400 or not
#    System should have DEC400 IP integrated
SUPPORT_DEC400 = n

# 6. Configure encoder SW/testbench support AXIFE or not
#    System should have AXIFE IP integrated
SUPPORT_AXIFE = n

# 6.1 Configure encoder SW/testbench to use cmodel AXIFE or not
#     When CMODEL_AXIFE = y, indicates use cmodel test, and axife enable defined by command line.
#     When CMODEL_AXIFE = n, indicates hardware axife, whose enable is forcibly set to 1 in testbench.
COMODEL_AXIFE = n

# 7. Configure encoder SW/testbench support APBFILTER or not
#    System should have APBFILTER IP integrated
SUPPORT_APBFT = n

# 8. Configure encoder SW/testbench support MEM_SYNC or not
SUPPORT_MEM_SYNC = n

# 9. Configure encoder SW/testbench support MEM_STATISTIC or not
SUPPORT_MEM_STATISTIC = n

# 10. Configure encoder SW support virtual platform test ot not
VIRTUAL_PLATFORM_TEST = n

# add a option to switch between safe string lib and libc string functions, if need, set "y", the default is "n"
USE_SAFESTRING = n

#########################################################
#   Section: customized features section
#            one customer correspond feature to control
#            group of features
#########################################################
# 1. FB customized section
FB_FEATURE_ENABLE = n
ifeq ($(FB_FEATURE_ENABLE),y)
  RECON_REF_1KB_BURST_RW = y
  SUPPORT_CACHE = n
endif

# 2. TBH customized section
TBH_FEATURE_ENABLE = n
ifeq ($(TBH_FEATURE_ENABLE),y)
  RECON_REF_ALIGN64 = y
endif

# use VCLE C-model or not
VCLE = n
SYSTEM = system
# if name of the target hase "V60_" prefix, it is VCLE core.
ifeq ($(findstring V60_, $(target)), V60_)
  VCLE=y
endif
# use system60 as c-model when VCLE is clarified
ifeq ($(VCLE),y)
  SYSTEM = system60
endif

#########################################################
#   Section: Features build support
#            This section could be disabled for code reduce
#            set y ,feature files in SW be complied
#            set n ,feature files in SW not be complied
#########################################################

#1. axife
# use SUPPORT_AXIFE defined above

#2.checksum & crc
CHECKSUM_CRC_BUILD_SUPPORT =y

#3.cutree
CUTREE_BUILD_SUPPORT = y

#4.vcmd
VCMD_BUILD_SUPPORT = y

#5.log
# use LOGMSG defined above

#6.dec400
# use SUPPORT_DEC400 defined above

#7.low latency
LOW_LATENCY_BUILD_SUPPORT = y

#8.error log
ERROR_BUILD_SUPPORT = y

#9.linux lock
LINUX_LOCK_BUILD_SUPPORT = y

#10.memory synchronization
# use SUPPORT_MEM_SYNC defined above

#11.fifo
FIFO_BUILD_SUPPORT = y

#12.cache
# use SUPPORT_CACHE defined above

#13.sei
SEI_BUILD_SUPPORT = y

#14.rate control
RATE_CONTROL_BUILD_SUPPORT = y

#15.test id
# use INTERNAL_TEST defined above

#16.cuinfo
CUINFO_BUILD_SUPPORT = y

#17.roi
ROI_BUILD_SUPPORT = y

#18.stream ctrl
STREAM_CTRL_BUILD_SUPPORT = y


