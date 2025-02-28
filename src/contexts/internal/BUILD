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

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(
    default_visibility = [
        "//:internal",
    ],
)

licenses(["notice"])

cc_library(
    name = "basic_istream_context",
    srcs = ["basic_istream_context.cc"],
    hdrs = ["basic_istream_context.h"],
    deps = [
        "//src:io_config",
    ],
)

cc_library(
    name = "basic_ostream_context",
    srcs = ["basic_ostream_context.cc"],
    hdrs = ["basic_ostream_context.h"],
    deps = [
        "//src:io_config",
    ],
)

cc_library(
    name = "mutable_values_context",
    hdrs = ["mutable_values_context.h"],
    deps = [
        "//src/internal:value_set",
    ],
)

cc_test(
    name = "basic_istream_context_test",
    srcs = ["basic_istream_context_test.cc"],
    deps = [
        ":basic_istream_context",
        "//src:io_config",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "basic_ostream_context_test",
    srcs = ["basic_ostream_context_test.cc"],
    deps = [
        ":basic_ostream_context",
        "@googletest//:gtest_main",
    ],
)
