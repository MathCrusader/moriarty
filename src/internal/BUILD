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
    ],
)

cc_library(
    name = "analysis_bootstrap",
    srcs = ["analysis_bootstrap.cc"],
    hdrs = ["analysis_bootstrap.h"],
    deps = [
        ":value_set",
        ":variable_set",
        "//src/contexts/librarian:analysis_context",
        "@abseil-cpp//absl/status",
    ],
)

cc_library(
    name = "combinatorial_coverage",
    srcs = ["combinatorial_coverage.cc"],
    hdrs = ["combinatorial_coverage.h"],
    deps = [
        "@abseil-cpp//absl/algorithm:container",
        "@abseil-cpp//absl/log:absl_check",
    ],
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
        "@abseil-cpp//absl/numeric:int128",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "generation_config",
    srcs = ["generation_config.cc"],
    hdrs = ["generation_config.h"],
    deps = [
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
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
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
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
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "simple_pattern",
    srcs = ["simple_pattern.cc"],
    hdrs = ["simple_pattern.h"],
    deps = [
        ":random_engine",
        "//src/util/status_macro:status_macros",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "status_utils",
    srcs = ["status_utils.cc"],
    hdrs = ["status_utils.h"],
    deps = [
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "value_set",
    srcs = ["value_set.cc"],
    hdrs = ["value_set.h"],
    deps = [
        ":abstract_variable",
        "//src:errors",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
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
        "//src:errors",
        "//src/librarian:conversions",
        "@abseil-cpp//absl/container:flat_hash_map",
    ],
)

cc_test(
    name = "analysis_bootstrap_test",
    srcs = ["analysis_bootstrap_test.cc"],
    deps = [
        ":analysis_bootstrap",
        "//src/librarian:test_utils",
        "//src/testing:mtest_type",
        "//src/testing:status_test_util",
        "//src/variables:minteger",
        "@abseil-cpp//absl/status",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "combinatorial_coverage_test_util",
    testonly = True,
    hdrs = ["combinatorial_coverage_test_util.h"],
    deps = [
        ":combinatorial_coverage",
        "@abseil-cpp//absl/log:absl_log",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "combinatorial_coverage_test",
    srcs = ["combinatorial_coverage_test.cc"],
    deps = [
        ":combinatorial_coverage",
        ":combinatorial_coverage_test_util",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "expressions_test",
    size = "small",
    srcs = [
        "expressions_test.cc",
    ],
    deps = [
        ":expressions",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "generation_config_test",
    srcs = ["generation_config_test.cc"],
    deps = [
        ":generation_config",
        "//src/util/test_status_macro:status_testutil",
        "@abseil-cpp//absl/status",
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
        "//src/util/status_macro:status_macros",
        "//src/util/test_status_macro:status_testutil",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
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
        ":expressions",
        ":range",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "simple_pattern_test",
    srcs = ["simple_pattern_test.cc"],
    deps = [
        ":random_engine",
        ":simple_pattern",
        "//src/util/test_status_macro:status_testutil",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "status_utils_test",
    srcs = ["status_utils_test.cc"],
    deps = [
        ":status_utils",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "value_set_test",
    srcs = ["value_set_test.cc"],
    deps = [
        ":value_set",
        "//src/testing:status_test_util",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables:marray",
        "//src/variables:minteger",
        "//src/variables:mstring",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
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
        ":generation_bootstrap",
        ":random_engine",
        ":value_set",
        ":variable_set",
        "//src/librarian:test_utils",
        "//src/testing:mtest_type",
        "//src/testing:mtest_type2",
        "//src/testing:status_test_util",
        "//src/util/test_status_macro:status_testutil",
        "@abseil-cpp//absl/status",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "generation_bootstrap",
    srcs = ["generation_bootstrap.cc"],
    hdrs = ["generation_bootstrap.h"],
    deps = [
        ":abstract_variable",
        ":generation_config",
        ":random_engine",
        ":value_set",
        ":variable_set",
        "//src:test_case",
        "//src/contexts/librarian:analysis_context",
        "//src/contexts/librarian:assignment_context",
        "//src/contexts/librarian:resolver_context",
        "//src/util/status_macro:status_macros",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
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
        "//src/librarian:test_utils",
        "//src/testing:status_test_util",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables:minteger",
        "//src/variables:mstring",
        "@abseil-cpp//absl/algorithm:container",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/status",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "variable_name_utils",
    srcs = ["variable_name_utils.cc"],
    hdrs = ["variable_name_utils.h"],
    deps = ["@abseil-cpp//absl/strings"],
)

cc_test(
    name = "variable_name_utils_test",
    srcs = ["variable_name_utils_test.cc"],
    deps = [
        ":variable_name_utils",
        "@googletest//:gtest_main",
    ],
)
