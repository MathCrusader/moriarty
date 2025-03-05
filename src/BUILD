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
    srcs = ["errors.cc"],
    hdrs = ["errors.h"],
    deps = [
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:cord",
    ],
)

cc_library(
    name = "exporter",
    srcs = [
        "exporter.cc",
    ],
    hdrs = [
        "exporter.h",
    ],
    deps = [
        ":errors",
        ":test_case",
        "//src/internal:abstract_variable",
        "//src/internal:status_utils",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/librarian:mvariable",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "generator",
    srcs = [
        "generator.cc",
    ],
    hdrs = [
        "generator.h",
    ],
    deps = [
        ":errors",
        ":property",
        ":scenario",
        ":test_case",
        "//src/internal:analysis_bootstrap",
        "//src/internal:generation_config",
        "//src/internal:random_config",
        "//src/internal:random_engine",
        "//src/internal:status_utils",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/librarian:mvariable",
        "//src/util/status_macro:status_macros",
        "@abseil-cpp//absl/base:nullability",
        "@abseil-cpp//absl/log:check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/types:span",
    ],
)

cc_library(
    name = "importer",
    srcs = ["importer.cc"],
    hdrs = ["importer.h"],
    deps = [
        "//src/internal:abstract_variable",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/librarian:mvariable",
        "//src/util/status_macro:status_macros",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
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
        ":exporter",
        ":generator",
        ":importer",
        ":test_case",
        "//src/contexts/users:export_context",
        "//src/contexts/users:import_context",
        "//src/internal:status_utils",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/librarian:mvariable",
        "//src/util/status_macro:status_macros",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/types:span",
    ],
)

cc_library(
    name = "property",
    srcs = [
        "property.cc",
    ],
    hdrs = [
        "property.h",
    ],
    deps = ["@abseil-cpp//absl/strings"],
)

cc_library(
    name = "scenario",
    srcs = ["scenario.cc"],
    hdrs = [
        "scenario.h",
    ],
    deps = [
        ":property",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "simple_io",
    srcs = ["simple_io.cc"],
    hdrs = ["simple_io.h"],
    deps = [
        ":exporter",
        ":importer",
        "//src/contexts/librarian:printer_context",
        "//src/contexts/librarian:reader_context",
        "//src/internal:abstract_variable",
        "//src/util/status_macro:status_macros",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/types:span",
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
        ":scenario",
        "//src/internal:abstract_variable",
        "//src/internal:generation_bootstrap",
        "//src/internal:random_engine",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/librarian:mvariable",
        "//src/util/status_macro:status_macros",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
    ],
)

cc_test(
    name = "errors_test",
    srcs = ["errors_test.cc"],
    deps = [
        ":errors",
        "//src/testing:status_test_util",
        "//src/util/test_status_macro:status_testutil",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "exporter_test",
    size = "small",
    srcs = [
        "exporter_test.cc",
    ],
    deps = [
        ":errors",
        ":exporter",
        ":test_case",
        "//src/internal:abstract_variable",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/testing:status_test_util",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables:minteger",
        "@abseil-cpp//absl/algorithm:container",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "generator_test",
    size = "small",
    srcs = [
        "generator_test.cc",
    ],
    deps = [
        ":errors",
        ":generator",
        ":scenario",
        ":test_case",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/librarian:size_property",
        "//src/librarian:test_utils",
        "//src/testing:generator_test_util",
        "//src/testing:mtest_type",
        "//src/testing:random_test_util",
        "//src/testing:status_test_util",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables:minteger",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/types:span",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "importer_test",
    srcs = ["importer_test.cc"],
    deps = [
        ":importer",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/testing:status_test_util",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables:minteger",
        "//src/variables:mstring",
        "@abseil-cpp//absl/status",
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
        ":moriarty",
        ":test_case",
        "//src/testing:exporter_test_util",
        "//src/testing:generator_test_util",
        "//src/testing:importer_test_util",
        "//src/testing:mtest_type",
        "//src/testing:status_test_util",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables:minteger",
        "@abseil-cpp//absl/status",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "simple_io_test",
    srcs = ["simple_io_test.cc"],
    deps = [
        ":exporter",
        ":importer",
        ":simple_io",
        ":test_case",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/testing:exporter_test_util",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables:minteger",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/status",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "scenario_test",
    srcs = ["scenario_test.cc"],
    deps = [
        ":property",
        ":scenario",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_case_test",
    srcs = ["test_case_test.cc"],
    deps = [
        ":scenario",
        ":test_case",
        "//src/internal:random_engine",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/testing:mtest_type",
        "//src/testing:mtest_type2",
        "//src/testing:status_test_util",
        "//src/util/status_macro:status_macros",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables:minteger",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/types:span",
        "@googletest//:gtest_main",
    ],
)
