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
    name = "librarian_context",
    srcs = ["librarian_context.cc"],
    hdrs = ["librarian_context.h"],
    deps = [
        "//moriarty/contexts/internal:basic_istream_context",
        "//moriarty/contexts/internal:basic_ostream_context",
        "//moriarty/contexts/internal:basic_random_context",
        "//moriarty/contexts/internal:generation_orchestration_context",
        "//moriarty/contexts/internal:mutable_values_context",
        "//moriarty/contexts/internal:name_context",
        "//moriarty/contexts/internal:resolve_values_context",
        "//moriarty/contexts/internal:view_only_context",
        "//moriarty/internal:generation_handler",
        "//moriarty/internal:random_engine",
        "//moriarty/internal:value_set",
        "//moriarty/internal:variable_set",
        "//moriarty/librarian:io_config",
        "//moriarty/librarian/util:ref",
    ],
)
