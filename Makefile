PULP_APP = main
PULP_APP_FC_SRCS =  src/main.c src/decoder.c src/detect_loop.c src/service.c src/demod/cicFilterCplxStep.c src/demod/pttA2Demod.c src/demod/sampler.c 
PULP_APP_HOST_SRCS = src/main.c src/decoder.c src/detect_loop.c src/service.c src/demod/cicFilterCplxStep.c src/demod/pttA2Demod.c src/demod/sampler.c 
PULP_CFLAGS = -O2 -g -I$(GAP_SDK_HOME)/rtos/pulp/pulp-os/kernel/gap/
LIBS += -lm

include $(GAP_SDK_HOME)/tools/rules/pulp_rules.mk
