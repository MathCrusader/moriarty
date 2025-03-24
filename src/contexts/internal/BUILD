# Copyright 2025 Darcy Best
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

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(
    default_visibility = [
        "//:internal",
    ],
)

licenses(["notice"])

cc_library(
    name = "basic_istream_context",
    srcs = ["basic_istream_context.cc"],
    hdrs = ["basic_istream_context.h"],
    deps = [
        "//src/librarian:io_config",
        "//src/librarian:policies",
    ],
)

cc_library(
    name = "basic_ostream_context",
    srcs = ["basic_ostream_context.cc"],
    hdrs = ["basic_ostream_context.h"],
    deps = [
        "//src/librarian:io_config",
    ],
)

cc_library(
    name = "basic_random_context",
    srcs = ["basic_random_context.cc"],
    hdrs = ["basic_random_context.h"],
    deps = [
        "//src/internal:random_engine",
    ],
)

cc_library(
    name = "generation_orchestration_context",
    srcs = ["generation_orchestration_context.cc"],
    hdrs = ["generation_orchestration_context.h"],
    deps = [
        "//src/internal:generation_handler",
    ],
)

cc_library(
    name = "mutable_values_context",
    srcs = ["mutable_values_context.cc"],
    hdrs = ["mutable_values_context.h"],
    deps = [
        "//src/internal:value_set",
    ],
)

cc_library(
    name = "name_context",
    srcs = ["name_context.cc"],
    hdrs = ["name_context.h"],
    deps = [
    ],
)

cc_library(
    name = "resolve_values_context",
    srcs = ["resolve_values_context.cc"],
    hdrs = ["resolve_values_context.h"],
    deps = [
        "//src/internal:abstract_variable",
        "//src/internal:generation_handler",
        "//src/internal:random_engine",
        "//src/internal:value_set",
        "//src/internal:variable_set",
    ],
)

cc_library(
    name = "variable_random_context",
    srcs = ["variable_random_context.cc"],
    hdrs = ["variable_random_context.h"],
    deps = [
        "//src/internal:abstract_variable",
        "//src/internal:generation_bootstrap",
        "//src/internal:random_engine",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/variables/constraints:size_constraints",
    ],
)

cc_library(
    name = "variable_istream_context",
    srcs = ["variable_istream_context.cc"],
    hdrs = ["variable_istream_context.h"],
    deps = [
        "//src:test_case",
        "//src/internal:abstract_variable",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/librarian:policies",
    ],
)

cc_library(
    name = "variable_ostream_context",
    srcs = ["variable_ostream_context.cc"],
    hdrs = ["variable_ostream_context.h"],
    deps = [
        "//src:test_case",
        "//src/internal:abstract_variable",
        "//src/internal:value_set",
        "//src/internal:variable_set",
    ],
)

cc_library(
    name = "view_only_context",
    srcs = ["view_only_context.cc"],
    hdrs = ["view_only_context.h"],
    deps = [
        "//src/internal:value_set",
        "//src/internal:variable_set",
    ],
)

cc_test(
    name = "basic_istream_context_test",
    srcs = ["basic_istream_context_test.cc"],
    deps = [
        ":basic_istream_context",
        "//src/librarian:io_config",
        "//src/librarian:policies",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "basic_ostream_context_test",
    srcs = ["basic_ostream_context_test.cc"],
    deps = [
        ":basic_ostream_context",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "basic_random_context_test",
    srcs = ["basic_random_context_test.cc"],
    deps = [
        ":basic_random_context",
        "//src/internal:random_engine",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "mutable_values_context_test",
    srcs = ["mutable_values_context_test.cc"],
    deps = [
        "//src/internal:value_set",
        "//src/variables:minteger",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "name_context_test",
    srcs = ["name_context_test.cc"],
    deps = [
        ":name_context",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "resolve_values_context_test",
    srcs = ["resolve_values_context_test.cc"],
    deps = [
        ":resolve_values_context",
        "//src/internal:abstract_variable",
        "//src/internal:generation_handler",
        "//src/internal:random_engine",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/testing:gtest_helpers",
        "//src/variables:minteger",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "variable_istream_context_test",
    srcs = ["variable_istream_context_test.cc"],
    deps = [
        ":variable_istream_context",
        "//src/librarian:policies",
        "//src/testing:gtest_helpers",
        "//src/variables:marray",
        "//src/variables:minteger",
        "//src/variables/constraints:container_constraints",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "variable_ostream_context_test",
    srcs = ["variable_ostream_context_test.cc"],
    deps = [
        ":variable_ostream_context",
        "//src/testing:gtest_helpers",
        "//src/variables:minteger",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "variable_random_context_test",
    srcs = ["variable_random_context_test.cc"],
    deps = [
        ":variable_random_context",
        "//src/internal:random_engine",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/testing:gtest_helpers",
        "//src/variables:minteger",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "view_only_context_test",
    srcs = ["view_only_context_test.cc"],
    deps = [
        ":view_only_context",
        "//src/testing:gtest_helpers",
        "//src/variables:marray",
        "//src/variables:minteger",
        "//src/variables:mstring",
        "@googletest//:gtest_main",
    ],
)
