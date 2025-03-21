"""Load dependencies needed to compile p4c as a 3rd-party consumer."""

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def p4c_deps(name = "com_github_p4lang_p4c_extension"):
    """Loads dependencies need to compile p4c.

    Args:
        name: The name of the repository.
    Third party projects can define the target
    @com_github_p4lang_p4c_extension:ir_extension with a `filegroup`
    containing their custom .def files.
    """
    if not native.existing_rule("com_github_p4lang_p4c_extension"):
        # By default, no IR extensions.
        native.new_local_repository(
            name = name,
            path = ".",
            build_file_content = """
filegroup(
    name = "ir_extension",
    srcs = [],
    visibility = ["//visibility:public"],
)
            """,
        )
    if not native.existing_rule("com_github_nelhage_rules_boost"):
        git_repository(
            name = "com_github_nelhage_rules_boost",
            # Newest commit on main branch as of April 11, 2023.
            commit = "e1854fb177ae91dc82ce9534737f5238d2cee9d0",
            remote = "https://github.com/nelhage/rules_boost",
            shallow_since = "1711834277 -0700",
        )
    if not native.existing_rule("com_github_p4lang_p4runtime"):
        # Cannot currently use local_repository due to Bazel limitation,
        # see https://github.com/bazelbuild/bazel/issues/11573.
        #
        # native.local_repository(
        #     name = "com_github_p4lang_p4runtime",
        #     path = "@com_github_p4lang_p4c//:control-plane/p4runtime/proto",
        # )
        #
        # We use git_repository as a workaround; the version used here should
        # ideally be kept in sync with the submodule control-plane/p4runtime.
        git_repository(
            name = "com_github_p4lang_p4runtime",
            remote = "https://github.com/p4lang/p4runtime",
            # Newest commit on main branch as of August 18, 2024.
            commit = "ec4eb5ef70dbcbcbf2f8357a4b2b8c2f218845a5",
            shallow_since = "1680213111 -0700",
            strip_prefix = "proto",
        )
    if not native.existing_rule("com_google_googletest"):
        # Cannot currently use local_repository due to Bazel limitation,
        # see https://github.com/bazelbuild/bazel/issues/11573.
        #
        # local_repository(
        #     name = "com_google_googletest",
        #     path = "@com_github_p4lang_p4c//:test/frameworks/gtest",
        # )
        #
        # We use http_archive as a workaround; the version used here should
        # ideally be kept in sync with the submodule test/frameworks/gtest.
        http_archive(
            name = "com_google_googletest",
            urls = ["https://github.com/google/googletest/archive/refs/tags/v1.13.0.tar.gz"],
            strip_prefix = "googletest-1.13.0",
            sha256 = "ad7fdba11ea011c1d925b3289cf4af2c66a352e18d4c7264392fead75e919363",
        )
    if not native.existing_rule("com_google_absl"):
        http_archive(
            name = "com_google_absl",
            url = "https://github.com/abseil/abseil-cpp/releases/download/20240116.1/abseil-cpp-20240116.1.tar.gz",
            strip_prefix = "abseil-cpp-20240116.1",
            sha256 = "3c743204df78366ad2eaf236d6631d83f6bc928d1705dd0000b872e53b73dc6a",
        )
    if not native.existing_rule("com_google_protobuf"):
        http_archive(
            name = "com_google_protobuf",
            url = "https://github.com/protocolbuffers/protobuf/releases/download/v29.3/protobuf-29.3.tar.gz",
            strip_prefix = "protobuf-29.3",
            integrity = "sha256-AIoRzFb5uWZ5tMKF/QX0bTF9aFvjq1JLKjEL4PutmH4=",
            # sha256 = "d19643d265b978383352b3143f04c0641eea75a75235c111cc01a1350173180e",
        )
    if not native.existing_rule("rules_foreign_cc"):
        http_archive(
            name = "rules_foreign_cc",
            sha256 = "e0f0ebb1a2223c99a904a565e62aa285bf1d1a8aeda22d10ea2127591624866c",
            strip_prefix = "rules_foreign_cc-0.14.0",
            url = "https://github.com/bazel-contrib/rules_foreign_cc/releases/download/0.14.0/rules_foreign_cc-0.14.0.tar.gz",
        )
    if not native.existing_rule("com_github_z3prover_z3"):
        http_archive(
            name = "com_github_z3prover_z3",
            url = "https://github.com/Z3Prover/z3/archive/z3-4.8.12.tar.gz",
            strip_prefix = "z3-z3-4.8.12",
            sha256 = "e3aaefde68b839299cbc988178529535e66048398f7d083b40c69fe0da55f8b7",
            build_file = "@//:bazel/BUILD.z3.bazel",
        )
    if not native.existing_rule("com_github_pantor_inja"):
        http_archive(
            name = "com_github_pantor_inja",
            url = "https://github.com/pantor/inja/archive/refs/tags/v3.4.0.zip",
            strip_prefix = "inja-3.4.0/single_include",
            sha256 = "4ad04d380b8377874c7a097a662c1f67f40da5fb7d3abc3851544f59c3613a20",
            build_file = "@//:bazel/BUILD.inja.bazel",
        )
    if not native.existing_rule("nlohmann_json"):
        http_archive(
            name = "nlohmann_json",
            url = "https://github.com/nlohmann/json/archive/refs/tags/v3.11.2.zip",
            strip_prefix = "json-3.11.2/single_include",
            sha256 = "95651d7d1fcf2e5c3163c3d37df6d6b3e9e5027299e6bd050d157322ceda9ac9",
            build_file = "@//:bazel/BUILD.json.bazel",
        )
