/* SPDX-License-Identifier: MIT */
/* Copyright (C) 2026 Interpretica, Unipessoal Lda. All rights reserved. */
/** @file
 * @brief Fetching a URL over HTTP/3 (QUIC) from an agent
 *
 * @defgroup tapi_quic QUIC / HTTP-3 (tapi_quic)
 * @{
 *
 * Driving an HTTP/3 request from a Test Agent, over libcurl in the
 * agent's RPC server (not by running @c curl): fetch a URL preferring
 * or demanding HTTP/3, and read back which HTTP version actually
 * answered, the status, the ALPN and TLS version, and the connect and
 * handshake timings. The transport is the subject - the response body
 * is discarded.
 *
 * - tapi_quic_available() says whether the agent's libcurl has an
 *   HTTP/3 backend at all (a test skips cleanly when it does not);
 * - tapi_quic_get() makes the request and fills a #tapi_quic_result.
 *
 * @code
 * tapi_quic_result result;
 * tapi_quic_opts opts = TAPI_QUIC_OPTS_INIT;
 *
 * if (!tapi_quic_available(rpcs, NULL))
 *     TEST_SKIP("The agent's libcurl has no HTTP/3");
 *
 * opts.h3_only = true;   // demand HTTP/3, do not fall back
 * CHECK_RC(tapi_quic_get(rpcs, "https://example.org/", &opts, &result));
 * if (!result.used_h3)
 *     TEST_VERDICT("the endpoint did not answer over HTTP/3");
 * RING("h3 in %d us, ALPN %s, %s", result.total_us, result.alpn,
 *      result.tls_version);
 * tapi_quic_result_free(&result);
 * @endcode
 */

#ifndef __TAPI_QUIC_H__
#define __TAPI_QUIC_H__

#include "te_defs.h"
#include "te_errno.h"
#include "te_string.h"
#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Normalized HTTP version: 10, 11, 20 or 30; 0 when unknown. */
enum {
    TAPI_QUIC_HTTP_1_0 = 10,
    TAPI_QUIC_HTTP_1_1 = 11,
    TAPI_QUIC_HTTP_2   = 20,
    TAPI_QUIC_HTTP_3   = 30,
};

/** Options for a request. Zero/NULL means the default. */
typedef struct tapi_quic_opts {
    /** Demand HTTP/3 (no fallback); otherwise prefer it. */
    bool h3_only;
    /** Use HEAD instead of GET. */
    bool head;
    /** POST body, or @c NULL for GET/HEAD. An empty string is a POST. */
    const char *body;
    /** Whole-request timeout, ms, or @c 0 for libcurl's default. */
    int timeout_ms;
} tapi_quic_opts;

/** Initializer for #tapi_quic_opts: a plain preferred-HTTP/3 GET. */
#define TAPI_QUIC_OPTS_INIT { .h3_only = false }

/** The outcome of a request. */
typedef struct tapi_quic_result {
    /** HTTP version that answered, a @c TAPI_QUIC_HTTP_* value. */
    int http_version;
    /** HTTP response code, or @c 0. */
    int status;
    /** @c true when HTTP/3 actually answered. */
    bool used_h3;
    /** ALPN protocol: @c "h3", @c "h2", @c "http/1.1"; never @c NULL. */
    char *alpn;
    /** TLS version (@c "TLS1.3" for HTTP/3, empty otherwise). */
    char *tls_version;
    /** Total time, microseconds. */
    int total_us;
    /** Connect (UDP/TCP) time, microseconds. */
    int connect_us;
    /** Handshake-done (TLS/QUIC) time, microseconds. */
    int appconnect_us;
    /** Final URL after redirects; never @c NULL. */
    char *effective_url;
    /** libcurl's error text on failure, or @c NULL on success. */
    char *error;
} tapi_quic_result;

/**
 * Does the agent's libcurl have an HTTP/3 backend?
 *
 * @param[in]  rpcs     RPC server on the agent.
 * @param[out] version  libcurl's version string, appended, or @c NULL.
 *
 * @return @c true when HTTP/3 is available.
 */
extern bool tapi_quic_available(rcf_rpc_server *rpcs, te_string *version);

/**
 * Fetch a URL, preferring or demanding HTTP/3.
 *
 * @param[in]  rpcs     RPC server on the agent.
 * @param[in]  url      The URL (https).
 * @param[in]  opts     Options, or @c NULL for a preferred-HTTP/3 GET.
 * @param[out] result   The outcome; release with tapi_quic_result_free().
 *
 * @return Status code.
 * @retval TE_ECOMM          The request could not be completed (no HTTP/3
 *                           where it was demanded, no route, ...);
 *                           @p result->error carries libcurl's reason.
 * @retval TE_EOPNOTSUPP     The agent's libcurl has no HTTP/3.
 */
extern te_errno tapi_quic_get(rcf_rpc_server *rpcs, const char *url,
                              const tapi_quic_opts *opts,
                              tapi_quic_result *result);

/**
 * Spell out a normalized HTTP version.
 *
 * @param http_version  A @c TAPI_QUIC_HTTP_* value.
 *
 * @return A static string, never @c NULL.
 */
extern const char *tapi_quic_http_version2str(int http_version);

/**
 * Write a result into the log.
 *
 * @param result        Result.
 */
extern void tapi_quic_result_log(const tapi_quic_result *result);

/**
 * Release a result.
 *
 * @param result        Result.
 */
extern void tapi_quic_result_free(tapi_quic_result *result);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TAPI_QUIC_H__ */

/**@} <!-- END tapi_quic --> */
