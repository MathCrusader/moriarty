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
    name = "actions",
    srcs = ["actions.cc"],
    hdrs = ["actions.h"],
    deps = [
        ":context",
        ":problem",
        ":test_case",
        "//moriarty/internal:analysis_bootstrap",
        "//moriarty/internal:generation_bootstrap",
        "//moriarty/internal:random_engine",
        "//moriarty/librarian:dependencies",
        "//moriarty/librarian:errors",
    ],
)

cc_library(
    name = "context",
    srcs = ["context.cc"],
    hdrs = ["context.h"],
    deps = [
        ":test_case",
        "//moriarty/contexts/internal:arg_context",
        "//moriarty/contexts/internal:basic_istream_context",
        "//moriarty/contexts/internal:basic_ostream_context",
        "//moriarty/contexts/internal:basic_random_context",
        "//moriarty/contexts/internal:name_context",
        "//moriarty/contexts/internal:variable_istream_context",
        "//moriarty/contexts/internal:variable_ostream_context",
        "//moriarty/contexts/internal:variable_random_context",
        "//moriarty/contexts/internal:view_only_context",
        "//moriarty/internal:random_engine",
        "//moriarty/internal:value_set",
        "//moriarty/internal:variable_set",
        "//moriarty/librarian:io_config",
        "//moriarty/librarian:policies",
        "//moriarty/librarian/util:ref",
    ],
)

cc_library(
    name = "problem",
    srcs = ["problem.cc"],
    hdrs = ["problem.h"],
    deps = [
        ":context",
        ":simple_io",
        "//moriarty/internal:abstract_variable",
        "//moriarty/internal:variable_set",
        "//moriarty/librarian:dependencies",
        "//moriarty/librarian:errors",
    ],
)

cc_library(
    name = "simple_io",
    srcs = ["simple_io.cc"],
    hdrs = ["simple_io.h"],
    deps = [
        ":context",
        "//moriarty:test_case",
        "//moriarty/constraints:numeric_constraints",
        "//moriarty/internal:expressions",
        "//moriarty/internal:value_printer",
        "//moriarty/librarian:dependencies",
        "//moriarty/librarian:errors",
        "//moriarty/variables:minteger",
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
        "//moriarty/internal:abstract_variable",
        "//moriarty/internal:value_set",
        "//moriarty/internal:variable_set",
    ],
)

cc_test(
    name = "actions_test",
    srcs = ["actions_test.cc"],
    deps = [
        ":actions",
        ":problem",
        ":simple_io",
        "//moriarty/constraints:numeric_constraints",
        "//moriarty/librarian:errors",
        "//moriarty/librarian:policies",
        "//moriarty/librarian/testing:gtest_helpers",
        "//moriarty/variables:minteger",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "problem_test",
    srcs = ["problem_test.cc"],
    deps = [
        ":problem",
        "//moriarty/variables:minteger",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "simple_io_test",
    srcs = ["simple_io_test.cc"],
    deps = [
        ":context",
        ":simple_io",
        ":test_case",
        "//moriarty/librarian:errors",
        "//moriarty/librarian:io_config",
        "//moriarty/librarian/testing:gtest_helpers",
        "//moriarty/variables:marray",
        "//moriarty/variables:minteger",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_case_test",
    srcs = ["test_case_test.cc"],
    deps = [
        ":test_case",
        "//moriarty/internal:generation_bootstrap",
        "//moriarty/internal:value_set",
        "//moriarty/internal:variable_set",
        "//moriarty/librarian/testing:gtest_helpers",
        "//moriarty/librarian/testing:mtest_type",
        "//moriarty/variables:minteger",
        "@googletest//:gtest_main",
    ],
)
