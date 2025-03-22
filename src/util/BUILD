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
        "//visibility:public",
    ],
)

licenses(["notice"])

cc_library(
    name = "cow_ptr",
    hdrs = ["cow_ptr.h"],
)

cc_library(
    name = "debug_string",
    srcs = ["debug_string.cc"],
    hdrs = ["debug_string.h"],
)

cc_library(
    name = "locked_optional",
    hdrs = ["locked_optional.h"],
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
    name = "debug_string_test",
    srcs = ["debug_string_test.cc"],
    deps = [
        ":debug_string",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "locked_optional_test",
    srcs = ["locked_optional_test.cc"],
    deps = [
        ":locked_optional",
        "@googletest//:gtest_main",
    ],
)
