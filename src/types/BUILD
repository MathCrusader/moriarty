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

# Auxiliary types that are a part of the Moriarty library.

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(
    default_visibility = [
        "//visibility:public",
    ],
)

licenses(["notice"])

cc_library(
    name = "all_types",
    deps = [
        ":graph",
        ":real",
    ],
)

cc_library(
    name = "graph",
    hdrs = ["graph.h"],
    deps = [
    ],
)

cc_library(
    name = "real",
    srcs = ["real.cc"],
    hdrs = ["real.h"],
)

cc_test(
    name = "graph_test",
    srcs = ["graph_test.cc"],
    deps = [
        ":graph",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "real_test",
    srcs = ["real_test.cc"],
    deps = [
        ":real",
        "@googletest//:gtest_main",
    ],
)
