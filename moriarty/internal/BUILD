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

# The core parts of the Moriarty library.

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(
    default_visibility = [
        "//:internal",
    ],
)

licenses(["notice"])

cc_library(
    name = "abstract_variable",
    hdrs = [
        "abstract_variable.h",
    ],
    deps = [
        "//moriarty/constraints:constraint_violation",
        "//moriarty/librarian:io_config",
        "//moriarty/librarian/util:ref",
    ],
)

cc_library(
    name = "analysis_bootstrap",
    srcs = ["analysis_bootstrap.cc"],
    hdrs = ["analysis_bootstrap.h"],
    deps = [
        ":abstract_variable",
        ":value_set",
        ":variable_set",
        "//moriarty:context",
        "//moriarty/constraints:constraint_violation",
    ],
)

cc_library(
    name = "combinatorial_coverage",
    srcs = ["combinatorial_coverage.cc"],
    hdrs = ["combinatorial_coverage.h"],
    deps = ["@abseil-cpp//absl/log:absl_check"],
)

cc_library(
    name = "expressions",
    srcs = [
        "expressions.cc",
    ],
    hdrs = [
        "expressions.h",
    ],
    deps = [
        "//moriarty/librarian:errors",
        "@abseil-cpp//absl/numeric:int128",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "generation_handler",
    srcs = ["generation_handler.cc"],
    hdrs = ["generation_handler.h"],
    deps = [
        "//moriarty/librarian:errors",
        "//moriarty/librarian:policies",
        "//moriarty/librarian/util:str_hash",
    ],
)

cc_library(
    name = "random_engine",
    srcs = [
        "random_engine.cc",
    ],
    hdrs = [
        "random_engine.h",
    ],
    deps = [
    ],
)

cc_library(
    name = "range",
    srcs = [
        "range.cc",
    ],
    hdrs = [
        "range.h",
    ],
    deps = [
        ":expressions",
        "//moriarty/types:real",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "simple_pattern",
    srcs = ["simple_pattern.cc"],
    hdrs = ["simple_pattern.h"],
    deps = [
        "//moriarty/internal:expressions",
        "//moriarty/internal:value_printer",
        "//moriarty/librarian:errors",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "value_printer",
    srcs = ["value_printer.cc"],
    hdrs = ["value_printer.h"],
    deps = [
    ],
)

cc_library(
    name = "value_set",
    srcs = ["value_set.cc"],
    hdrs = ["value_set.h"],
    deps = [
        ":abstract_variable",
        "//moriarty/librarian:errors",
        "//moriarty/librarian/util:str_hash",
    ],
)

cc_library(
    name = "variable_set",
    srcs = [
        "variable_set.cc",
    ],
    hdrs = [
        "variable_set.h",
    ],
    deps = [
        ":abstract_variable",
        "//moriarty/librarian:conversions",
        "//moriarty/librarian:errors",
        "//moriarty/librarian/util:str_hash",
    ],
)

cc_test(
    name = "analysis_bootstrap_test",
    srcs = ["analysis_bootstrap_test.cc"],
    deps = [
        ":analysis_bootstrap",
        "//moriarty:context",
        "//moriarty/constraints:equality_constraints",
        "//moriarty/librarian/testing:gtest_helpers",
        "//moriarty/librarian/testing:mtest_type",
        "//moriarty/variables:minteger",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "combinatorial_coverage_test_util",
    testonly = True,
    hdrs = ["combinatorial_coverage_test_util.h"],
    deps = [
        ":combinatorial_coverage",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "combinatorial_coverage_test",
    srcs = ["combinatorial_coverage_test.cc"],
    deps = [
        ":combinatorial_coverage",
        ":combinatorial_coverage_test_util",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "expressions_test",
    size = "small",
    srcs = ["expressions_test.cc"],
    deps = [
        ":expressions",
        "//moriarty/librarian:errors",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "generation_handler_test",
    srcs = ["generation_handler_test.cc"],
    deps = [
        ":generation_handler",
        "//moriarty/librarian:policies",
        "//moriarty/librarian/testing:gtest_helpers",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "random_engine_test",
    size = "small",
    srcs = [
        "random_engine_test.cc",
    ],
    deps = [
        ":random_engine",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "range_test",
    size = "small",
    srcs = [
        "range_test.cc",
    ],
    deps = [
        ":range",
        "//moriarty/types:real",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "simple_pattern_test",
    srcs = ["simple_pattern_test.cc"],
    deps = [
        ":expressions",
        ":random_engine",
        ":simple_pattern",
        ":value_set",
        "//moriarty/librarian:errors",
        "//moriarty/librarian/testing:gtest_helpers",
        "//moriarty/variables:minteger",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "value_printer_test",
    srcs = ["value_printer_test.cc"],
    deps = [
        ":value_printer",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "value_set_test",
    srcs = ["value_set_test.cc"],
    deps = [
        ":value_set",
        "//moriarty/librarian/testing:gtest_helpers",
        "//moriarty/variables:minteger",
        "//moriarty/variables:mstring",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "variable_set_test",
    size = "small",
    srcs = [
        "variable_set_test.cc",
    ],
    deps = [
        ":variable_set",
        "//moriarty/constraints:equality_constraints",
        "//moriarty/librarian/testing:gtest_helpers",
        "//moriarty/librarian/testing:mtest_type",
        "//moriarty/variables:minteger",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "generation_bootstrap",
    srcs = ["generation_bootstrap.cc"],
    hdrs = ["generation_bootstrap.h"],
    deps = [
        ":abstract_variable",
        ":generation_handler",
        ":random_engine",
        ":value_set",
        ":variable_set",
        "//moriarty/constraints:constraint_violation",
        "//moriarty/librarian:errors",
        "//moriarty/librarian:policies",
    ],
)

cc_test(
    name = "generation_bootstrap_test",
    srcs = ["generation_bootstrap_test.cc"],
    deps = [
        ":generation_bootstrap",
        ":random_engine",
        ":value_set",
        ":variable_set",
        "//moriarty/constraints:equality_constraints",
        "//moriarty/constraints:numeric_constraints",
        "//moriarty/librarian:errors",
        "//moriarty/librarian/testing:gtest_helpers",
        "//moriarty/variables:minteger",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "variable_name_utils",
    srcs = ["variable_name_utils.cc"],
    hdrs = ["variable_name_utils.h"],
)

cc_test(
    name = "variable_name_utils_test",
    srcs = ["variable_name_utils_test.cc"],
    deps = [
        ":variable_name_utils",
        "@googletest//:gtest_main",
    ],
)
