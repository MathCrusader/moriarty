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
        "//moriarty:context",
        "//moriarty/constraints:constraint_violation",
        "//moriarty/librarian/util:cow_ptr",
    ],
)

cc_library(
    name = "conversions",
    hdrs = ["conversions.h"],
    deps = [
        ":errors",
        "//moriarty/internal:abstract_variable",
    ],
)

cc_library(
    name = "dependencies",
    srcs = ["dependencies.cc"],
    hdrs = ["dependencies.h"],
    deps = [
        "//moriarty/librarian/util:cow_ptr",
    ],
)

cc_library(
    name = "errors",
    hdrs = ["errors.h"],
    deps = [
        "//moriarty/librarian:policies",
    ],
)

cc_library(
    name = "io_config",
    srcs = ["io_config.cc"],
    hdrs = ["io_config.h"],
    deps = [
        "//moriarty/internal:value_printer",
        "//moriarty/librarian:errors",
        "//moriarty/librarian:policies",
        "//moriarty/librarian/util:ref",
    ],
)

cc_library(
    name = "mvariable",
    hdrs = ["mvariable.h"],
    deps = [
        ":constraint_handler",
        ":conversions",
        ":errors",
        ":io_config",
        ":one_of_handler",
        ":policies",
        "//moriarty:context",
        "//moriarty/constraints:constraint_violation",
        "//moriarty/constraints:custom_constraint",
        "//moriarty/constraints:equality_constraints",
        "//moriarty/contexts:librarian_context",
        "//moriarty/contexts/internal:mutable_values_context",
        "//moriarty/contexts/internal:view_only_context",
        "//moriarty/internal:abstract_variable",
        "//moriarty/internal:variable_set",
        "//moriarty/librarian:dependencies",
        "//moriarty/librarian/util:ref",
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
        "//moriarty/internal:range",
        "@abseil-cpp//absl/log:absl_check",
    ],
)

cc_test(
    name = "constraint_handler_test",
    srcs = ["constraint_handler_test.cc"],
    deps = [
        ":constraint_handler",
        "//moriarty:context",
        "//moriarty/constraints:constraint_violation",
        "//moriarty/internal:value_set",
        "//moriarty/internal:variable_set",
        "//moriarty/librarian/testing:gtest_helpers",
        "//moriarty/variables:minteger",
        "//moriarty/variables:mstring",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "dependencies_test",
    srcs = ["dependencies_test.cc"],
    deps = [
        ":dependencies",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "errors_test",
    srcs = ["errors_test.cc"],
    deps = [
        ":errors",
        "//moriarty/librarian:policies",
        "//moriarty/librarian/testing:gtest_helpers",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "io_config_test",
    srcs = ["io_config_test.cc"],
    deps = [
        ":errors",
        ":io_config",
        "//moriarty/librarian/testing:gtest_helpers",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "mvariable_test",
    size = "small",
    srcs = ["mvariable_test.cc"],
    deps = [
        ":mvariable",
        "//moriarty:context",
        "//moriarty/contexts:librarian_context",
        "//moriarty/internal:abstract_variable",
        "//moriarty/internal:generation_handler",
        "//moriarty/internal:value_set",
        "//moriarty/internal:variable_set",
        "//moriarty/librarian/testing:gtest_helpers",
        "//moriarty/librarian/testing:mtest_type",
        "//moriarty/variables:minteger",
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
        "//moriarty/internal:range",
        "@googletest//:gtest_main",
    ],
)
