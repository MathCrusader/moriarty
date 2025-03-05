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

package(
    default_visibility = [
        "//visibility:public",
    ],
)

licenses(["notice"])

cc_library(
    name = "export_context",
    hdrs = ["export_context.h"],
    deps = [
        "//src:io_config",
        "//src:test_case",
        "//src/contexts/internal:basic_ostream_context",
        "//src/contexts/internal:name_context",
        "//src/contexts/internal:view_only_context",
        "//src/contexts/librarian:printer_context",
        "//src/internal:abstract_variable",
        "//src/internal:value_set",
        "//src/internal:variable_set",
    ],
)

cc_library(
    name = "import_context",
    hdrs = ["import_context.h"],
    deps = [
        "//src:io_config",
        "//src:test_case",
        "//src/contexts/internal:basic_istream_context",
        "//src/contexts/internal:mutable_values_context",
        "//src/contexts/internal:name_context",
        "//src/contexts/internal:view_only_context",
        "//src/contexts/librarian:printer_context",
        "//src/internal:abstract_variable",
        "//src/internal:value_set",
        "//src/internal:variable_set",
    ],
)
