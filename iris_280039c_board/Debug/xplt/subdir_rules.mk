################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
xplt/%.obj: ../xplt/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"D:/software/ti/ccs1281/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcrc -O1 --fp_mode=relaxed --include_path="D:/Github/gmp_pro" --include_path="D:/Github/gmp_pro/csp/c28x_syscfg/" --include_path="D:/Github/gmp_pro" --include_path="D:/Github/gmp_pro/csp/c28x_syscfg/" --include_path=".." --include_path="D:/software/ti/ccs1281/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/include" --include_path="../user" --include_path="../xplt" --include_path="../oled_driver" --include_path="../u8g2_src" --include_path="D:/software/ti/c2000/C2000Ware_5_04_00_00/device_support/f28003x/headers/include" --include_path="D:/software/ti/c2000/C2000Ware_5_04_00_00/device_support/f28003x/common/include" --include_path="D:/software/ti/c2000/C2000Ware_5_04_00_00/driverlib/f28003x/driverlib/" --advice:performance=all --define=_FLASH -g --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="xplt/$(basename $(<F)).d_raw" --include_path="./syscfg" --obj_directory="xplt" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


