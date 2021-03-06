# Description:
#   TensorFlow C++ inference example for labeling images.

package(default_visibility = ["//tensorflow:internal"])

licenses(["notice"])  # Apache 2.0

exports_files(["LICENSE"])

cc_binary(
    name = "ch-tf-label-image-client",
    srcs = [
        "main.cc", "label-client.h", "label-client.cc", "label-image.h", "label-image.cc", "config.h", "config.cc"
    ],
    linkopts = select({
        "//tensorflow:android": [
            "-pie",
            "-landroid",
            "-ljnigraphics",
            "-llog",
            "-lm",
            "-z defs",
            "-s",
            "-Wl,--exclude-libs,ALL",
        ],
        "//conditions:default": ["-lm", "-L/usr/local/lib", "-lch-pal", "-lch-utils", "-lch-cpp-utils", "-lch-protos", "-lglog"],
    }),
    deps = select({
        "//tensorflow:android": [
            # cc:cc_ops is used to include image ops (for label_image)
            # Jpg, gif, and png related code won't be included
            "//tensorflow/cc:cc_ops",
            "//tensorflow/core:android_tensorflow_lib",
        ],
        "//conditions:default": [
            "//tensorflow/cc:cc_ops",
            "//tensorflow/core:framework_internal",
            "//tensorflow/core:tensorflow",
        ],
    }),
)

filegroup(
    name = "all_files",
    srcs = glob(
        ["**/*"],
        exclude = [
            "**/METADATA",
            "**/OWNERS",
            "bin/**",
            "gen/**",
        ],
    ),
    visibility = ["//tensorflow:__subpackages__"],
)
