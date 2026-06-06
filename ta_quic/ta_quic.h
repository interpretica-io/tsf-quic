/* SPDX-License-Identifier: MIT */
/* Copyright (C) 2026 Interpretica, Unipessoal Lda. All rights reserved. */
/** @file
 * @brief Agent-side QUIC / HTTP-3 client
 *
 * Fetching a URL over HTTP/3 (QUIC) from an agent, over **libcurl**
 * (@c curl/curl.h, @c -lcurl): the library is linked into the agent and
 * called in-process, the @c curl program is not run. The agent and its
 * RPC server both link this; the RPCs (see quic_rpc.x.m4) are thin
 * wrappers over these functions.
 *
 * QUIC needs a libcurl built with an HTTP/3 backend (ngtcp2+quictls,
 * quiche, msh3...). ta_quic_available() reports whether this libcurl
 * advertises @c CURL_VERSION_HTTP3; without it, a request forced to
 * HTTP/3 fails rather than silently falling back.
 *
 * A request can prefer HTTP/3 (@c CURL_HTTP_VERSION_3, with the usual
 * fallback) or demand it (@c CURL_HTTP_VERSION_3ONLY); either way the
 * reply says which HTTP version actually answered, so a test can assert
 * that HTTP/3 was really used, not merely offered.
 */

#ifndef __TA_QUIC_H__
#define __TA_QUIC_H__

#include "te_defs.h"
#include "te_errno.h"
#include "te_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Normalized HTTP version: 10, 11, 20 or 30; 0 when unknown. */
enum {
    TA_QUIC_HTTP_1_0 = 10,
    TA_QUIC_HTTP_1_1 = 11,
    TA_QUIC_HTTP_2   = 20,
    TA_QUIC_HTTP_3   = 30,
};

/**
 * Does this agent's libcurl have HTTP/3?
 *
 * @param[out] has_http3     @c true when libcurl advertises
 *                           @c CURL_VERSION_HTTP3.
 * @param[out] version       libcurl's version string (e.g.
 *                           @c "8.7.1"), or @c NULL to skip.
 *
 * @return Status code.
 */
extern te_errno ta_quic_available(te_bool *has_http3, te_string *version);

/**
 * Fetch a URL, preferring or demanding HTTP/3.
 *
 * Read-only by default (GET, or HEAD when @p head); an optional
 * @p body makes it a POST. The response body is discarded - this is
 * about the transport, not the content. The negotiated TLS version and
 * ALPN are reported as libcurl exposes them: for an HTTP/3 answer they
 * are @c "TLS1.3" and @c "h3" (QUIC mandates TLS 1.3), since libcurl
 * has no stable getinfo for the negotiated TLS protocol version.
 *
 * @param[in]  url              The URL (https).
 * @param[in]  h3_only          Demand HTTP/3 (@c CURL_HTTP_VERSION_3ONLY);
 *                              otherwise prefer it with fallback.
 * @param[in]  head             Use HEAD instead of GET.
 * @param[in]  body             POST body, or @c NULL for GET/HEAD.
 * @param[in]  timeout_ms       Whole-request timeout, ms, or @c 0 for
 *                              libcurl's default.
 * @param[out] http_version     Normalized HTTP version actually used
 *                              (a @c TA_QUIC_HTTP_* value).
 * @param[out] status           HTTP response code, or @c 0.
 * @param[out] used_h3          @c true when HTTP/3 actually answered.
 * @param[out] alpn             The ALPN protocol (@c "h3", @c "h2",
 *                              @c "http/1.1"), appended.
 * @param[out] tls_version      The TLS version, appended (@c "TLS1.3"
 *                              for HTTP/3; empty otherwise).
 * @param[out] total_us         Total time, microseconds.
 * @param[out] connect_us       TCP/UDP connect time, microseconds.
 * @param[out] appconnect_us    TLS/QUIC handshake-done time, microseconds.
 * @param[out] effective_url    The final URL after redirects, appended.
 * @param[out] error            libcurl's error string on failure,
 *                              appended; empty on success.
 *
 * @return Status code.
 * @retval TE_ECOMM          The request could not be completed (no
 *                           route, no HTTP/3 where it was demanded, ...).
 * @retval TE_EOPNOTSUPP     libcurl here has no HTTP/3 at all.
 */
extern te_errno ta_quic_get(const char *url, te_bool h3_only, te_bool head,
                            const char *body, int timeout_ms,
                            int *http_version, int *status, te_bool *used_h3,
                            te_string *alpn, te_string *tls_version,
                            int *total_us, int *connect_us,
                            int *appconnect_us, te_string *effective_url,
                            te_string *error);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TA_QUIC_H__ */
