#!/bin/sh
# --spawn_strategy=standalone -s
bazel build -c opt --verbose_failures --sandbox_debug -- \
    //xls/dslx:interpreter_main //xls/dslx/ir_convert:ir_converter_main \
    //xls/tools:opt_main //xls/tools:codegen_main -//xls/contrib/xlscc/... \
    //xls/dslx/lsp:dslx_ls //xls/dslx:dslx_fmt //xls/dslx:highlight_main //xls/dslx:dslx_fmt //xls/tools:repl \
    //xls/tools:proto_to_dslx_main //xls/tools:smtlib_emitter_main //xls/tools:booleanify_main \
    //xls/tools:solver //xls/tools:bdd_stats //xls/tools:benchmark_main //xls/tools:codegen_main \
    //xls/tools:delay_info_main //xls/tools:eval_ir_main //xls/tools:ir_minimizer_main \
    //xls/tools:ir_stats_main //xls/tools:check_ir_equivalence_main //xls/tools:print_bom \
    //xls/visualization/ir_viz/...