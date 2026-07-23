################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_bitreversal2.S 

C_SRCS += \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_bitreversal.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_f32.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_q15.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_q31.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_f32.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_init_f32.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_init_q15.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_init_q31.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_q15.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_q31.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_f32.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_init_f32.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_init_q15.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_init_q31.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_q15.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_q31.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix8_f32.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_f32.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_init_f32.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_init_q15.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_init_q31.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_q15.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_q31.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_f32.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_f32.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_init_f32.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_init_f32.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_init_q15.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_init_q31.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_q15.c \
../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_q31.c 

C_DEPS += \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_bitreversal.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_f32.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_q15.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_q31.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_f32.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_init_f32.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_init_q15.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_init_q31.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_q15.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_q31.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_f32.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_init_f32.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_init_q15.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_init_q31.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_q15.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_q31.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix8_f32.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_f32.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_init_f32.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_init_q15.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_init_q31.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_q15.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_q31.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_f32.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_f32.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_init_f32.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_init_f32.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_init_q15.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_init_q31.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_q15.d \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_q31.d 

OBJS += \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_bitreversal.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_bitreversal2.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_f32.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_q15.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_q31.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_f32.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_init_f32.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_init_q15.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_init_q31.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_q15.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_q31.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_f32.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_init_f32.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_init_q15.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_init_q31.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_q15.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_q31.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix8_f32.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_f32.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_init_f32.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_init_q15.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_init_q31.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_q15.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_q31.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_f32.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_f32.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_init_f32.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_init_f32.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_init_q15.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_init_q31.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_q15.o \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_q31.o 

S_UPPER_DEPS += \
./LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_bitreversal2.d 

S_UPPER_DEPS__QUOTED += \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_bitreversal2.d" 

OBJS__QUOTED += \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_bitreversal.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_bitreversal2.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_f32.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_q15.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_q31.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix2_f32.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix2_init_f32.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix2_init_q15.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix2_init_q31.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix2_q15.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix2_q31.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix4_f32.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix4_init_f32.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix4_init_q15.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix4_init_q31.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix4_q15.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix4_q31.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix8_f32.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_dct4_f32.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_dct4_init_f32.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_dct4_init_q15.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_dct4_init_q31.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_dct4_q15.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_dct4_q31.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_rfft_f32.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_rfft_fast_f32.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_rfft_fast_init_f32.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_rfft_init_f32.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_rfft_init_q15.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_rfft_init_q31.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_rfft_q15.o" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_rfft_q31.o" 

C_DEPS__QUOTED += \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_bitreversal.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_f32.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_q15.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_q31.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix2_f32.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix2_init_f32.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix2_init_q15.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix2_init_q31.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix2_q15.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix2_q31.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix4_f32.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix4_init_f32.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix4_init_q15.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix4_init_q31.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix4_q15.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix4_q31.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_cfft_radix8_f32.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_dct4_f32.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_dct4_init_f32.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_dct4_init_q15.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_dct4_init_q31.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_dct4_q15.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_dct4_q31.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_rfft_f32.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_rfft_fast_f32.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_rfft_fast_init_f32.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_rfft_init_f32.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_rfft_init_q15.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_rfft_init_q31.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_rfft_q15.d" \
"LeArm\Drivers\CMSIS\DSP\Source\TransformFunctions\arm_rfft_q31.d" 

C_SRCS__QUOTED += \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_bitreversal.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_f32.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_q15.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_q31.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_f32.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_init_f32.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_init_q15.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_init_q31.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_q15.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix2_q31.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_f32.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_init_f32.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_init_q15.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_init_q31.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_q15.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix4_q31.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix8_f32.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_f32.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_init_f32.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_init_q15.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_init_q31.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_q15.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_dct4_q31.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_f32.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_f32.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_init_f32.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_init_f32.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_init_q15.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_init_q31.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_q15.c" \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_q31.c" 

S_UPPER_SRCS__QUOTED += \
"../LeArm/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_bitreversal2.S" 


