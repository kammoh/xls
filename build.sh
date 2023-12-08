#!/bin/bash

[ -z "$PREFIX" ] && PREFIX=$PWD/inst_$(date +%m%d%H%M%S)

echo "XLS will be installed in $PREFIX"

# in "$PREFIX" is not empty ask for confirmation
if [ -d "$PREFIX" ] && [ -n "$(ls -A $PREFIX)" ]; then
  echo "Directory $PREFIX is not empty. Continue?"
  select yn in "Yes" "No"; do
    echo "You entered $yn"
    case $yn in
    1 | y | Y | Yes) break ;;
    *) exit ;;
    esac
  done
fi

rm -rf ${PREFIX}

# function fail which will be called on error and exit
fail() {
  echo "Build failed"
  exit 1
}

BINARIES_SET="xls/dslx/... xls/tools/... xls/netlist/... //xls/visualization/..."
BINARIES=( $(bazel query 'kind("cc_binary", set(' ${BINARIES_SET} '))') )

echo "BINARIES: ${BINARIES[@]}"

BINARIES+=( //xls/synthesis/yosys:yosys_server_main
  //xls/synthesis:synthesis_client_main
  //xls/synthesis:timing_characterization_client_main
)
# echo "BINARIES: ${BINARIES[@]}"

STDLIB=($(bazel query 'kind("xls_dslx_library", set(xls/dslx/stdlib/...) )'))

# echo "STDLIB: ${STDLIB[@]}"

MODULES=() #$(bazel query 'kind("xls_dslx_library", set(xls/modules/...) )')

BUILD_TARGETS=( ${BINARIES[@]} ${STDLIB[@]} ${MODULES[@]} //xls/visualization/... )

echo "BUILD_TARGETS: ${BUILD_TARGETS[@]}"

# EXTRA="//xls/tools/... \
#     //xls/dslx/... \
#     -//xls/dslx/stdlib:float32_add_cc -//xls/dslx/stdlib:float32_add_cc_gen_aot \
#     -//xls/dslx/stdlib:float32_fma_cc -//xls/dslx/stdlib:float32_fma_cc_gen_aot \
#     //xls/modules/... \
#     -//xls/modules/aes:aes_test -//xls/modules/aes:aes_encrypt_cc -//xls/modules/aes:aes_decrypt_cc \
#     //xls/modules/aes:aes_dslx //xls/modules/aes:aes_ctr //xls/modules/aes:aes_ghash //xls/modules/aes:aes_gcm
# "

bazel build -c opt --verbose_failures --sandbox_debug -- ${BUILD_TARGETS[@]} || fail

# create share directory if not exists
[ -d "$PREFIX/share" ] || mkdir -p $PREFIX/share || fail

PKG_TARGETS=( ${BINARIES[@]} )

# use package_bazel_build to copy files
bazel-bin/xls/tools/package_bazel_build --output_dir $PREFIX/share ${PKG_TARGETS[@]/#/--inc_target } || fail

BIN_DIR=$PREFIX/bin

# create tools symlinks
[ -d "$BIN_DIR" ] || mkdir -p "$BIN_DIR" || fail

pushd $PREFIX/share
BINS=$(ls xls/dslx/*_main xls/dslx/*/*_main xls/tools/*_main)
popd

for f in $BINS \
  xls/tools/repl \
  xls/dslx/lsp/dslx_ls \
  xls/dslx/dslx_fmt \
  xls/contrib/xlscc/xlscc; do
  ln -sf ../share/$f $BIN_DIR/$(basename $f) || fail
  echo "$BIN_DIR/$(basename $f) --> $PREFIX/share/$f"
done

[ -f $PWD/inst ] && rm $PWD/inst
ln -sf $PREFIX $PWD/inst || fail

echo "Build completed. Please add $BIN_DIR to PATH to use XLS tools"
