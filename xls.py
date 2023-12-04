#!/usr/bin/env python3

# A command-line utility providing a more user-friendly interface to XLS tools
# Primary targeting DSLX
# Supports simulation, optimization, and verilog generation

import argparse
from dataclasses import dataclass
from pathlib import Path
import subprocess
from typing import Any, Optional

# get full path to this script
import os
import sys
import inspect

# get full path to this script
script_path = Path(os.path.abspath(inspect.getfile(inspect.currentframe()))).parent


tool_path_map = {
    "ir_converter": "dslx/ir_convert/ir_converter_main",
    "opt": "tools/opt_main",
    "codegen": "tools/codegen_main",
    "interpreter": "dslx/interpreter_main",
}


def run_command(command: str, args: list, stdout: Optional[Path | str] = None, use_bazel=False):
    """Run a command with arguments."""
    [cmd_path, cmd_target] = tool_path_map[command].rsplit("/", 1)
    if use_bazel:
        cmd = ["bazel", "run", "-c", "opt", f"//xls/{cmd_path}:{cmd_target}", "--"]
    else:
        cmd = [script_path / f"bazel-bin/xls/{cmd_path}/{cmd_target}"]
    cmd += args
    cmd = [str(arg) for arg in cmd]
    print("Running command: " + " ".join(cmd))
    if stdout is not None:
        print(f"Writing stdout to {stdout}")
        with open(stdout, "w", encoding="utf-8") as f:
            subprocess.run(cmd, check=True, stdout=f)
    else:
        subprocess.run(cmd, check=True)


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

    def get_settings(tool: str) -> dict[str, str]:
        return settings.get(tool, {})

    def settings_to_args(settings: str | dict[str, dict[str, Any]]) -> list[str]:
        """Convert a dictionary of settings into a list of command-line arguments."""
        s = get_settings(settings) if isinstance(settings, str) else settings
        return [f"--{k}={v}" for k, v in s.items()]

    if args.action == "gen":
        output_base = Path(args.input[-1]).stem + f"_{args.top}"
        base_dir = Path("./")
        print(f"Generating Verilog in {base_dir}")
        ir_out = base_dir / Path(output_base).with_suffix(".ir")
        print(f"Writing IR to {ir_out}")
        run_args = args.input
        if args.top:
            run_args += ["--top", args.top]
        run_command(
            "ir_converter",
            run_args + settings_to_args("ir_converter"),
            stdout=ir_out,
        )
        opt_ir_out = base_dir / Path(output_base).with_suffix(".opt.ir")
        run_command("opt", [ir_out] + settings_to_args("opt"), stdout=opt_ir_out)
        verilog_out = base_dir / Path(output_base).with_suffix(".v")
        run_command(
            "codegen",
            [opt_ir_out] + settings_to_args("codegen"),
            stdout=verilog_out,
        )
        print(f"Generated verilog: {verilog_out}")
    elif args.action == "test":
        run_command(
            "interpreter",
            args.input + settings_to_args("interpreter"),
        )
    else:
        print(f"Unknown action: {args.action}")
        exit(1)


# dataclass for settings
@dataclass
class Settings:
    """Settings for a tool."""

    pipeline_stages: int = 1
    delay_model: str = "unit"


def set_if_not_exists(d: dict, k: str, v: Any):
    if k not in d:
        d[k] = v
    return d


# get command line arguments
def main():
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
        default="main",
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
        default=0,  # 0: combinational
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
    # get remainder unparsed arguments as a list
    # args, argv = parser.parse_known_args()
    args = parser.parse_args()
    ss = kvlist_to_dict(args.settings)
    set_if_not_exists(ss, "codegen", {})
    cg = ss["codegen"]
    set_if_not_exists(cg, "delay_model", args.delay_model)
    set_if_not_exists(cg, "generator", "pipeline" if args.pipeline_stages > 0 else "combinational")
    if cg.get("generator") == "pipeline":
        set_if_not_exists(cg, "pipeline_stages", args.pipeline_stages)
    run(args, ss)


if __name__ == "__main__":
    main()
