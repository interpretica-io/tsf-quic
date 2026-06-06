/* SPDX-License-Identifier: MIT */
/* Copyright (C) 2026 Interpretica, Unipessoal Lda. All rights reserved. */
/** @file
 * @brief RPC for QUIC / HTTP-3 requests
 *
 * The RPCs of rpcs_quic, a thin layer over ta_quic, which fetches a URL
 * over HTTP/3 in the RPC server process with libcurl. Add this file to
 * the rpcxdr definitions of the engine platform and of the agent
 * platform:
 *
 *   TE_LIB_PARMS([rpcxdr], [<platform>], [],
 *                [--with-rpcdefs=tarpc_job.x.m4,../ta_quic/quic_rpc.x.m4])
 *
 * No handle survives between calls: each request is whole in and the
 * verdict out. Timings are microseconds.
 */

/* quic_available(): does this agent's libcurl have HTTP/3? */
struct tarpc_quic_available_in {
    struct tarpc_in_arg common;
};

struct tarpc_quic_available_out {
    struct tarpc_out_arg common;

    tarpc_int       retval;
    tarpc_bool      has_http3;
    string          version<>;
};

/*
 * quic_get(): fetch a URL preferring or demanding HTTP/3.
 *
 *   http_version   10/11/20/30, the version that actually answered
 *   used_h3         true when HTTP/3 answered
 *   alpn            "h3" / "h2" / "http/1.1"
 *   tls_version     "TLS1.3" for HTTP/3, empty otherwise
 *   *_us            total / connect / app-connect time, microseconds
 */
struct tarpc_quic_get_in {
    struct tarpc_in_arg common;

    string          url<>;
    tarpc_bool      h3_only;
    tarpc_bool      head;
    string          body<>;          /* "" for GET/HEAD, else a POST body */
    tarpc_bool      has_body;        /* distinguishes an empty POST from none */
    tarpc_int       timeout_ms;
};

struct tarpc_quic_get_out {
    struct tarpc_out_arg common;

    tarpc_int       retval;
    tarpc_int       http_version;
    tarpc_int       status;
    tarpc_bool      used_h3;
    string          alpn<>;
    string          tls_version<>;
    tarpc_int       total_us;
    tarpc_int       connect_us;
    tarpc_int       appconnect_us;
    string          effective_url<>;
    string          error<>;
};

program quic
{
    version ver0
    {
        RPC_DEF(quic_available)
        RPC_DEF(quic_get)
    } = 1;
} = 29;
