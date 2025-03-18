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
    name = "marray",
    hdrs = ["marray.h"],
    deps = [
        ":minteger",
        "//src:errors",
        "//src/contexts/librarian:analysis_context",
        "//src/contexts/librarian:printer_context",
        "//src/contexts/librarian:reader_context",
        "//src/contexts/librarian:resolver_context",
        "//src/librarian:locked_optional",
        "//src/librarian:mvariable",
        "//src/util/status_macro:status_macros",
        "//src/variables/constraints:base_constraints",
        "//src/variables/constraints:container_constraints",
        "//src/variables/constraints:io_constraints",
        "//src/variables/constraints:size_constraints",
        "@abseil-cpp//absl/algorithm:container",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "minteger",
    srcs = [
        "minteger.cc",
    ],
    hdrs = [
        "minteger.h",
    ],
    deps = [
        "//src:errors",
        "//src/contexts/librarian:analysis_context",
        "//src/contexts/librarian:printer_context",
        "//src/contexts/librarian:reader_context",
        "//src/contexts/librarian:resolver_context",
        "//src/internal:random_engine",
        "//src/internal:range",
        "//src/librarian:cow_ptr",
        "//src/librarian:mvariable",
        "//src/librarian:one_of_handler",
        "//src/librarian:size_property",
        "//src/util/status_macro:status_macros",
        "//src/variables/constraints:base_constraints",
        "//src/variables/constraints:numeric_constraints",
        "//src/variables/constraints:size_constraints",
        "@abseil-cpp//absl/algorithm:container",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "mstring",
    srcs = [
        "mstring.cc",
    ],
    hdrs = [
        "mstring.h",
    ],
    deps = [
        ":minteger",
        "//src:errors",
        "//src/contexts/librarian:analysis_context",
        "//src/contexts/librarian:printer_context",
        "//src/contexts/librarian:reader_context",
        "//src/contexts/librarian:resolver_context",
        "//src/internal:random_engine",
        "//src/internal:simple_pattern",
        "//src/librarian:mvariable",
        "//src/util/status_macro:status_macros",
        "//src/variables/constraints:base_constraints",
        "//src/variables/constraints:container_constraints",
        "//src/variables/constraints:numeric_constraints",
        "//src/variables/constraints:size_constraints",
        "//src/variables/constraints:string_constraints",
        "@abseil-cpp//absl/algorithm:container",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "mtuple",
    hdrs = ["mtuple.h"],
    deps = [
        "//src:errors",
        "//src/contexts/librarian:analysis_context",
        "//src/contexts/librarian:printer_context",
        "//src/contexts/librarian:reader_context",
        "//src/contexts/librarian:resolver_context",
        "//src/librarian:locked_optional",
        "//src/librarian:mvariable",
        "//src/util/status_macro:status_macros",
        "//src/variables/constraints:io_constraints",
        "@abseil-cpp//absl/algorithm:container",
        "@abseil-cpp//absl/log:check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
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
        "//src/librarian:mvariable",
        "//src/librarian:test_utils",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables/constraints:base_constraints",
        "//src/variables/constraints:container_constraints",
        "//src/variables/constraints:io_constraints",
        "//src/variables/constraints:numeric_constraints",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/status",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "minteger_test",
    size = "small",
    srcs = [
        "minteger_test.cc",
    ],
    deps = [
        ":minteger",
        "//src/librarian:size_property",
        "//src/librarian:test_utils",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables/constraints:base_constraints",
        "//src/variables/constraints:numeric_constraints",
        "//src/variables/constraints:size_constraints",
        "@abseil-cpp//absl/status",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "mstring_test",
    size = "small",
    timeout = "moderate",
    srcs = [
        "mstring_test.cc",
    ],
    deps = [
        ":minteger",
        ":mstring",
        "//src/librarian:mvariable",
        "//src/librarian:test_utils",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables/constraints:container_constraints",
        "//src/variables/constraints:numeric_constraints",
        "//src/variables/constraints:string_constraints",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/strings",
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
        "//src/librarian:test_utils",
        "//src/util/test_status_macro:status_testutil",
        "@abseil-cpp//absl/status",
        "@googletest//:gtest_main",
    ],
)
