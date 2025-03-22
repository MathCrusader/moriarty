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

# Several classes in Moriarty must be derived to work. These classes provide simple
# derived classes to be used for testing.

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = [
        "//visibility:public",
    ],
)

licenses(["notice"])

cc_library(
    name = "mtest_type",
    testonly = True,
    srcs = ["mtest_type.cc"],
    hdrs = ["mtest_type.h"],
    deps = [
        "//src/contexts/librarian:analysis_context",
        "//src/contexts/librarian:printer_context",
        "//src/contexts/librarian:reader_context",
        "//src/contexts/librarian:resolver_context",
        "//src/librarian:errors",
        "//src/librarian:mvariable",
        "//src/variables:minteger",
    ],
)

cc_library(
    name = "gtest_helpers",
    testonly = True,
    hdrs = ["gtest_helpers.h"],
    deps = [
        "//src/contexts/internal:mutable_values_context",
        "//src/contexts/librarian:analysis_context",
        "//src/contexts/librarian:printer_context",
        "//src/contexts/librarian:reader_context",
        "//src/internal:abstract_variable",
        "//src/internal:generation_bootstrap",
        "//src/internal:generation_handler",
        "//src/internal:random_engine",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/librarian:errors",
        "//src/librarian:mvariable",
        "@googletest//:gtest",
    ],
)
