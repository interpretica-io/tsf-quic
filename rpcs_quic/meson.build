# SPDX-License-Identifier: MIT
# Copyright (C) 2026 Interpretica, Unipessoal Lda. All rights reserved.
#
# External TE library (see TE_EXT_REPO): the quic_* RPCs for the RPC
# server of an agent. List it in the libraries of the ta_rpcprovider
# application together with ta_quic, and add ta_quic/quic_rpc.x.m4 to
# the rpcxdr definitions of both platforms.

sources += files(
    'rpcs_quic.c',
)

te_libs += [
    'rpcserver',
    'ta_quic',
]
