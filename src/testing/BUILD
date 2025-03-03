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
    name = "exporter_test_util",
    testonly = True,
    hdrs = [
        "exporter_test_util.h",
    ],
    deps = [
        ":mtest_type",
        "//src:exporter",
        "//src:test_case",
        "//src/internal:value_set",
        "//src/util/status_macro:status_macros",
        "//src/variables:minteger",
        "//src/variables:mstring",
        "@abseil-cpp//absl/status:statusor",
    ],
)

cc_library(
    name = "generator_test_util",
    testonly = True,
    hdrs = [
        "generator_test_util.h",
    ],
    deps = [
        ":mtest_type",
        "//src:generator",
        "//src:test_case",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/variables:minteger",
        "//src/variables:mstring",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/types:span",
    ],
)

cc_library(
    name = "importer_test_util",
    testonly = True,
    hdrs = [
        "importer_test_util.h",
    ],
    deps = [
        "//src:importer",
        "//src/variables:minteger",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/status",
    ],
)

cc_library(
    name = "mtest_type",
    testonly = True,
    srcs = [
        "mtest_type.cc",
    ],
    hdrs = [
        "mtest_type.h",
    ],
    deps = [
        "//src:errors",
        "//src:property",
        "//src/contexts/librarian:analysis_context",
        "//src/contexts/librarian:printer_context",
        "//src/contexts/librarian:reader_context",
        "//src/contexts/librarian:resolver_context",
        "//src/librarian:mvariable",
        "//src/librarian:subvalues",
        "//src/util/status_macro:status_macros",
        "//src/variables:minteger",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "mtest_type2",
    testonly = True,
    srcs = [
        "mtest_type2.cc",
    ],
    hdrs = [
        "mtest_type2.h",
    ],
    deps = [
        "//src:errors",
        "//src:property",
        "//src/contexts/librarian:analysis_context",
        "//src/contexts/librarian:printer_context",
        "//src/contexts/librarian:reader_context",
        "//src/contexts/librarian:resolver_context",
        "//src/librarian:mvariable",
        "//src/librarian:subvalues",
        "//src/util/status_macro:status_macros",
        "//src/variables:minteger",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "random_test_util",
    testonly = True,
    hdrs = [
        "random_test_util.h",
    ],
    deps = [
        "//src/util/test_status_macro:status_testutil",
        "@abseil-cpp//absl/algorithm:container",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@googletest//:gtest",
    ],
)

cc_library(
    name = "status_test_util",
    testonly = True,
    hdrs = [
        "status_test_util.h",
    ],
    deps = [
        "//src:errors",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
    ],
)
