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

package(
    default_visibility = [
        "//visibility:public"
    ],
)

licenses(["notice"])

cc_library(
    name = "combinatorial_generator",
    srcs = ["combinatorial_generator.cc"],
    hdrs = ["combinatorial_generator.h"],
    deps = [
        "@abseil-cpp//absl/base:nullability",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/types:span",
        "//src:generator",
        "//src:test_case",
        "//src/internal:abstract_variable",
        "//src/internal:combinatorial_coverage",
        "//src/internal:random_engine",
        "//src/internal:variable_set",
    ],
)

cc_test(
    name = "combinatorial_generator_test",
    srcs = ["combinatorial_generator_test.cc"],
    deps = [
        ":combinatorial_generator",
        "@googletest//:gtest_main",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/types:span",
        "//src:generator",
        "//src/internal:combinatorial_coverage",
        "//src/internal:combinatorial_coverage_test_util",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/testing:mtest_type",
        "//src/util/test_status_macro:status_testutil",
    ],
)
