# Copyright 2025 Darcy Best
# Copyright 2024 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

load("@bazel_cc_meta//cc_meta:cc_meta.bzl", "make_cc_meta_deviations", "refresh_cc_meta")
load("@rules_cc//cc:cc_static_library.bzl", "cc_static_library")

package(
    default_visibility = ["//:internal"],
)

package_group(
    name = "internal",
    packages = [
        "//...",
    ],
)

licenses(["notice"])

exports_files(["LICENSE"])

# The core items in Moriarty. You can use this to have a single dependency in a C++ lib file.
cc_static_library(
    name = "core_moriarty",
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//src:actions",
        "//src:context",
        "//src:moriarty",
        "//src:problem",
        "//src:simple_io",
        "//src:test_case",
        "//src/constraints:all_mconstraints",
        "//src/variables:all_mvariables",
    ],
)

make_cc_meta_deviations(
    name = "moriarty_cc_meta_deviations",
    deviations = {
        # gtest and gtest_main export the gtest headers and should always be linked in (for main).
        "@googletest//:gtest": {"exports": [
            "gtest/gtest.h",
            "gmock/gmock.h",
        ]},
        "@googletest//:gtest_main": {
            "alwaysused": True,
            "forward_exports": True,
        },
        "@abseil-cpp//absl/container:flat_hash_map": {"exports": [
            "absl/container/flat_hash_map.h",
        ]},
        "@abseil-cpp//absl/container:flat_hash_set": {"exports": [
            "absl/container/flat_hash_set.h",
        ]},
        "@abseil-cpp//absl/log:absl_check": {"exports": [
            "absl/log/absl_check.h",
        ]},
        "@abseil-cpp//absl/numeric:int128": {"exports": [
            "absl/numeric/int128.h",
        ]},
        "@abseil-cpp//absl/strings": {"exports": [
            "absl/strings/match.h",
            "absl/strings/numbers.h",
            "absl/strings/str_join.h",
        ]},
    },
    visibility = ["//visibility:public"],
)

# ./build/refresh_cc_meta.sh
# bazel run //:refresh_cc_meta
# bazel run @bazel_cc_meta//cc_meta:fix_deps

refresh_cc_meta(
    name = "refresh_cc_meta",
    cc_meta_aspect = "//:build/cc_meta_defs.bzl%moriarty_cc_meta_aspect",
    visibility = ["//visibility:public"],
)
