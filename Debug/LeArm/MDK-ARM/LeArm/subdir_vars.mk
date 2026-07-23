################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../LeArm/MDK-ARM/LeArm/adc.o \
../LeArm/MDK-ARM/LeArm/adc_sample.o \
../LeArm/MDK-ARM/LeArm/app_porting.o \
../LeArm/MDK-ARM/LeArm/avoid_obstacles.o \
../LeArm/MDK-ARM/LeArm/buzzer.o \
../LeArm/MDK-ARM/LeArm/differential_chassis.o \
../LeArm/MDK-ARM/LeArm/dma.o \
../LeArm/MDK-ARM/LeArm/eventrecorder.o \
../LeArm/MDK-ARM/LeArm/gpio.o \
../LeArm/MDK-ARM/LeArm/i2c.o \
../LeArm/MDK-ARM/LeArm/led.o \
../LeArm/MDK-ARM/LeArm/led_porting.o \
../LeArm/MDK-ARM/LeArm/lwrb.o \
../LeArm/MDK-ARM/LeArm/main.o \
../LeArm/MDK-ARM/LeArm/mecanum_chassis.o \
../LeArm/MDK-ARM/LeArm/motor_module.o \
../LeArm/MDK-ARM/LeArm/mui.o \
../LeArm/MDK-ARM/LeArm/mui_u8g2.o \
../LeArm/MDK-ARM/LeArm/oled.o \
../LeArm/MDK-ARM/LeArm/ps2_porting.o \
../LeArm/MDK-ARM/LeArm/pwm_servo_porting.o \
../LeArm/MDK-ARM/LeArm/pwm_servos.o \
../LeArm/MDK-ARM/LeArm/qmi8658.o \
../LeArm/MDK-ARM/LeArm/retarget_io.o \
../LeArm/MDK-ARM/LeArm/robot_arm.o \
../LeArm/MDK-ARM/LeArm/serial_servo.o \
../LeArm/MDK-ARM/LeArm/spi.o \
../LeArm/MDK-ARM/LeArm/startup_stm32f103xb.o \
../LeArm/MDK-ARM/LeArm/stepper_strip.o \
../LeArm/MDK-ARM/LeArm/stm32f1xx_hal.o \
../LeArm/MDK-ARM/LeArm/stm32f1xx_hal_adc.o \
../LeArm/MDK-ARM/LeArm/stm32f1xx_hal_adc_ex.o \
../LeArm/MDK-ARM/LeArm/stm32f1xx_hal_cortex.o \
../LeArm/MDK-ARM/LeArm/stm32f1xx_hal_dma.o \
../LeArm/MDK-ARM/LeArm/stm32f1xx_hal_exti.o \
../LeArm/MDK-ARM/LeArm/stm32f1xx_hal_flash.o \
../LeArm/MDK-ARM/LeArm/stm32f1xx_hal_flash_ex.o \
../LeArm/MDK-ARM/LeArm/stm32f1xx_hal_gpio.o \
../LeArm/MDK-ARM/LeArm/stm32f1xx_hal_gpio_ex.o \
../LeArm/MDK-ARM/LeArm/stm32f1xx_hal_i2c.o \
../LeArm/MDK-ARM/LeArm/stm32f1xx_hal_msp.o \
../LeArm/MDK-ARM/LeArm/stm32f1xx_hal_pwr.o \
../LeArm/MDK-ARM/LeArm/stm32f1xx_hal_rcc.o \
../LeArm/MDK-ARM/LeArm/stm32f1xx_hal_rcc_ex.o \
../LeArm/MDK-ARM/LeArm/stm32f1xx_hal_spi.o \
../LeArm/MDK-ARM/LeArm/stm32f1xx_hal_tim.o \
../LeArm/MDK-ARM/LeArm/stm32f1xx_hal_tim_ex.o \
../LeArm/MDK-ARM/LeArm/stm32f1xx_hal_uart.o \
../LeArm/MDK-ARM/LeArm/stm32f1xx_it.o \
../LeArm/MDK-ARM/LeArm/system_stm32f1xx.o \
../LeArm/MDK-ARM/LeArm/tim.o \
../LeArm/MDK-ARM/LeArm/touch.o \
../LeArm/MDK-ARM/LeArm/u8g2_bitmap.o \
../LeArm/MDK-ARM/LeArm/u8g2_box.o \
../LeArm/MDK-ARM/LeArm/u8g2_buffer.o \
../LeArm/MDK-ARM/LeArm/u8g2_button.o \
../LeArm/MDK-ARM/LeArm/u8g2_circle.o \
../LeArm/MDK-ARM/LeArm/u8g2_cleardisplay.o \
../LeArm/MDK-ARM/LeArm/u8g2_d_memory.o \
../LeArm/MDK-ARM/LeArm/u8g2_d_setup.o \
../LeArm/MDK-ARM/LeArm/u8g2_font.o \
../LeArm/MDK-ARM/LeArm/u8g2_fonts.o \
../LeArm/MDK-ARM/LeArm/u8g2_hvline.o \
../LeArm/MDK-ARM/LeArm/u8g2_input_value.o \
../LeArm/MDK-ARM/LeArm/u8g2_intersection.o \
../LeArm/MDK-ARM/LeArm/u8g2_kerning.o \
../LeArm/MDK-ARM/LeArm/u8g2_line.o \
../LeArm/MDK-ARM/LeArm/u8g2_ll_hvline.o \
../LeArm/MDK-ARM/LeArm/u8g2_message.o \
../LeArm/MDK-ARM/LeArm/u8g2_polygon.o \
../LeArm/MDK-ARM/LeArm/u8g2_selection_list.o \
../LeArm/MDK-ARM/LeArm/u8g2_setup.o \
../LeArm/MDK-ARM/LeArm/u8log.o \
../LeArm/MDK-ARM/LeArm/u8log_u8g2.o \
../LeArm/MDK-ARM/LeArm/u8log_u8x8.o \
../LeArm/MDK-ARM/LeArm/u8x8_8x8.o \
../LeArm/MDK-ARM/LeArm/u8x8_byte.o \
../LeArm/MDK-ARM/LeArm/u8x8_cad.o \
../LeArm/MDK-ARM/LeArm/u8x8_capture.o \
../LeArm/MDK-ARM/LeArm/u8x8_d_ssd1306_128x32.o \
../LeArm/MDK-ARM/LeArm/u8x8_d_ssd1306_128x64_noname.o \
../LeArm/MDK-ARM/LeArm/u8x8_d_stdio.o \
../LeArm/MDK-ARM/LeArm/u8x8_debounce.o \
../LeArm/MDK-ARM/LeArm/u8x8_display.o \
../LeArm/MDK-ARM/LeArm/u8x8_fonts.o \
../LeArm/MDK-ARM/LeArm/u8x8_gpio.o \
../LeArm/MDK-ARM/LeArm/u8x8_input_value.o \
../LeArm/MDK-ARM/LeArm/u8x8_message.o \
../LeArm/MDK-ARM/LeArm/u8x8_selection_list.o \
../LeArm/MDK-ARM/LeArm/u8x8_setup.o \
../LeArm/MDK-ARM/LeArm/u8x8_string.o \
../LeArm/MDK-ARM/LeArm/u8x8_u16toa.o \
../LeArm/MDK-ARM/LeArm/u8x8_u8toa.o \
../LeArm/MDK-ARM/LeArm/ultrasound.o \
../LeArm/MDK-ARM/LeArm/usart.o \
../LeArm/MDK-ARM/LeArm/w25q64.o \
../LeArm/MDK-ARM/LeArm/wonder_echo.o \
../LeArm/MDK-ARM/LeArm/wonder_mv.o 


