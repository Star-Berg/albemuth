################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
build-1734394257: ../F280039_Iris_node.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"D:/software/ti/ccs1281/ccs/utils/sysconfig_1.21.0/sysconfig_cli.bat" --script "D:/Github/albemuth/iris_280039c_board/F280039_Iris_node.syscfg" -o "syscfg" -s "D:/software/ti/c2000/C2000Ware_5_04_00_00/.metadata/sdk.json" -s "D:/Github/gmp_pro1/.metadata/product.json" -d "TMS320F280039C" -p "100PZ" --compiler ccs
	@echo 'Finished building: "$<"'
	@echo ' '

syscfg/board.c: build-1734394257 ../F280039_Iris_node.syscfg
syscfg/board.h: build-1734394257
syscfg/board.cmd.genlibs: build-1734394257
syscfg/board.opt: build-1734394257
syscfg/board.json: build-1734394257
syscfg/pinmux.csv: build-1734394257
syscfg/epwm.dot: build-1734394257
syscfg/device.c: build-1734394257
syscfg/device.h: build-1734394257
syscfg/adc.dot: build-1734394257
syscfg/c2000ware_libraries.cmd.genlibs: build-1734394257
syscfg/c2000ware_libraries.opt: build-1734394257
syscfg/c2000ware_libraries.c: build-1734394257
syscfg/c2000ware_libraries.h: build-1734394257
syscfg/clocktree.h: build-1734394257
syscfg: build-1734394257

syscfg/%.obj: ./syscfg/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"D:/software/ti/ccs1281/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcrc -O0 --fp_mode=relaxed --include_path="D:/Github/gmp_pro1" --include_path="D:/Github/gmp_pro1/csp/c28x_syscfg/" --include_path="D:/Github/gmp_pro1" --include_path="D:/Github/gmp_pro1/csp/c28x_syscfg/" --include_path="D:/Github/albemuth/iris_280039c_board" --include_path="D:/software/ti/ccs1281/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/include" --include_path="D:/Github/albemuth/iris_280039c_board/user" --include_path="D:/Github/albemuth/iris_280039c_board/xplt" --include_path="D:/Github/albemuth/iris_280039c_board/oled_driver" --include_path="D:/Github/albemuth/iris_280039c_board/u8g2_src" --include_path="D:/software/ti/c2000/C2000Ware_5_04_00_00/device_support/f28003x/headers/include" --include_path="D:/software/ti/c2000/C2000Ware_5_04_00_00/device_support/f28003x/common/include" --include_path="D:/software/ti/c2000/C2000Ware_5_04_00_00/driverlib/f28003x/driverlib/" --advice:performance=all --define=_FLASH -g --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="syscfg/$(basename $(<F)).d_raw" --include_path="D:/Github/albemuth/iris_280039c_board/Debug/syscfg" --obj_directory="syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

f28003x_codestartbranch.obj: D:/software/ti/c2000/C2000Ware_5_04_00_00/device_support/f28003x/common/source/f28003x_codestartbranch.asm $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"D:/software/ti/ccs1281/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcrc -O0 --fp_mode=relaxed --include_path="D:/Github/gmp_pro1" --include_path="D:/Github/gmp_pro1/csp/c28x_syscfg/" --include_path="D:/Github/gmp_pro1" --include_path="D:/Github/gmp_pro1/csp/c28x_syscfg/" --include_path="D:/Github/albemuth/iris_280039c_board" --include_path="D:/software/ti/ccs1281/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/include" --include_path="D:/Github/albemuth/iris_280039c_board/user" --include_path="D:/Github/albemuth/iris_280039c_board/xplt" --include_path="D:/Github/albemuth/iris_280039c_board/oled_driver" --include_path="D:/Github/albemuth/iris_280039c_board/u8g2_src" --include_path="D:/software/ti/c2000/C2000Ware_5_04_00_00/device_support/f28003x/headers/include" --include_path="D:/software/ti/c2000/C2000Ware_5_04_00_00/device_support/f28003x/common/include" --include_path="D:/software/ti/c2000/C2000Ware_5_04_00_00/driverlib/f28003x/driverlib/" --advice:performance=all --define=_FLASH -g --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="D:/Github/albemuth/iris_280039c_board/Debug/syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


