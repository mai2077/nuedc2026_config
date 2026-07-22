################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
build-59622764: ../empty.syscfg
	@echo 'SysConfig - building file: "$<"'
	"C:/ti/sysconfig_1.26.2/sysconfig_cli.bat" -s "C:/ti/mspm0_sdk_2_10_00_04/.metadata/product.json" --script "D:/MCU_Development/TI/MSPM0/nuedc2026_Pinconfig/empty.syscfg" -o "." --compiler ticlang
	@echo 'Finished building: "$<"'
	@echo ' '

device_linker.cmd: build-59622764 ../empty.syscfg
device.opt: build-59622764
device.cmd.genlibs: build-59622764
ti_msp_dl_config.c: build-59622764
ti_msp_dl_config.h: build-59622764
Event.dot: build-59622764

%.o: ./%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"C:/ti/ccs2100/ccs/tools/compiler/ti-cgt-armllvm_5.1.1.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"D:/MCU_Development/TI/MSPM0/nuedc2026_Pinconfig" -I"D:/MCU_Development/TI/MSPM0/nuedc2026_Pinconfig/Debug" -I"D:/MCU_Development/TI/MSPM0/nuedc2026_Pinconfig/bsp" -I"D:/MCU_Development/TI/MSPM0/nuedc2026_Pinconfig/app" -I"D:/MCU_Development/TI/MSPM0/nuedc2026_Pinconfig/control" -I"C:/ti/mspm0_sdk_2_10_00_04/source/third_party/CMSIS/Core/Include" -I"C:/ti/mspm0_sdk_2_10_00_04/source" -gdwarf-3 -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

startup_mspm0g350x_ticlang.o: C:/ti/mspm0_sdk_2_10_00_04/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"C:/ti/ccs2100/ccs/tools/compiler/ti-cgt-armllvm_5.1.1.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"D:/MCU_Development/TI/MSPM0/nuedc2026_Pinconfig" -I"D:/MCU_Development/TI/MSPM0/nuedc2026_Pinconfig/Debug" -I"D:/MCU_Development/TI/MSPM0/nuedc2026_Pinconfig/bsp" -I"D:/MCU_Development/TI/MSPM0/nuedc2026_Pinconfig/app" -I"D:/MCU_Development/TI/MSPM0/nuedc2026_Pinconfig/control" -I"C:/ti/mspm0_sdk_2_10_00_04/source/third_party/CMSIS/Core/Include" -I"C:/ti/mspm0_sdk_2_10_00_04/source" -gdwarf-3 -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


