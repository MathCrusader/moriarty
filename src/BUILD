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
        "//visibility:public",
    ],
)

licenses(["notice"])

cc_library(
    name = "errors",
    hdrs = ["errors.h"],
    deps = [
    ],
)

cc_library(
    name = "context",
    srcs = ["context.cc"],
    hdrs = ["context.h"],
    deps = [
        ":io_config",
        "//src/contexts/internal:basic_istream_context",
        "//src/contexts/internal:basic_ostream_context",
        "//src/contexts/internal:basic_random_context",
        "//src/contexts/internal:variable_istream_context",
        "//src/contexts/internal:variable_ostream_context",
        "//src/contexts/internal:variable_random_context",
        "//src/contexts/internal:view_only_context",
        "//src/internal:random_engine",
        "//src/internal:value_set",
        "//src/internal:variable_set",
    ],
)

cc_library(
    name = "io_config",
    hdrs = ["io_config.h"],
)

cc_library(
    name = "moriarty",
    srcs = [
        "moriarty.cc",
    ],
    hdrs = [
        "moriarty.h",
    ],
    deps = [
        ":context",
        ":test_case",
        "//src/internal:analysis_bootstrap",
        "//src/internal:generation_bootstrap",
        "//src/internal:value_set",
        "//src/internal:variable_name_utils",
        "//src/internal:variable_set",
        "//src/librarian:mvariable",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "simple_io",
    srcs = ["simple_io.cc"],
    hdrs = ["simple_io.h"],
    deps = [
        ":context",
        "//src:test_case",
        "//src/variables:minteger",
    ],
)

cc_library(
    name = "test_case",
    srcs = [
        "test_case.cc",
    ],
    hdrs = [
        "test_case.h",
    ],
    deps = [
        "//src/internal:abstract_variable",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/librarian:mvariable",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/strings",
    ],
)

cc_test(
    name = "errors_test",
    srcs = ["errors_test.cc"],
    deps = [
        ":errors",
        "//src/testing:gtest_helpers",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "moriarty_test",
    size = "small",
    srcs = [
        "moriarty_test.cc",
    ],
    deps = [
        ":context",
        ":moriarty",
        ":test_case",
        "//src/testing:gtest_helpers",
        "//src/testing:mtest_type",
        "//src/variables:minteger",
        "//src/variables:mstring",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "simple_io_test",
    srcs = ["simple_io_test.cc"],
    deps = [
        ":context",
        ":io_config",
        ":simple_io",
        ":test_case",
        "//src/testing:gtest_helpers",
        "//src/variables:minteger",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_case_test",
    srcs = ["test_case_test.cc"],
    deps = [
        ":test_case",
        "//src/internal:generation_bootstrap",
        "//src/internal:random_engine",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/testing:gtest_helpers",
        "//src/testing:mtest_type",
        "//src/variables:minteger",
        "@googletest//:gtest_main",
    ],
)
