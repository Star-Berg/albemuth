################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
driverlib/%.obj: ../driverlib/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"D:/software/ti/ccs1281/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcrc -O0 --fp_mode=relaxed --include_path="D:/Github/gmp_pro1" --include_path="D:/Github/gmp_pro1/csp/c28x_syscfg/" --include_path="D:/Github/gmp_pro1" --include_path="D:/Github/gmp_pro1/csp/c28x_syscfg/" --include_path="D:/Github/albemuth/iris_280039c_board" --include_path="D:/software/ti/ccs1281/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/include" --include_path="D:/Github/albemuth/iris_280039c_board/user" --include_path="D:/Github/albemuth/iris_280039c_board/xplt" --include_path="D:/Github/albemuth/iris_280039c_board/oled_driver" --include_path="D:/Github/albemuth/iris_280039c_board/u8g2_src" --include_path="D:/software/ti/c2000/C2000Ware_5_04_00_00/device_support/f28003x/headers/include" --include_path="D:/software/ti/c2000/C2000Ware_5_04_00_00/device_support/f28003x/common/include" --include_path="D:/software/ti/c2000/C2000Ware_5_04_00_00/driverlib/f28003x/driverlib/" --advice:performance=all --define=_FLASH -g --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="driverlib/$(basename $(<F)).d_raw" --include_path="D:/Github/albemuth/iris_280039c_board/Debug/syscfg" --obj_directory="driverlib" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


