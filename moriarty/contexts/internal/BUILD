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
    name = "arg_context",
    srcs = ["arg_context.cc"],
    hdrs = ["arg_context.h"],
    deps = [
        "//moriarty/librarian:errors",
        "//moriarty/librarian:policies",
        "//moriarty/librarian/util:ref",
    ],
)


cc_library(
    name = "basic_istream_context",
    srcs = ["basic_istream_context.cc"],
    hdrs = ["basic_istream_context.h"],
    deps = [
        "//moriarty/internal:value_printer",
        "//moriarty/librarian:io_config",
        "//moriarty/librarian:policies",
        "//moriarty/librarian/util:ref",
    ],
)

cc_library(
    name = "basic_ostream_context",
    srcs = ["basic_ostream_context.cc"],
    hdrs = ["basic_ostream_context.h"],
    deps = [
        "//moriarty/librarian:io_config",
        "//moriarty/librarian/util:ref",
    ],
)

cc_library(
    name = "basic_random_context",
    srcs = ["basic_random_context.cc"],
    hdrs = ["basic_random_context.h"],
    deps = [
        "//moriarty/internal:random_engine",
        "//moriarty/librarian/util:ref",
        "//moriarty/types:real",
    ],
)

cc_library(
    name = "generation_orchestration_context",
    srcs = ["generation_orchestration_context.cc"],
    hdrs = ["generation_orchestration_context.h"],
    deps = [
        "//moriarty/internal:generation_handler",
        "//moriarty/librarian/util:ref",
    ],
)

cc_library(
    name = "mutable_values_context",
    srcs = ["mutable_values_context.cc"],
    hdrs = ["mutable_values_context.h"],
    deps = [
        "//moriarty/internal:abstract_variable",
        "//moriarty/internal:value_set",
        "//moriarty/librarian/util:ref",
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
        "//moriarty/internal:abstract_variable",
        "//moriarty/internal:expressions",
        "//moriarty/internal:generation_handler",
        "//moriarty/internal:random_engine",
        "//moriarty/internal:value_set",
        "//moriarty/internal:variable_set",
        "//moriarty/librarian/util:ref",
    ],
)

cc_library(
    name = "variable_random_context",
    srcs = ["variable_random_context.cc"],
    hdrs = ["variable_random_context.h"],
    deps = [
        "//moriarty/constraints:size_constraints",
        "//moriarty/internal:abstract_variable",
        "//moriarty/internal:generation_bootstrap",
        "//moriarty/internal:random_engine",
        "//moriarty/internal:value_set",
        "//moriarty/internal:variable_set",
        "//moriarty/librarian/util:ref",
    ],
)

cc_library(
    name = "variable_istream_context",
    srcs = ["variable_istream_context.cc"],
    hdrs = ["variable_istream_context.h"],
    deps = [
        "//moriarty:test_case",
        "//moriarty/internal:abstract_variable",
        "//moriarty/internal:value_set",
        "//moriarty/internal:variable_set",
        "//moriarty/librarian:io_config",
        "//moriarty/librarian/util:ref",
    ],
)

cc_library(
    name = "variable_ostream_context",
    srcs = ["variable_ostream_context.cc"],
    hdrs = ["variable_ostream_context.h"],
    deps = [
        "//moriarty:test_case",
        "//moriarty/internal:abstract_variable",
        "//moriarty/internal:value_set",
        "//moriarty/internal:variable_set",
        "//moriarty/librarian/util:ref",
    ],
)

cc_library(
    name = "view_only_context",
    srcs = ["view_only_context.cc"],
    hdrs = ["view_only_context.h"],
    deps = [
        "//moriarty/internal:abstract_variable",
        "//moriarty/internal:expressions",
        "//moriarty/internal:range",
        "//moriarty/internal:value_set",
        "//moriarty/internal:variable_set",
        "//moriarty/librarian:errors",
        "//moriarty/librarian/util:ref",
    ],
)

cc_test(
    name = "arg_context_test",
    srcs = ["arg_context_test.cc"],
    deps = [
        ":arg_context",
        "//moriarty/librarian:errors",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "basic_istream_context_test",
    srcs = ["basic_istream_context_test.cc"],
    deps = [
        ":basic_istream_context",
        "//moriarty/librarian:errors",
        "//moriarty/librarian:io_config",
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
        "//moriarty/librarian/testing:gtest_helpers",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "mutable_values_context_test",
    srcs = ["mutable_values_context_test.cc"],
    deps = [
        ":mutable_values_context",
        "//moriarty/internal:value_set",
        "//moriarty/variables:minteger",
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
        "//moriarty/constraints:numeric_constraints",
        "//moriarty/internal:generation_handler",
        "//moriarty/librarian/testing:gtest_helpers",
        "//moriarty/variables:minteger",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "variable_istream_context_test",
    srcs = ["variable_istream_context_test.cc"],
    deps = [
        ":variable_istream_context",
        "//moriarty:test_case",
        "//moriarty/constraints:container_constraints",
        "//moriarty/internal:value_set",
        "//moriarty/librarian:errors",
        "//moriarty/librarian:io_config",
        "//moriarty/librarian:policies",
        "//moriarty/librarian/testing:gtest_helpers",
        "//moriarty/variables:marray",
        "//moriarty/variables:minteger",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "variable_ostream_context_test",
    srcs = ["variable_ostream_context_test.cc"],
    deps = [
        ":variable_ostream_context",
        "//moriarty:test_case",
        "//moriarty/librarian/testing:gtest_helpers",
        "//moriarty/variables:minteger",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "variable_random_context_test",
    srcs = ["variable_random_context_test.cc"],
    deps = [
        ":variable_random_context",
        "//moriarty/librarian/testing:gtest_helpers",
        "//moriarty/variables:minteger",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "view_only_context_test",
    srcs = ["view_only_context_test.cc"],
    deps = [
        ":view_only_context",
        "//moriarty/constraints:container_constraints",
        "//moriarty/librarian/testing:gtest_helpers",
        "//moriarty/variables:marray",
        "//moriarty/variables:minteger",
        "//moriarty/variables:mstring",
        "@googletest//:gtest_main",
    ],
)
