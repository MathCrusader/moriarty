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

package(
    default_visibility = ["//:internal"],
)

load("@rules_cc//cc:cc_static_library.bzl", "cc_static_library")

package_group(
    name = "internal",
    packages = [
        "//...",
    ],
)

licenses(["notice"])

exports_files(["LICENSE"])

# The core items in Moriarty. You can use this to have a single dependency in a C++ lib file.
cc_static_library(
    name = "core_moriarty",
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//src:actions",
        "//src:context",
        "//src:moriarty",
        "//src:problem",
        "//src:simple_io",
        "//src:test_case",
        "//src/constraints:all_mconstraints",
        "//src/variables:all_mvariables",
    ],
)
