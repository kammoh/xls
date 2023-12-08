# Copyright 2020 The XLS Authors
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

"""Provides helper that loads external repositories with third-party code."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("//dependency_support/boost:workspace.bzl", repo_boost = "repo")
load("//dependency_support/llvm:workspace.bzl", repo_llvm = "repo")
load("//dependency_support/rules_hdl:workspace.bzl", repo_rules_hdl = "repo")

def load_external_repositories():
    """Loads external repositories with third-party code."""

    # Note: there are more direct dependencies than are explicitly listed here.
    #
    # By letting direct dependencies be satisfied by transitive WORKSPACE
    # setups we let the transitive dependency versions "float" to satisfy their
    # package requirements, so that we can bump our dependency versions here
    # more easily without debugging/resolving unnecessary conflicts.
    #
    # This situation will change when XLS moves to bzlmod. See
    # https://github.com/google/xls/issues/865 and
    # https://github.com/google/xls/issues/931#issue-1667228764 for more
    # information / background.

    repo_boost()
    repo_llvm()
    repo_rules_hdl()

    http_archive(
        name = "rules_cc",
        urls = ["https://github.com/bazelbuild/rules_cc/releases/download/0.0.5/rules_cc-0.0.5.tar.gz"],
        sha256 = "2004c71f3e0a88080b2bd3b6d3b73b4c597116db9c9a36676d0ffad39b849214",
        strip_prefix = "rules_cc-0.0.5",
    )

    # V 1.14.0 (released 2023-08-02)
    http_archive(
        name = "com_google_googletest",
        urls = ["https://github.com/google/googletest/archive/76bb2afb8b522d24496ad1c757a49784fbfa2e42.zip"],
        strip_prefix = "googletest-76bb2afb8b522d24496ad1c757a49784fbfa2e42",
        sha256 = "c8c5fb6bf567995cb5ea5c088c2fbaca6430aebd8173dd7161975cd32cbe0bda",
    )

    # LTS 20230802.1 (released 2023-09-18)
    http_archive(
        name = "com_google_absl",
        urls = ["https://github.com/abseil/abseil-cpp/archive/refs/tags/20230802.1.tar.gz"],
        strip_prefix = "abseil-cpp-20230802.1",
        # sha256 = "6e19ff48bd49b54b55665824a4d62de4c38eb394628484db9917817daca9d2b5",
        patches = [
            "@com_google_ortools//patches:abseil-cpp-20230802.1.patch",
            "@com_google_xls//dependency_support/com_google_absl:ortools_log_macro_redef.patch"
        ],
    )

    # Protobuf depends on Skylib
    # Load bazel skylib as per
    # https://github.com/bazelbuild/bazel-skylib/releases
    http_archive(
        name = "bazel_skylib",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.3.0/bazel-skylib-1.3.0.tar.gz",
            "https://github.com/bazelbuild/bazel-skylib/releases/download/1.3.0/bazel-skylib-1.3.0.tar.gz",
        ],
        sha256 = "74d544d96f4a5bb630d465ca8bbcfe231e3594e5aae57e1edbf17a6eb3ca2506",
    )

    http_archive(
        name = "boringssl",
        # Commit date: 2023-09-25
        # Note for updating: we need to use a commit from the main-with-bazel branch.
        strip_prefix = "boringssl-50132857d4724991db6de99dc272acd223ed52df",
        sha256 = "73370e90e50b61f6485b0fd0034f4e4297764f03faceb3e9d97b6e2e27b915c0",
        urls = ["https://github.com/google/boringssl/archive/50132857d4724991db6de99dc272acd223ed52df.zip"],
    )

    # Commit on 2023-02-09
    http_archive(
        name = "pybind11_bazel",
        strip_prefix = "pybind11_bazel-fc56ce8a8b51e3dd941139d329b63ccfea1d304b",
        urls = ["https://github.com/pybind/pybind11_bazel/archive/fc56ce8a8b51e3dd941139d329b63ccfea1d304b.tar.gz"],
        sha256 = "150e2105f9243c445d48f3820b5e4e828ba16c41f91ab424deae1fa81d2d7ac6",
    )

    http_archive(
        name = "six_archive",
        build_file_content = """py_library(
            name = "six",
            visibility = ["//visibility:public"],
            srcs = glob(["*.py"])
        )""",
        sha256 = "105f8d68616f8248e24bf0e9372ef04d3cc10104f1980f54d57b2ce73a5ad56a",
        strip_prefix = "six-1.10.0",
        urls = [
            "https://mirror.bazel.build/pypi.python.org/packages/source/s/six/six-1.10.0.tar.gz",
            "https://pypi.python.org/packages/source/s/six/six-1.10.0.tar.gz",
        ],
    )

    # Version release tag 2023-09-19
    http_archive(
        name = "com_google_absl_py",
        strip_prefix = "abseil-py-2.0.0",
        urls = ["https://github.com/abseil/abseil-py/archive/refs/tags/v2.0.0.tar.gz"],
        sha256 = "2ab7ce101db02d7a1de48f8157cbd978f00a19bad44828fd213aa69fe352497d",
    )

    http_archive(
        name = "com_googlesource_code_re2",
        sha256 = "4e6593ac3c71de1c0f322735bc8b0492a72f66ffccfad76e259fa21c41d27d8a",
        strip_prefix = "re2-2023-11-01",
        urls = [
            "https://github.com/google/re2/archive/refs/tags/2023-11-01.tar.gz",
            "https://storage.googleapis.com/mirror.tensorflow.org/github.com/google/re2/archive/refs/tags/2023-06-01.tar.gz",
        ],
    )

    # Released on 2022-12-27, current as of 2023-09-27
    # https://github.com/bazelbuild/rules_proto/releases/tag/5.3.0-21.7
    http_archive(
        name = "rules_proto",
        sha256 = "903af49528dc37ad2adbb744b317da520f133bc1cbbecbdd2a6c546c9ead080b",
        # strip_prefix = "rules_proto-5.3.0-21.7",
        strip_prefix = "rules_proto-6.0.0-rc0",
        urls = [
            "https://github.com/bazelbuild/rules_proto/archive/refs/tags/6.0.0-rc0.tar.gz",
        ],
    )

    # Released on 2023-08-22, current as of 2023-09-26
    # https://github.com/bazelbuild/rules_python/releases/tag/0.25.0
    http_archive(
        name = "rules_python",
        # sha256 = "5868e73107a8e85d8f323806e60cad7283f34b32163ea6ff1020cf27abef6036",
        strip_prefix = "rules_python-0.27.0",
        url = "https://github.com/bazelbuild/rules_python/releases/download/0.27.0/rules_python-0.27.0.tar.gz",
    )

    http_archive(
        name = "z3",
        urls = ["https://github.com/Z3Prover/z3/archive/f36f21fa8c64c30c8a775ae6ca4950674bda33ae.tar.gz"],
        sha256 = "7eb543461a5ebb128f37668ea54ee58eb0fd1def9d8590ed9c674acfe9b2efc1",
        strip_prefix = "z3-f36f21fa8c64c30c8a775ae6ca4950674bda33ae",
        build_file = "@com_google_xls//dependency_support/z3:bundled.BUILD.bazel",
    )

    http_archive(
        name = "io_bazel_rules_closure",
        sha256 = "7d206c2383811f378a5ef03f4aacbcf5f47fd8650f6abbc3fa89f3a27dd8b176",
        strip_prefix = "rules_closure-0.10.0",
        urls = [
            "https://github.com/bazelbuild/rules_closure/archive/0.10.0.tar.gz",
        ],
    )

    # zlib is added automatically by gRPC, but the zlib BUILD file used by gRPC
    # does not include all the source code (e.g., gzread is missing) which
    # breaks other users of zlib like iverilog. So add zlib explicitly here with
    # a working BUILD file.
    http_archive(
        name = "zlib",
        sha256 = "f5cc4ab910db99b2bdbba39ebbdc225ffc2aa04b4057bc2817f1b94b6978cfc3",
        strip_prefix = "zlib-1.2.11",
        urls = [
            "https://github.com/madler/zlib/archive/v1.2.11.zip",
        ],
        build_file = "@com_google_xls//dependency_support/zlib:bundled.BUILD.bazel",
    )

    http_archive(
        name = "linenoise",
        # sha256 = "e7dbebca81b518544bea6622d5cc1a2e6347d080793cb0ba134edc66c3822fd5",
        strip_prefix = "linenoise-93b2db9bd4968f76148dd62cdadf050ed50b84b3",
        urls = ["https://github.com/antirez/linenoise/archive/93b2db9bd4968f76148dd62cdadf050ed50b84b3.zip"],
        sha256 = "03eb737b9d6db991d8f47eb4a446ba3a971b970cda18312bc9f9ecbb1e8b6c33",
        build_file = "@com_google_xls//dependency_support/linenoise:bundled.BUILD.bazel",
    )

    # Released on 2023-11-28, current as of 2023-11-29.
    http_archive(
        name = "com_github_grpc_grpc",
        urls = ["https://github.com/grpc/grpc/archive/v1.60.0.tar.gz"],
        sha256 = "437068b8b777d3b339da94d3498f1dc20642ac9bfa76db43abdd522186b1542b",
        strip_prefix = "grpc-1.60.0",
        patches = ["@com_google_xls//dependency_support:grpc_absl_strcat_include.patch"],
        patch_args = ["-p1"],
        repo_mapping = {
            "@local_config_python": "@python39",
            "@system_python": "@python39",
        },
    )

    # Used by xlscc.
    http_archive(
        name = "com_github_hlslibs_ac_types",
        urls = ["https://github.com/hlslibs/ac_types/archive/57d89634cb5034a241754f8f5347803213dabfca.tar.gz"],
        sha256 = "7ab5e2ee4c675ef6895fdd816c32349b3070dc8211b7d412242c66d0c6e8edca",
        strip_prefix = "ac_types-57d89634cb5034a241754f8f5347803213dabfca",
        build_file = "@com_google_xls//dependency_support/com_github_hlslibs_ac_types:bundled.BUILD.bazel",
    )

    http_archive(
        name = "platforms",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/platforms/releases/download/0.0.5/platforms-0.0.5.tar.gz",
            "https://github.com/bazelbuild/platforms/releases/download/0.0.5/platforms-0.0.5.tar.gz",
        ],
        sha256 = "379113459b0feaf6bfbb584a91874c065078aa673222846ac765f86661c27407",
    )

    http_archive(
        name = "com_google_ortools",
        strip_prefix = "or-tools-9.8",
        urls = ["https://github.com/google/or-tools/archive/refs/tags/v9.8.tar.gz"],
        sha256 = "85e10e7acf0a9d9a3b891b9b108f76e252849418c6230daea94ac429af8a4ea4",
        # Removes undesired dependencies like Eigen, BLISS, SCIP
        patches = [
            "@com_google_xls//dependency_support/com_google_ortools:add_logging_prefix.diff",
        #     "@com_google_xls//dependency_support/com_google_ortools:no_glpk.diff",
        #     "@com_google_xls//dependency_support/com_google_ortools:no_scip_or_pdlp.diff",
        #     "@com_google_xls//dependency_support/com_google_ortools:remove_abslstringify.diff",
        ],
    )

    http_archive(
        name = "com_google_benchmark",
        urls = ["https://github.com/google/benchmark/archive/refs/tags/v1.7.0.zip"],
        sha256 = "e0e6a0f2a5e8971198e5d382507bfe8e4be504797d75bb7aec44b5ea368fa100",
        strip_prefix = "benchmark-1.7.0",
    )

    # Updated 2023-11-29; latest version
    FUZZTEST_COMMIT = "231ecb9f4606adee57f1611f18dcee500e927eee"
    http_archive(
        name = "com_google_fuzztest",
        strip_prefix = "fuzztest-" + FUZZTEST_COMMIT,
        url = "https://github.com/google/fuzztest/archive/" + FUZZTEST_COMMIT + ".zip",
        sha256 = "bbdefcec894c18fb22ac8a97ca5a0547cb38f00ad0685c4045154dc381102da8",
    )

    # Updated 2023-2-1
    http_archive(
        name = "rules_license",
        urls = [
            "https://github.com/bazelbuild/rules_license/releases/download/0.0.4/rules_license-0.0.4.tar.gz",
        ],
        sha256 = "6157e1e68378532d0241ecd15d3c45f6e5cfd98fc10846045509fb2a7cc9e381",
    )

    # 2022-09-19
    http_archive(
        name = "com_grail_bazel_compdb",
        sha256 = "a3ff6fe238eec8202270dff75580cba3d604edafb8c3408711e82633c153efa8",
        strip_prefix = "bazel-compilation-database-940cedacdb8a1acbce42093bf67f3a5ca8b265f7",
        urls = ["https://github.com/grailbio/bazel-compilation-database/archive/940cedacdb8a1acbce42093bf67f3a5ca8b265f7.tar.gz"],
    )

    # # 2023-11-13
    http_archive(
        name = "verible",
        sha256 = "08da34659996c6868621b3fa93f5850ed67432133ebdd904b5deb43708f90d5f",
        strip_prefix = "verible-060bde0f3157021d3996e78b463248526579742d",
        urls = ["https://github.com/chipsalliance/verible/archive/060bde0f3157021d3996e78b463248526579742d.tar.gz"],
        patches = ["@com_google_xls//dependency_support/verible:visibility.patch"],
    )

    # 2023-03-17
    # http_archive(
    #     name = "verible",
    #     sha256 = "335673a5c74c9c10ce42e8abb36e89d93502734b54c6a9ff5a269a444dfe46a6",
    #     strip_prefix = "verible-2f16e8418e1b452d4f301a95f8af307079dd8e05",
    #     urls = ["https://github.com/chipsalliance/verible/archive/2f16e8418e1b452d4f301a95f8af307079dd8e05.tar.gz"],
    #     patches = ["@com_google_xls//dependency_support/verible:visibility.patch"],
    # )

    # Same as Verible as of 2023-05-18
    http_archive(
        name = "jsonhpp",
        build_file = "@verible//bazel:jsonhpp.BUILD",
        sha256 = "081ed0f9f89805c2d96335c3acfa993b39a0a5b4b4cef7edb68dd2210a13458c",
        strip_prefix = "json-3.10.2",
        urls = [
            "https://github.com/nlohmann/json/archive/refs/tags/v3.10.2.tar.gz",
        ],
    )
    http_archive(
        name = "com_github_google_rules_install",
        # The installer uses an option -T that is not available on MacOS, but
        # it is benign to leave out.
        # Upstream bug https://github.com/google/bazel_rules_install/issues/31
        patch_args = ["-p1"],
        patches = ["@verible//bazel:installer.patch"],
        sha256 = "880217b21dbd40928bbe3bca3d97bd4de7d70d5383665ec007d7e1aac41d9739",
        strip_prefix = "bazel_rules_install-5ae7c2a8d22de2558098e3872fc7f3f7edc61fb4",
        urls = ["https://github.com/google/bazel_rules_install/archive/5ae7c2a8d22de2558098e3872fc7f3f7edc61fb4.zip"],
    )


    # Version 1.4.7 released on 17.12.2020
    # https://github.com/facebook/zstd/releases/tag/v1.4.7
    # Updated 23.11.2023
    http_archive(
        name = "com_github_facebook_zstd",
        sha256 = "192cbb1274a9672cbcceaf47b5c4e9e59691ca60a357f1d4a8b2dfa2c365d757",
        strip_prefix = "zstd-1.4.7",
        urls = ["https://github.com/facebook/zstd/releases/download/v1.4.7/zstd-1.4.7.tar.gz"],
        build_file = "@//dependency_support/com_github_facebook_zstd:bundled.BUILD.bazel",
    )

    http_archive(
        name = "com_google_protobuf",
        # sha256 = "d0f5f605d0d656007ce6c8b5a82df3037e1d8fe8b121ed42e536f569dec16113",
        strip_prefix = "protobuf-842f56b57a9e825d6cdd6c00947f6c19c18fe466",
        urls = [
            "https://github.com/protocolbuffers/protobuf/archive/842f56b57a9e825d6cdd6c00947f6c19c18fe466.tar.gz",
        ],
        # patches = ["@com_google_xls//dependency_support/],

    )

