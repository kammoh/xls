# Copyright 2021 The XLS Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Loads the rules_hdl package which contains Bazel rules for other tools that
XLS uses."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def repo():
    # Required to support rules_hdl, 0.9.1 release is current as of 2023-11-22.
    http_archive(
        name = "rules_pkg",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/rules_pkg/releases/download/0.9.1/rules_pkg-0.9.1.tar.gz",
            "https://github.com/bazelbuild/rules_pkg/releases/download/0.9.1/rules_pkg-0.9.1.tar.gz",
        ],
        sha256 = "8f9ee2dc10c1ae514ee599a8b42ed99fa262b757058f65ad3c384289ff70c4b8",
    )

    # Required to support rules_hdl.
    http_archive(
        name = "rules_7zip",
        strip_prefix = "rules_7zip-e00b15d3cb76b78ddc1c15e7426eb1d1b7ddaa3e",
        urls = ["https://github.com/zaucy/rules_7zip/archive/e00b15d3cb76b78ddc1c15e7426eb1d1b7ddaa3e.zip"],
        sha256 = "fd9e99f6ccb9e946755f9bc444abefbdd1eedb32c372c56dcacc7eb486aed178",
    )

    # Current as of 2023-12-05
    # git_hash = "37efe7ca7b8469454eacd3b70ef5fe5ddfea2cf4"
    # archive_sha256 = "d32c5d3a0864e351ca8e6e40d3cf1bb4b78cfc85ea81b5c52bd44b23d274d321"

    maybe(
        git_repository,
        name = "rules_hdl",
        # sha256 = archive_sha256,
        # strip_prefix = "bazel_rules_hdl-%s" % git_hash,
        # urls = [
        #     # "https://github.com/hdl/bazel_rules_hdl/archive/%s.tar.gz" % git_hash,
        #     "https://github.com/kammoh/bazel_rules_hdl/archive/%s.tar.gz" % "main",
        # ],
        branch = "main",
        remote=  "https://github.com/kammoh/bazel_rules_hdl.git",
        repo_mapping = {
            "@rules_hdl_cpython": "@python39",
        },
    )
