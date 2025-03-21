# Test Macros used in Moriarty.

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(
    default_visibility = [
        "//visibility:public",
    ],
)

cc_library(
    name = "status_testutil",
    testonly = True,
    srcs = ["status_testutil.cc"],
    hdrs = ["status_testutil.h"],
    deps = [
        "//src/util/status_macro:status_macros",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@googletest//:gtest",
    ],
)

licenses(["notice"])

cc_test(
    name = "status_testutil_test",
    srcs = ["status_testutil_test.cc"],
    deps = [
        ":status_testutil",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@googletest//:gtest_main",
    ],
)
