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

# The core variables that are a part of the Moriarty library.

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(
    default_visibility = [
        "//visibility:public",
    ],
)

licenses(["notice"])

cc_library(
    name = "base_constraints",
    hdrs = ["base_constraints.h"],
    deps = [
        "//src/util:debug_string",
    ],
)

cc_library(
    name = "numeric_constraints",
    srcs = ["numeric_constraints.cc"],
    hdrs = ["numeric_constraints.h"],
    deps = [
        ":base_constraints",
        "//src/internal:range",
    ],
)

cc_library(
    name = "container_constraints",
    srcs = ["container_constraints.cc"],
    hdrs = ["container_constraints.h"],
    deps = [
        ":base_constraints",
        "//src/contexts/librarian:analysis_context",
        "//src/util:debug_string",
        "//src/variables:minteger",
    ],
)

cc_library(
    name = "custom_constraint",
    hdrs = ["custom_constraint.h"],
    deps = [
        ":base_constraints",
        "//src/contexts/librarian:analysis_context",
        "//src/util:debug_string",
    ],
)

cc_library(
    name = "io_constraints",
    srcs = ["io_constraints.cc"],
    hdrs = ["io_constraints.h"],
    deps = [
        ":base_constraints",
        "//src:io_config",
    ],
)

cc_library(
    name = "size_constraints",
    srcs = ["size_constraints.cc"],
    hdrs = ["size_constraints.h"],
    deps = [
        ":base_constraints",
        "//src/librarian:size_property",
    ],
)

cc_library(
    name = "string_constraints",
    srcs = ["string_constraints.cc"],
    hdrs = ["string_constraints.h"],
    deps = [
        ":base_constraints",
        "//src/internal:simple_pattern",
        "@abseil-cpp//absl/strings",
    ],
)

cc_test(
    name = "base_constraints_test",
    srcs = ["base_constraints_test.cc"],
    deps = [
        ":base_constraints",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "container_constraints_test",
    srcs = ["container_constraints_test.cc"],
    deps = [
        ":container_constraints",
        ":numeric_constraints",
        "//src/testing:gtest_helpers",
        "//src/variables:minteger",
        "//src/variables:mstring",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "custom_constraint_test",
    srcs = ["custom_constraint_test.cc"],
    deps = [
        ":custom_constraint",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/variables:minteger",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "io_constraints_test",
    srcs = ["io_constraints_test.cc"],
    deps = [
        ":io_constraints",
        "//src:io_config",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "numeric_constraints_test",
    srcs = ["numeric_constraints_test.cc"],
    deps = [
        ":numeric_constraints",
        "//src/internal:range",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "size_constraints_test",
    srcs = ["size_constraints_test.cc"],
    deps = [
        ":size_constraints",
        "//src/librarian:size_property",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "string_constraints_test",
    srcs = ["string_constraints_test.cc"],
    deps = [
        ":string_constraints",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@googletest//:gtest_main",
    ],
)
