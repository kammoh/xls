#!/bin/sh

[ -z "$PREFIX" ] && PREFIX=$PWD/inst

# in "$PREFIX" is not empty ask for confirmation
if [ -d "$PREFIX" ] && [ -n "$(ls -A $PREFIX)" ]; then
    echo "Directory $PREFIX is not empty. Continue?"
    select yn in "Yes" "No"; do
        echo "You entered $yn"
        case $yn in
            1 | y | Y | Yes) break;;
            *) exit;;
        esac
    done
fi

rm -rf ${PREFIX}

BASE="//xls/tools/... \
    //xls/dslx/... \
    -//xls/dslx/stdlib:float32_add_cc -//xls/dslx/stdlib:float32_add_cc_gen_aot \
    -//xls/dslx/stdlib:float32_fma_cc -//xls/dslx/stdlib:float32_fma_cc_gen_aot \
    //xls/interpreter/... \
    //xls/codegen/... \
    //xls/visualization/...
"

EXTRA="//xls/scheduling/... \
    //xls/fuzzer/... \
    //xls/netlist/... \
    //xls/contrib/xlscc:xlscc \
    //xls/solvers/... \
    //xls/uncore_rtl/... \
    //xls/synthesis/yosys/... \
    //xls/modules/... \
    -//xls/modules/aes:aes_encrypt_cc -//xls/modules/aes:aes_encrypt_cc.cc -//xls/modules/aes:aes_encrypt_cc.h -//xls/modules/aes:aes_encrypt_cc.o -//xls/modules/aes:aes_encrypt_cc_gen_aot \
    -//xls/modules/aes:aes_decrypt_cc -//xls/modules/aes:aes_decrypt_cc.cc -//xls/modules/aes:aes_decrypt_cc.h -//xls/modules/aes:aes_decrypt_cc.o -//xls/modules/aes:aes_decrypt_cc_gen_aot \
"
    # //xls/modules/rle/... \
    # //xls/modules/aes:aes_dslx //xls/modules/aes:aes_ctr //xls/modules/aes:aes_ghash //xls/modules/aes:aes_gcm  \

# --spawn_strategy=standalone -s
bazel build -c opt --verbose_failures --sandbox_debug -- ${BASE} ${EXTRA}


# install targets
mkdir -p $PREFIX/share/xls
bazel-bin/xls/tools/package_bazel_build --output_dir $PREFIX/share/xls \
					--inc_target xls/dslx/interpreter_main \
					--inc_target xls/dslx/ir_convert/ir_converter_main \
					--inc_target xls/tools/repl \
					--inc_target xls/tools/opt_main \
					--inc_target xls/tools/codegen_main \
					--inc_target xls/tools/proto_to_dslx_main \
                    --inc_target xls/contrib/xlscc/xlscc

# create tools symlinks
mkdir -p $PREFIX/bin
for f in xls/dslx/interpreter_main \
	 xls/dslx/ir_convert/ir_converter_main \
	 xls/tools/opt_main \
	 xls/tools/codegen_main \
	 xls/tools/proto_to_dslx_main \
         xls/contrib/xlscc/xlscc
do
    ln -sf $PREFIX/share/xls/$f $PREFIX/bin/$(basename $f)
done