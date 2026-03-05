"""
Defines the cc_meta aspect for Moriarty for developer use to create compile commands, etc.
"""

load("@bazel_cc_meta//cc_meta:cc_meta.bzl", "cc_meta_aspect_factory")

moriarty_cc_meta_aspect = cc_meta_aspect_factory(
    deviations = [Label("@//:moriarty_cc_meta_deviations")],
)
