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
        "//:internal",
    ],
)

licenses(["notice"])

cc_library(
    name = "printer_context",
    hdrs = ["printer_context.h"],
    deps = [
        "//src/contexts/internal:analysis_context",
        "//src/contexts/internal:basic_ostream_context",
        "//src/contexts/internal:variable_name_context",
    ],
)

cc_library(
    name = "reader_context",
    hdrs = ["reader_context.h"],
    deps = [
        "//src:io_config",
        "//src/contexts/internal:analysis_context",
        "//src/contexts/internal:basic_istream_context",
        "//src/contexts/internal:mutable_values_context",
        "//src/contexts/internal:variable_name_context",
    ],
)
