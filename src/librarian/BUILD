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
    name = "constraint_handler",
    hdrs = ["constraint_handler.h"],
    deps = [
        "//src/constraints:constraint_violation",
        "//src/contexts:librarian_context",
        "//src/librarian/util:cow_ptr",
        "//src/librarian/util:ref",
    ],
)

cc_library(
    name = "conversions",
    hdrs = ["conversions.h"],
    deps = [
        ":errors",
    ],
)

cc_library(
    name = "errors",
    hdrs = ["errors.h"],
    deps = [
        "//src/librarian:io_config",
        "//src/librarian:policies",
        "//src/librarian/util:debug_string",
    ],
)

cc_library(
    name = "io_config",
    hdrs = ["io_config.h"],
    deps = [
        "//src/librarian:policies",
        "//src/librarian/util:ref",
    ],
)

cc_library(
    name = "mvariable",
    hdrs = ["mvariable.h"],
    deps = [
        ":constraint_handler",
        ":conversions",
        ":one_of_handler",
        "//src/constraints:custom_constraint",
        "//src/contexts:librarian_context",
        "//src/internal:abstract_variable",
        "//src/internal:generation_handler",
        "//src/internal:random_engine",
        "//src/internal:variable_name_utils",
    ],
)

cc_library(
    name = "one_of_handler",
    hdrs = ["one_of_handler.h"],
)

cc_library(
    name = "policies",
    hdrs = ["policies.h"],
)

cc_library(
    name = "size_property",
    srcs = ["size_property.cc"],
    hdrs = ["size_property.h"],
    deps = [
        "//src/internal:range",
        "@abseil-cpp//absl/log:absl_check",
    ],
)

cc_test(
    name = "constraint_handler_test",
    srcs = ["constraint_handler_test.cc"],
    deps = [
        ":constraint_handler",
        "//src/constraints:constraint_violation",
        "//src/contexts:librarian_context",
        "//src/librarian/testing:gtest_helpers",
        "//src/variables:minteger",
        "//src/variables:mstring",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "errors_test",
    srcs = ["errors_test.cc"],
    deps = [
        ":errors",
        "//src/librarian:policies",
        "//src/librarian/testing:gtest_helpers",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "mvariable_test",
    size = "small",
    srcs = ["mvariable_test.cc"],
    deps = [
        ":mvariable",
        "//src/contexts:librarian_context",
        "//src/internal:abstract_variable",
        "//src/internal:generation_handler",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/librarian/testing:gtest_helpers",
        "//src/librarian/testing:mtest_type",
        "//src/variables:minteger",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "one_of_handler_test",
    srcs = ["one_of_handler_test.cc"],
    deps = [
        ":one_of_handler",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "size_property_test",
    srcs = ["size_property_test.cc"],
    deps = [
        ":size_property",
        "//src/internal:range",
        "@googletest//:gtest_main",
    ],
)
