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

package(
    default_visibility = [
        "//visibility:public"
    ],
)

licenses(["notice"])

cc_library(
    name = "mvariable",
    hdrs = [
        "mvariable.h",
    ],
    deps = [
        ":io_config",
        ":subvalues",
        "@abseil-cpp//absl/algorithm:container",
        "@abseil-cpp//absl/base:nullability",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "//src:constraint_values",
        "//src:errors",
        "//src:property",
        "//src/internal:abstract_variable",
        "//src/internal:generation_config",
        "//src/internal:random_config",
        "//src/internal:random_engine",
        "//src/internal:status_utils",
        "//src/internal:universe",
        "//src/internal:variable_name_utils",
        "//src/util/status_macro:status_macros",
    ],
)

cc_library(
    name = "test_utils",
    testonly = True,
    hdrs = ["test_utils.h"],
    deps = [
        ":io_config",
        ":mvariable",
        "@googletest//:gtest",
        "@abseil-cpp//absl/algorithm:container",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "//src:errors",
        "//src/internal:abstract_variable",
        "//src/internal:analysis_bootstrap",
        "//src/internal:generation_bootstrap",
        "//src/internal:generation_config",
        "//src/internal:random_engine",
        "//src/internal:universe",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/util/status_macro:status_macros",
    ],
)

cc_test(
    name = "mvariable_test",
    size = "small",
    srcs = [
        "mvariable_test.cc",
    ],
    deps = [
        ":io_config",
        ":mvariable",
        ":test_utils",
        "@googletest//:gtest_main",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/types:span",
        "//src:constraint_values",
        "//src:errors",
        "//src:property",
        "//src/internal:abstract_variable",
        "//src/internal:generation_config",
        "//src/internal:random_engine",
        "//src/internal:universe",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/testing:mtest_type",
        "//src/testing:random_test_util",
        "//src/testing:status_test_util",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables:minteger",
    ],
)

cc_library(
    name = "io_config",
    srcs = ["io_config.cc"],
    hdrs = ["io_config.h"],
    deps = [
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "//src:errors",
    ],
)

cc_library(
    name = "subvalues",
    srcs = ["subvalues.cc"],
    hdrs = ["subvalues.h"],
    deps = [
        "@abseil-cpp//absl/base:nullability",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "//src/internal:abstract_variable",
    ],
)

cc_test(
    name = "io_config_test",
    srcs = ["io_config_test.cc"],
    deps = [
        ":io_config",
        "@googletest//:gtest_main",
        "@abseil-cpp//absl/status",
        "//src:errors",
        "//src/testing:status_test_util",
        "//src/util/test_status_macro:status_testutil",
    ],
)

cc_library(
    name = "size_property",
    srcs = ["size_property.cc"],
    hdrs = ["size_property.h"],
    deps = [
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/strings:string_view",
        "//src/internal:range",
    ],
)

cc_test(
    name = "size_property_test",
    srcs = ["size_property_test.cc"],
    deps = [
        ":size_property",
        "@googletest//:gtest_main",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "//src/internal:range",
        "//src/util/status_macro:status_macros",
        "//src/util/test_status_macro:status_testutil",
    ],
)

cc_test(
    name = "subvalues_test",
    srcs = ["subvalues_test.cc"],
    deps = [
        ":subvalues",
        "@googletest//:gtest_main",
        "@abseil-cpp//absl/status",
        "//src/internal:abstract_variable",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables:minteger",
        "//src/variables:mstring",
    ],
)
