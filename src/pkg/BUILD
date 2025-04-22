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

# A concise package to export Moriarty's core functionality.

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = [
        "//visibility:public",
    ],
)

licenses(["notice"])

# The core items in Moriarty.
cc_library(
    name = "core_moriarty",
    srcs = ["empty.cc"],
    linkstatic = True,  # Emits a `.a` file.
    deps = [
        "//src:context",
        "//src:moriarty",
        "//src:simple_io",
        "//src:test_case",
        "//src/variables:all_mvariables",
        "//src/variables/constraints:all_mconstraints",
    ],
)

# The core headers in Moriarty.
filegroup(
    name = "core_headers",
    srcs = [
        "//src:moriarty_headers",
        "//src/variables:mvariable_headers",
        "//src/variables/constraints:mconstraint_headers",
    ],
    visibility = ["//visibility:public"],
)
