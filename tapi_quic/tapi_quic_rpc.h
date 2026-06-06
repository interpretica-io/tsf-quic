/* SPDX-License-Identifier: MIT */
/* Copyright (C) 2026 Interpretica, Unipessoal Lda. All rights reserved. */
/** @file
 * @brief QUIC TAPI: RPC client wrappers
 *
 * Client wrappers of the quic_* RPCs, see quic_rpc.x.m4. Tests use
 * tapi_quic.h; these are the calls behind it, one per RPC.
 */

#ifndef __TAPI_QUIC_RPC_H__
#define __TAPI_QUIC_RPC_H__

#include "te_defs.h"
#include "te_errno.h"
#include "te_string.h"
#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Does the agent's libcurl have HTTP/3?
 *
 * @param[in]  rpcs         RPC server on the agent.
 * @param[out] has_http3    @c true when HTTP/3 is available.
 * @param[out] version      libcurl version, appended, or @c NULL.
 *
 * @return Status code.
 */
extern te_errno rpc_quic_available(rcf_rpc_server *rpcs, te_bool *has_http3,
                                   te_string *version);

/**
 * Fetch a URL over HTTP/3. See ta_quic.h for the field meanings.
 *
 * @param[in]  rpcs             RPC server on the agent.
 * @param[in]  url              The URL.
 * @param[in]  h3_only          Demand HTTP/3.
 * @param[in]  head             Use HEAD.
 * @param[in]  body             POST body, or @c NULL for GET/HEAD.
 * @param[in]  timeout_ms       Request timeout, ms, or @c 0.
 * @param[out] http_version     Normalized version used.
 * @param[out] status           HTTP status.
 * @param[out] used_h3          @c true when HTTP/3 answered.
 * @param[out] alpn             ALPN, appended, or @c NULL.
 * @param[out] tls_version      TLS version, appended, or @c NULL.
 * @param[out] total_us         Total time, microseconds, or @c NULL.
 * @param[out] connect_us       Connect time, microseconds, or @c NULL.
 * @param[out] appconnect_us    Handshake time, microseconds, or @c NULL.
 * @param[out] effective_url    Final URL, appended, or @c NULL.
 * @param[out] error            libcurl error text, appended, or @c NULL.
 *
 * @return Status code.
 */
extern te_errno rpc_quic_get(rcf_rpc_server *rpcs, const char *url,
                             te_bool h3_only, te_bool head, const char *body,
                             int timeout_ms, int *http_version, int *status,
                             te_bool *used_h3, te_string *alpn,
                             te_string *tls_version, int *total_us,
                             int *connect_us, int *appconnect_us,
                             te_string *effective_url, te_string *error);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TAPI_QUIC_RPC_H__ */
