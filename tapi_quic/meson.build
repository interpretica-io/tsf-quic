# SPDX-License-Identifier: MIT
# Copyright (C) 2026 Interpretica, Unipessoal Lda. All rights reserved.
#
# External TE library (see TE_EXT_REPO): engine-side TAPI for QUIC /
# HTTP-3 requests on agents. Built as a shared library on the engine
# platform. It needs the quic_* RPCs in rpcxdr, so add
# ../ta_quic/quic_rpc.x.m4 to the rpcxdr definitions of the engine
# platform. No tsf-cybersec dependency: this is a functional transport
# TAPI, not an audit.

if get_option('engine')
    build_lib_shared = true
endif
build_lib_static = not get_option('engine')
install_lib = install_dev

headers += files(
    'tapi_quic.h',
    'tapi_quic_rpc.h',
)

sources += files(
    'tapi_quic.c',
    'tapi_quic_rpc.c',
)

te_libs += [
    'tapi',
    'tapi_rpc',
    'rcfrpc',
    'confapi',
    'rcfapi',
    'rpcxdr',
    'logger_core',
    'tools',
]
