#!/usr/bin/env python3

# A command-line utility providing a more user-friendly interface to XLS tools
# Primary targeting DSLX
# Supports simulation, optimization, and verilog generation

import argparse
from dataclasses import dataclass
import dataclasses
import logging
from pathlib import Path
import shutil
import subprocess
from types import UnionType
from typing import Any, Callable
import sys


# get full path to this script
import os
import inspect
import typing

# get full path to this script
SCRIPT_DIR = Path(os.path.abspath(inspect.getfile(inspect.currentframe()))).parent  # type: ignore


tool_path_map = {
    "ir_converter": "dslx/ir_convert/ir_converter_main",
    "opt": "tools/opt_main",
    "codegen": "tools/codegen_main",
    "interpreter": "dslx/interpreter_main",
}


class RunError(Exception):
    """Error raised when a command fails to run."""


def run_executable(executable: str | Path, args: list, stdout=None):
    """Run an executable with arguments."""
    exec_path = executable
    if isinstance(executable, str) and executable.count("/") == 0:
        exec_path = shutil.which(executable)
        if exec_path is None:
            raise RunError(f"Could not find executable {executable}")
    cmd = [str(exec_path)] + [str(arg) for arg in args]

    if stdout is not None:
        print("Running '" + " ".join(cmd) + f"' with <stdout> redirected to to '{stdout}'")
        with open(stdout, "w", encoding="utf-8") as f:
            subprocess.run(cmd, check=True, stdout=f)
    else:
        print("Running " + " ".join(cmd))
        subprocess.run(cmd, check=True)


def should_run(srcs_: list[Path | str], artifacts_: str | list[Path | str] | None) -> bool:
    """Check if a command should be run based on the timestamps of its inputs and outputs."""
    # TODO handle flag changes
    srcs = [Path(src) for src in srcs_]
    if artifacts_ is None:
        artifacts_ = []
    elif not isinstance(artifacts_, list):
        artifacts_ = [artifacts_]
    artifacts = [Path(art) for art in artifacts_]
    missing_artifacts = [
        art
        for art in artifacts
        if not art.exists() or (os.path.isfile(art) and os.path.getsize(art) == 0)
    ]
    if not artifacts or missing_artifacts:
        logger.debug(f"artifacts: {missing_artifacts} do not exist, should run")
        return True
    srcs_mtime = max([src.stat().st_mtime for src in srcs])
    artifacts_mtime = min([art.stat().st_mtime for art in artifacts])
    ret = artifacts_mtime < srcs_mtime
    logger.debug(f"srcs_mtime: {srcs_mtime} artifacts_mtime: {artifacts_mtime} should_run: {ret}")
    return ret


def find_executable(name: str, priority_paths=None, alt_paths=None) -> str | None:
    """Find an executable in the system path."""
    if priority_paths:
        priority_paths = (os.pathsep).join(str(p) for p in priority_paths)
        path = shutil.which(name, path=priority_paths)
        if path:
            return path
    path = shutil.which(name)
    if path:
        return path
    elif alt_paths:
        alt_paths = (os.pathsep).join(str(p) for p in alt_paths)
        return shutil.which(name, path=alt_paths)
    return None


logger = logging.getLogger(__name__)


@dataclass
class Generator:
    """Base class for generators."""

    _srcs: list[str | Path]
    _exec_path: str | Path | dict[str, str | Path]
    _force_run: bool = False

    def __post_init__(self):
        for field in dataclasses.fields(self):
            value = getattr(self, field.name)
            required_type = field.type
            # logger.debug(
            #     f"field: {field.name} type: {required_type} origin:{typing.get_origin(required_type)}"
            # )
            convert_type = required_type
            if typing.get_origin(required_type) == UnionType:
                required_type = typing.get_args(required_type)
                assert isinstance(required_type, tuple)
                convert_type = required_type[0]
            elif hasattr(required_type, "__origin__"):
                required_type = required_type.__origin__

            logger.debug(
                "name: %s value: %s required type: %s (%s) provided type: %s",
                field.name,
                value,
                required_type,
                type(required_type),
                type(value),
            )
            if value is not None and not isinstance(value, required_type):
                if isinstance(value, str) and isinstance(
                    required_type, (int, bool, float, Callable)
                ):
                    logger.debug(f"Converting {field.name} from {type(value)} to {required_type}")
                    try:
                        value = convert_type(value)
                        setattr(self, field.name, value)
                        return
                    except ValueError:
                        pass
                raise ValueError(
                    f"Expected {field.name} to be {required_type}, " f"got {repr(value)}"
                )

    def _phoney(self):
        return []

    def settings(self):
        return {
            k: v
            for k, v in self.__dict__.items()
            if not k.startswith("_") and k not in self._phoney() and v is not None
        }

    def _validate(self):
        pass

    def _generate(self):
        ...

    def run(self):
        self._validate()
        self._generate()

    def _parse_output(self) -> dict[str, Any]:
        return dict()

    def _should_run(self, artifacts: str | list[Path | str] | None) -> bool:
        return self._force_run or should_run(self._srcs, artifacts)


String = str | None
Int = int | None
Float = float | None
Bool = bool | None


@dataclass
class XlsGenerator(Generator):
    pass


@dataclass
class Codegen(XlsGenerator):
    """generates verilog from XLS IR."""

    top: String = None
    generator: String = None
    pipeline_stages: Int = None

    input_valid_signal: String = None
    output_valid_signal: String = None
    manual_load_enable_signal: String = None
    flop_inputs_kind: String = None
    flop_outputs_kind: String = None
    module_name: String = None
    reset: String = None
    gate_format: String = None
    assert_format: String = None
    smulp_format: String = None
    umulp_format: String = None
    ram_configurations: String = None
    gate_recvs: Bool = None
    array_index_bounds_checking: Bool = None
    clock_period_ps: Int = None
    # # =============== scheduling: =================
    clock_margin_percent: Int = None
    period_relaxation_percent: Int = None
    worst_case_throughput: Int = None
    additional_input_delay_ps: Int = None
    ffi_fallback_delay_ps: Int = None
    io_constraints: list[str] | str | None = None  # repeated
    receives_first_sends_last: Bool = None
    mutual_exclusion_z3_rlimit: Int = None
    # failure_behavior: str  # SchedulingFailureBehaviorProto
    use_fdo: Bool = None
    fdo_iteration_number: Int = None
    fdo_delay_driven_path_number: Int = None
    fdo_fanout_driven_path_number: Int = None
    fdo_refinement_stochastic_ratio: Float = None
    fdo_path_evaluate_strategy: String = None
    fdo_synthesizer_name: String = None
    fdo_yosys_path: String = None
    fdo_sta_path: String = None
    fdo_synthesis_libraries: String = None

    output_verilog_path: String = None

    minimize_clock_on_failure: Bool = None
    flop_single_value_channels: Bool = None
    add_idle_output: Bool = None
    reset_active_low: Bool = None
    reset_asynchronous: Bool = None
    reset_data_path: Bool = None
    separate_lines: Bool = None

    delay_model: str = "unit"
    output_verilog_line_map_path: String = None

    flop_inputs: bool = True
    flop_outputs: bool = True
    streaming_channel_data_suffix: str = "_bits"
    streaming_channel_valid_suffix: str = "_valid"
    streaming_channel_ready_suffix: str = "_ready"

    use_system_verilog: bool = True

    _exec_path: str | Path | None = find_executable(
        "codegen_main", priority_paths=[SCRIPT_DIR / "bazel-bin/xls/tools"]
    )

    def _validate(self):
        assert self._srcs, "No input files specified"
        for src in self._srcs:
            assert Path(src).exists(), f"Input file {src} does not exist"
        if self.generator is None:
            self.generator = (
                "pipeline" if self.pipeline_stages and self.pipeline_stages > 0 else "combinational"
            )
        elif self.generator == "pipeline" and not self.pipeline_stages:
            self.pipeline_stages = 1

        if self.output_verilog_path is None:
            suffix = ".sv" if self.use_system_verilog else ".v"
            p = Path(self._srcs[-1])
            self.output_verilog_path = str(p.with_stem(p.stem.split(".", 1)[0]).with_suffix(suffix))

    def _generate(self):
        def fmt_val(v):
            return str(v).lower() if isinstance(v, bool) else str(v)

        if self._should_run(self.output_verilog_path):
            args = self._srcs + [f"--{k}={fmt_val(v)}" for k, v in self.settings().items()]
            assert self._exec_path is not None
            run_executable(self._exec_path, args)


__HAS_VARNAME__: bool | None = None


def get_argname(up_name: str):
    global __HAS_VARNAME__  # pylint: disable=global-statement
    if __HAS_VARNAME__ is None:
        try:
            from varname import argname  # pylint: disable=import-outside-toplevel

            print(f"{'varname' in sys.modules}")
            print(f"{'argname' in sys.modules}")
            __HAS_VARNAME__ = True
        except ImportError:
            __HAS_VARNAME__ = False
    return argname(up_name, frame=2) if __HAS_VARNAME__ else up_name  # type: ignore


def fail(msg: str):
    """Fail with a message."""
    raise RunError(msg)


def enforce_type(expected_type, value, none_ok=True):
    """Enforce a type."""
    if value is None and none_ok:
        return value
    convert_type = expected_type
    if typing.get_origin(expected_type) == UnionType:
        expected_type = typing.get_args(expected_type)
        assert isinstance(expected_type, tuple)
        convert_type = expected_type[0]
    if not isinstance(value, expected_type):
        if isinstance(convert_type, (bool, int, float)):
            value = convert_type(value)  # type: ignore
        else:
            fail(f"Expected {get_argname('value')} to be {expected_type}, got {repr(value)}")
    return value


def get_mangled_ir_symbol(
    module_name, function_name, parametric_values=None, is_implicit_token=False, is_proc_next=False
):
    """Returns the mangled IR symbol for the module/function combination.

    "Mangling" is the process of turning nicely namedspaced symbols into
    "grosser" (mangled) flat (non hierarchical) symbol, e.g. that lives on a
    package after IR conversion. To retrieve/execute functions that have been IR
    converted, we use their mangled names to refer to them in the IR namespace.

    Args:
      module_name: The DSLX module name that the function is within.
      function_name: The DSLX function name within the module.
      parametric_values: Any parametric values used for instantiation (e.g. for
        a parametric entry point that is known to be instantiated in the IR
        converted module). This is generally for more advanced use cases like
        internals testing. The argument is mutually exclusive with argument
        'is_proc_next'.
      is_implicit_token: A boolean flag denoting whether the symbol contains an
        implicit token. The argument is mutually exclusive with argument
        'is_proc_next'.
      is_proc_next: A boolean flag denoting whether the symbol is a
        next proc function. The argument is mutually exclusive with arguments:
        'parametric_values' and 'is_implicit_token'.

    Returns:
      The "mangled" symbol string.
    """

    # Type validation for optional inputs.
    parametric_values = enforce_type(list | tuple, parametric_values)
    is_implicit_token = enforce_type(bool, is_implicit_token)
    is_proc_next = enforce_type(bool, is_proc_next)

    print(f"parametric_values: {parametric_values}")

    # Presence validation for optional inputs.
    if is_proc_next and (parametric_values or is_implicit_token):
        fail(
            "Argument 'is_proc_next' is mutually exclusive with arguments: 'parametric_values' and 'is_implicit_token'."
        )

    prefix_str = ""
    if is_implicit_token:
        prefix_str = "itok__"

    suffix = ""

    if parametric_values:
        suffix = "__" + "_".join(
            [str(v) for v in parametric_values],
        )

    mangled_name = "__{}{}__{}{}".format(
        prefix_str,
        module_name,
        function_name,
        suffix,
    )

    if is_proc_next:
        mangled_name = mangled_name.replace(":", "_")
        mangled_name = mangled_name.replace("->", "__")
        mangled_name = mangled_name + "_0_next"

    return mangled_name


@dataclass
class OptIr(XlsGenerator):
    """Optimize XLS IR."""

    _exec_path: str | Path | None = find_executable(
        "opt_main", priority_paths=[SCRIPT_DIR / "bazel-bin/xls/tools"]
    )

    top: String = None
    ir_dump_path: String = None
    output_ir_path: String = None
    run_only_passes: list[str] | str | None = None
    skip_passes: list[str] | str | None = None
    opt_level: Int = None
    ram_rewrites_pb: String = None  # Path to protobuf describing ram rewrites; default: ""

    #  If specified, convert array indexes with
    # fewer than or equal to the given number of possible indices (by range
    # analysis) into chains of selects. Otherwise, this optimization is skipped,
    # since it can sometimes reduce output quality.); default: -1
    convert_array_index_to_select: Int = None

    inline_procs: Bool = None
    use_context_narrowing_analysis: Bool = None

    def _phoney(self):
        return super()._phoney() + ["output_ir_path"]

    def _validate(self):
        assert self._srcs, "No input files specified"
        for src in self._srcs:
            assert Path(src).exists(), f"Input file {src} does not exist"
        if isinstance(self.run_only_passes, list):
            self.run_only_passes = ":".join(self.run_only_passes)
        if isinstance(self.skip_passes, list):
            self.skip_passes = ":".join(self.skip_passes)

        if self.output_ir_path is None:
            verilog_out = Path(self._srcs[-1]).with_suffix(".opt.ir")
            self.output_ir_path = str(verilog_out)

    def _generate(self):
        def fmt_val(v):
            return str(v).lower() if isinstance(v, bool) else str(v)

        assert self.output_ir_path is not None

        if self._should_run(self.output_ir_path):
            args = self._srcs + [f"--{k}={fmt_val(v)}" for k, v in self.settings().items()]
            assert self._exec_path is not None
            run_executable(self._exec_path, args, stdout=self.output_ir_path)


@dataclass
class IrConverter(XlsGenerator):
    """Convert DSLX to XLS IR."""

    _exec_path: str | Path | None = find_executable(
        "ir_converter_main",
        priority_paths=[SCRIPT_DIR / "bazel-bin/xls/dslx/ir_convert"],
    )

    top: String = None
    package_name: String = None
    # ir_dump_path: String = None
    output_ir_path: String = None
    dslx_path: String | list[str] = None
    stdlib_path: String = None
    verify: Bool = None
    warnings_as_errors: Bool = None

    def _phoney(self):
        return super()._phoney() + ["output_ir_path"]

    def _validate(self):
        assert self._srcs, "No input files specified"
        for src in self._srcs:
            assert Path(src).exists(), f"Input file {src} does not exist"

        if self.output_ir_path is None:
            verilog_out = Path(self._srcs[-1]).with_suffix(".ir")
            self.output_ir_path = str(verilog_out)
        if isinstance(self.dslx_path, list):
            self.dslx_path = ":".join(self.dslx_path)

    def _generate(self):
        def fmt_val(v):
            return str(v).lower() if isinstance(v, bool) else str(v)

        if self._should_run(self.output_ir_path):
            assert self._exec_path is not None
            args = self._srcs + [f"--{k}={fmt_val(v)}" for k, v in self.settings().items()]
            run_executable(self._exec_path, args, stdout=self.output_ir_path)


def kvlist_to_dict(kvlist: list[str]) -> dict[str, dict[str, str]]:
    """Convert a list of key=value strings into a dictionary."""
    d = {k: v for k, v in [s.split("=", 1) for s in kvlist]}
    ret = dict()
    for k, v in d.items():
        k_split = k.split(".", 1)  # split on dots
        tk = k_split[-1]
        if len(k_split) == 1:
            t = "*"
        else:
            assert len(k_split) == 2
            t = k_split[0]
        ret[t] = {**ret.get(t, dict()), tk: v}
    return ret


def run(args, settings: dict[str, dict[str, Any]]):
    """Main entry point for the script."""

    def get_settings(tool: str) -> dict:
        return settings.get(tool, {})

    if args.action == "gen":
        ir_converter = IrConverter(
            _srcs=args.input,
            _force_run=args.force,
            **get_settings("ir_converter"),
        )
        ir_converter.run()
        assert ir_converter.output_ir_path

        opt_ir = OptIr(
            _srcs=[ir_converter.output_ir_path],
            _force_run=args.force,
            **get_settings("opt"),
        )
        opt_ir.run()
        assert opt_ir.output_ir_path
        Codegen(
            _srcs=[opt_ir.output_ir_path],
            _force_run=args.force,
            **get_settings("codegen"),
        ).run()

    elif args.action == "test":
        ...
    else:
        print(f"Unknown action: {args.action}")
        exit(1)


def set_if_not_exists(d: dict, k: str, v: Any):
    """Set a value in a dictionary if it does not already exist."""
    if k not in d:
        d[k] = v
    return d


def merge_hierarchical_dicts(d1: dict, d2: dict) -> dict:
    """Merge two hierarchical dictionaries."""
    ret = dict()
    for k, v in d1.items():
        if k in d2:
            if isinstance(v, dict):
                ret[k] = merge_hierarchical_dicts(v, d2[k])
            else:
                ret[k] = d2[k]
        else:
            ret[k] = v
    for k, v in d2.items():
        if k not in d1:
            ret[k] = v
    return ret


def path_stem(path: str | Path) -> str:
    """Get the stem of a path."""
    if isinstance(path, Path):
        return path.stem
    else:
        return os.path.basename(path).split(os.path.extsep, 1)[0]


# get command line arguments
def main():
    """Main entry point for the script."""
    parser = argparse.ArgumentParser(description="XLS command line utility")
    # action
    parser.add_argument(
        "action",
        metavar="action",
        default="gen",
        type=str,
        choices=["test", "gen"],
        help="action to perform",
    )
    # input file(s):
    parser.add_argument(
        "input",
        type=str,
        nargs="+",
        help="input file(s)",
    )
    # top module
    parser.add_argument(
        "-t",
        "--top",
        type=str,
        default=None,
        help="top module",
    )
    parser.add_argument(
        "-n",
        "--top_proc_next",
        action="store_true",
        default=False,
        help="top module",
    )
    parser.add_argument(
        "-s",
        "--settings",
        metavar="settings",
        type=str,
        default=[],
        nargs="+",
    )
    parser.add_argument(
        "-d",
        "--delay_model",
        type=str,
        #     choices=["unit", "sky130", "asap7", "ecp5"]
        default="unit",
    )
    parser.add_argument(
        "-p",
        "--pipeline_stages",
        type=int,
        default=None,
    )
    parser.add_argument(
        "-o",
        "--output-dir",
        type=Path,
        default=None,
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="print verbose output",
    )
    parser.add_argument(
        "-f",
        "--force",
        action="store_true",
        help="force execution of all commands",
    )
    parser.add_argument(
        "-c",
        "--clean",
        action="store_true",
        help="clean output files before running",
    )
    parser.add_argument(
        "-m",
        "--module",
        type=str,
        default=None,
        help="module name",
    )
    parser.add_argument(
        "--params",
        type=str,
        nargs="+",
        help="list of parametric values",
    )
    # get remainder unparsed arguments as a list
    # args, argv = parser.parse_known_args()
    args = parser.parse_args()
    ss = kvlist_to_dict(args.settings)
    opt_top = get_mangled_ir_symbol(
        args.module or path_stem(args.input[-1]),
        args.top,
        parametric_values=args.params,
        is_proc_next=args.top_proc_next,
    )
    print(f"opt_top: {opt_top}")
    ss = merge_hierarchical_dicts(
        {
            "ir_converter": {"top": args.top},
            "opt": {"top": opt_top},
            "codegen": {"pipeline_stages": args.pipeline_stages, "delay_model": args.delay_model},
        },
        ss,
    )

    run(args, ss)


if __name__ == "__main__":
    main()
