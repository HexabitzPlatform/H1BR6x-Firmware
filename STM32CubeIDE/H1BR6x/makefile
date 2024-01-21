################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include User/subdir.mk
-include Thirdparty/STM32F0xx_HAL_Driver/Src/subdir.mk
-include Thirdparty/Middleware/FreeRTOS/Source/portable/MemMang/subdir.mk
-include Thirdparty/Middleware/FreeRTOS/Source/portable/GCC/ARM_CM0/subdir.mk
-include Thirdparty/Middleware/FreeRTOS/Source/CMSIS_RTOS/subdir.mk
-include Thirdparty/Middleware/FreeRTOS/Source/subdir.mk
-include Thirdparty/Middleware/FatFs/source/subdir.mk
-include Thirdparty/Middleware/FatFs/drivers/subdir.mk
-include Thirdparty/CMSIS/Device/ST/STM32F0xx/Source/Templates/subdir.mk
-include H1BR6/subdir.mk
-include BOS/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

BUILD_ARTIFACT_NAME := H1BR6x
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX := 
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME).$(BUILD_ARTIFACT_EXTENSION)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
H1BR6x.elf \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
H1BR6x.list \

OBJCOPY_HEX += \
H1BR6x.hex \


# All Target
all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: H1BR6x.elf secondary-outputs

# Tool invocations
H1BR6x.elf: $(OBJS) $(USER_OBJS) D:\Hexabitz\for\ Release\Modules\ firmware\H1BR6x\STM32CubeIDE\STM32F091RCTX_FLASH.ld
	arm-none-eabi-gcc -o "H1BR6x.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m0 -T"D:\Hexabitz\for Release\Modules firmware\H1BR6x\STM32CubeIDE\STM32F091RCTX_FLASH.ld" --specs=nosys.specs -Wl,-Map="H1BR6x.map" -Wl,--gc-sections -static -Wstack-usage=256  -mfloat-abi=soft -mthumb -u _printf_float -u _scanf_float -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

H1BR6x.list: $(EXECUTABLES)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "H1BR6x.list"
	@echo 'Finished building: $@'
	@echo ' '

H1BR6x.hex: $(EXECUTABLES)
	arm-none-eabi-objcopy  -O ihex $(EXECUTABLES) "H1BR6x.hex"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) *
	-@echo ' '

post-build:
	arm-none-eabi-objcopy -O ihex H1BR6x.elf H1BR6x.hex
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_HEX)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents fail-specified-linker-script-missing warn-no-linker-script-specified
.SECONDARY: post-build

-include ../makefile.targets
