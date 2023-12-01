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

# function fail which will be called on error and exit
fail() {
    echo "Build failed"
    exit 1
}

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
    -//xls/modules/aes:aes_test -//xls/modules/aes:aes_encrypt_cc -//xls/modules/aes:aes_decrypt_cc \
    //xls/modules/aes:aes_dslx //xls/modules/aes:aes_ctr //xls/modules/aes:aes_ghash //xls/modules/aes:aes_gcm
"
# -//xls/modules/aes/... \

# [ -z $JOBS ] && JOBS=$(nproc) #  --jobs=$JOBS
# --spawn_strategy=standalone -s

bazel build -c opt --verbose_failures --sandbox_debug -- ${BASE} ${EXTRA} || fail

# create share directory
mkdir -p $PREFIX/share

# copy files
bazel-bin/xls/tools/package_bazel_build --output_dir $PREFIX/share \
					--inc_target xls/dslx/lsp/dslx_ls \
					--inc_target xls/dslx/interpreter_main \
					--inc_target xls/dslx/dslx_fmt \
					--inc_target xls/dslx/highlight_main \
					--inc_target xls/dslx/type_system/typecheck_main \
					--inc_target xls/dslx/strip_comments_main \
					--inc_target xls/dslx/ir_convert/ir_converter_main \
					--inc_target xls/dslx/cpp_transpiler/cpp_transpiler_main \
					--inc_target xls/tools/repl \
					--inc_target xls/tools/eval_dslx_main \
					--inc_target xls/tools/eval_proc_main \
					--inc_target xls/tools/eval_ir_main \
					--inc_target xls/tools/opt_main \
					--inc_target xls/tools/lec_main \
					--inc_target xls/tools/proto_to_dslx_main \
					--inc_target xls/tools/booleanify_main \
					--inc_target xls/tools/simulate_module_main \
					--inc_target xls/tools/benchmark_main \
					--inc_target xls/tools/codegen_main \
					--inc_target xls/tools/benchmark_codegen_main \
					--inc_target xls/tools/smtlib_emitter_main \
					--inc_target xls/tools/extract_stage_main \
					--inc_target xls/tools/wrap_io_main \
					--inc_target xls/tools/netlist_interpreter_main \
					--inc_target xls/tools/check_ir_equivalence_main \
					--inc_target xls/tools/ir_stats_main \
					--inc_target xls/tools/drpc_main \
					--inc_target xls/tools/delay_info_main \
                    --inc_target xls/contrib/xlscc/xlscc \
                    || fail

# create tools symlinks
mkdir -p $PREFIX/bin
for f in xls/dslx/interpreter_main \
	 xls/dslx/ir_convert/ir_converter_main \
	 xls/tools/opt_main \
	 xls/tools/codegen_main \
	 xls/tools/lec_main \
	 xls/tools/repl \
	 xls/tools/wrap_io_main \
	 xls/dslx/lsp/dslx_ls \
	 xls/dslx/dslx_fmt \
	 xls/tools/proto_to_dslx_main \
     xls/contrib/xlscc/xlscc
do
    ln -sf ../share/$f $PREFIX/bin/$(basename $f) || fail
done