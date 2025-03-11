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
    name = "cow_ptr",
    hdrs = [
        "cow_ptr.h",
    ],
)

cc_library(
    name = "mvariable",
    hdrs = [
        "mvariable.h",
    ],
    deps = [
        "//src:errors",
        "//src:property",
        "//src/contexts/librarian:analysis_context",
        "//src/contexts/librarian:assignment_context",
        "//src/contexts/librarian:printer_context",
        "//src/contexts/librarian:reader_context",
        "//src/contexts/librarian:resolver_context",
        "//src/internal:abstract_variable",
        "//src/internal:generation_config",
        "//src/internal:random_engine",
        "//src/internal:status_utils",
        "//src/internal:variable_name_utils",
        "//src/util/status_macro:status_macros",
        "@abseil-cpp//absl/algorithm:container",
        "@abseil-cpp//absl/base:nullability",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "size_property",
    srcs = ["size_property.cc"],
    hdrs = ["size_property.h"],
    deps = [
        "//src/internal:range",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/strings:string_view",
    ],
)

cc_library(
    name = "test_utils",
    testonly = True,
    hdrs = ["test_utils.h"],
    deps = [
        ":mvariable",
        "//src:errors",
        "//src/contexts/librarian:analysis_context",
        "//src/contexts/librarian:printer_context",
        "//src/contexts/librarian:reader_context",
        "//src/internal:abstract_variable",
        "//src/internal:generation_bootstrap",
        "//src/internal:generation_config",
        "//src/internal:random_engine",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/util/status_macro:status_macros",
        "@abseil-cpp//absl/algorithm:container",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "cow_ptr_test",
    srcs = ["cow_ptr_test.cc"],
    deps = [
        ":cow_ptr",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "mvariable_test",
    size = "small",
    srcs = [
        "mvariable_test.cc",
    ],
    deps = [
        ":mvariable",
        ":test_utils",
        "//src:errors",
        "//src:property",
        "//src/internal:abstract_variable",
        "//src/internal:generation_config",
        "//src/internal:random_engine",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/testing:mtest_type",
        "//src/testing:status_test_util",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables:minteger",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/types:span",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "size_property_test",
    srcs = ["size_property_test.cc"],
    deps = [
        ":size_property",
        "//src/internal:range",
        "//src/util/status_macro:status_macros",
        "//src/util/test_status_macro:status_testutil",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@googletest//:gtest_main",
    ],
)
