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
    name = "all_mvariables",
    deps = [
        ":marray",
        ":mgraph",
        ":minteger",
        ":mnone",
        ":mreal",
        ":mstring",
        ":mtuple",
    ],
)

cc_library(
    name = "marray",
    srcs = ["marray.cc"],
    hdrs = ["marray.h"],
    deps = [
        ":minteger",
        "//src/constraints:base_constraints",
        "//src/constraints:container_constraints",
        "//src/constraints:size_constraints",
        "//src/contexts:librarian_context",
        "//src/librarian:errors",
        "//src/librarian:mvariable",
        "//src/librarian:policies",
        "@abseil-cpp//absl/algorithm:container",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "mgraph",
    srcs = ["mgraph.cc"],
    hdrs = ["mgraph.h"],
    deps = [
        ":minteger",
        ":mnone",
        "//src/constraints:base_constraints",
        "//src/constraints:graph_constraints",
        "//src/constraints:size_constraints",
        "//src/contexts:librarian_context",
        "//src/librarian:mvariable",
        "//src/types:graph",
    ],
)

cc_library(
    name = "minteger",
    srcs = ["minteger.cc"],
    hdrs = ["minteger.h"],
    deps = [
        "//src/constraints:base_constraints",
        "//src/constraints:integer_constraints",
        "//src/constraints:numeric_constraints",
        "//src/constraints:size_constraints",
        "//src/contexts:librarian_context",
        "//src/internal:random_engine",
        "//src/internal:range",
        "//src/librarian:errors",
        "//src/librarian:mvariable",
        "//src/librarian:one_of_handler",
        "//src/librarian:policies",
        "//src/librarian:size_property",
        "//src/librarian/util:cow_ptr",
    ],
)

cc_library(
    name = "mnone",
    hdrs = ["mnone.h"],
    deps = [
        "//src/contexts:librarian_context",
        "//src/librarian:errors",
        "//src/librarian:mvariable",
        "//src/types:no_type",
    ],
)

cc_library(
    name = "mreal",
    srcs = ["mreal.cc"],
    hdrs = ["mreal.h"],
    deps = [
        ":minteger",
        "//src/constraints:numeric_constraints",
        "//src/internal:expressions",
        "//src/internal:range",
        "//src/librarian:mvariable",
        "//src/librarian:one_of_handler",
        "//src/librarian:size_property",
        "//src/librarian/util:cow_ptr",
        "//src/librarian/util:debug_string",
        "//src/types:real",
    ],
)

cc_library(
    name = "mstring",
    srcs = ["mstring.cc"],
    hdrs = ["mstring.h"],
    deps = [
        ":minteger",
        "//src/constraints:base_constraints",
        "//src/constraints:container_constraints",
        "//src/constraints:numeric_constraints",
        "//src/constraints:size_constraints",
        "//src/constraints:string_constraints",
        "//src/contexts:librarian_context",
        "//src/internal:random_engine",
        "//src/internal:simple_pattern",
        "//src/librarian:errors",
        "//src/librarian:mvariable",
        "//src/librarian:policies",
        "@abseil-cpp//absl/algorithm:container",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "mtuple",
    srcs = ["mtuple.cc"],
    hdrs = ["mtuple.h"],
    deps = [
        "//src/constraints:container_constraints",
        "//src/contexts:librarian_context",
        "//src/librarian:errors",
        "//src/librarian:mvariable",
        "@abseil-cpp//absl/strings",
    ],
)

cc_test(
    name = "marray_test",
    srcs = ["marray_test.cc"],
    deps = [
        ":marray",
        ":minteger",
        ":mstring",
        ":mtuple",
        "//src:context",
        "//src/constraints:base_constraints",
        "//src/constraints:container_constraints",
        "//src/constraints:numeric_constraints",
        "//src/librarian:mvariable",
        "//src/librarian/testing:gtest_helpers",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "minteger_test",
    size = "small",
    srcs = ["minteger_test.cc"],
    deps = [
        ":minteger",
        "//src:context",
        "//src/constraints:base_constraints",
        "//src/constraints:numeric_constraints",
        "//src/constraints:size_constraints",
        "//src/librarian:size_property",
        "//src/librarian/testing:gtest_helpers",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "mgraph_test",
    srcs = ["mgraph_test.cc"],
    data = glob(["testing/mgraph/*.in"]),
    deps = [
        ":mgraph",
        ":minteger",
        ":mstring",
        "//src/constraints:graph_constraints",
        "//src/librarian/testing:gtest_helpers",
        "//src/types:graph",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "mstring_test",
    srcs = ["mstring_test.cc"],
    deps = [
        ":minteger",
        ":mstring",
        "//src/constraints:container_constraints",
        "//src/constraints:numeric_constraints",
        "//src/constraints:string_constraints",
        "//src/librarian:mvariable",
        "//src/librarian/testing:gtest_helpers",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "mreal_test",
    srcs = ["mreal_test.cc"],
    deps = [
        ":minteger",
        ":mreal",
        "//src:context",
        "//src/constraints:base_constraints",
        "//src/constraints:numeric_constraints",
        "//src/librarian/testing:gtest_helpers",
        "//src/types:real",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "mtuple_test",
    srcs = ["mtuple_test.cc"],
    deps = [
        ":minteger",
        ":mstring",
        ":mtuple",
        "//src/librarian:mvariable",
        "//src/librarian/testing:gtest_helpers",
        "@googletest//:gtest_main",
    ],
)
