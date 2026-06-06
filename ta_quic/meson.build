# SPDX-License-Identifier: MIT
# Copyright (C) 2026 Interpretica, Unipessoal Lda. All rights reserved.
#
# External TE library (see TE_EXT_REPO): agent-side QUIC / HTTP-3 client
# over libcurl. The agent and its RPC server both link it. The libcurl
# it links must be built with an HTTP/3 backend; ta_quic_available()
# reports whether it was.

headers += files(
    'ta_quic.h',
)

sources += files(
    'ta_quic.c',
)

te_libs += [
    'logger_core',
    'tools',
]

# libcurl ships libcurl.pc almost everywhere; fall back to finding the
# library and header directly if not.
cc = meson.get_compiler('c')
libcurl_dep = dependency('libcurl', required: false)
if not libcurl_dep.found()
    libcurl_dep = cc.find_library('curl', has_headers: 'curl/curl.h')
endif

deps += [
    libcurl_dep,
]
