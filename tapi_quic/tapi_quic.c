/* SPDX-License-Identifier: MIT */
/* Copyright (C) 2026 Interpretica, Unipessoal Lda. All rights reserved. */
/** @file
 * @brief Fetching a URL over HTTP/3 (QUIC) from an agent
 *
 * The engine-side layer over the quic_* RPCs: it asks the agent to make
 * the request over libcurl and collects the outcome into a
 * #tapi_quic_result.
 */

#define TE_LGR_USER     "TAPI QUIC"

#include "te_config.h"

#include <stdlib.h>
#include <string.h>

#include "te_defs.h"
#include "te_errno.h"
#include "te_alloc.h"
#include "te_str.h"
#include "te_string.h"
#include "logger_api.h"

#include "tapi_quic.h"
#include "tapi_quic_rpc.h"

/* See description in tapi_quic.h */
const char *
tapi_quic_http_version2str(int http_version)
{
    switch (http_version)
    {
        case TAPI_QUIC_HTTP_1_0: return "1.0";
        case TAPI_QUIC_HTTP_1_1: return "1.1";
        case TAPI_QUIC_HTTP_2:   return "2";
        case TAPI_QUIC_HTTP_3:   return "3";
        default:                 return "?";
    }
}

/* See description in tapi_quic.h */
bool
tapi_quic_available(rcf_rpc_server *rpcs, te_string *version)
{
    te_bool has_http3 = false;

    if (rpc_quic_available(rpcs, &has_http3, version) != 0)
        return false;

    return has_http3;
}

/* See description in tapi_quic.h */
te_errno
tapi_quic_get(rcf_rpc_server *rpcs, const char *url,
              const tapi_quic_opts *opts, tapi_quic_result *result)
{
    tapi_quic_opts local = TAPI_QUIC_OPTS_INIT;
    te_string alpn = TE_STRING_INIT;
    te_string tls = TE_STRING_INIT;
    te_string effective_url = TE_STRING_INIT;
    te_string error = TE_STRING_INIT;
    te_errno rc;

    memset(result, 0, sizeof(*result));
    if (opts != NULL)
        local = *opts;

    rc = rpc_quic_get(rpcs, url, local.h3_only, local.head, local.body,
                      local.timeout_ms, &result->http_version,
                      &result->status, &result->used_h3, &alpn, &tls,
                      &result->total_us, &result->connect_us,
                      &result->appconnect_us, &effective_url, &error);

    result->alpn = TE_STRDUP(te_string_value(&alpn));
    result->tls_version = TE_STRDUP(te_string_value(&tls));
    result->effective_url = TE_STRDUP(te_string_value(&effective_url));
    if (error.len != 0)
        result->error = TE_STRDUP(te_string_value(&error));

    te_string_free(&alpn);
    te_string_free(&tls);
    te_string_free(&effective_url);
    te_string_free(&error);

    return rc;
}

/* See description in tapi_quic.h */
void
tapi_quic_result_log(const tapi_quic_result *result)
{
    RING("QUIC: HTTP/%s status %d (%s), ALPN %s %s, "
         "connect %d us, handshake %d us, total %d us\n  %s%s%s",
         tapi_quic_http_version2str(result->http_version), result->status,
         result->used_h3 ? "HTTP/3" : "not HTTP/3",
         result->alpn != NULL ? result->alpn : "?",
         result->tls_version != NULL && result->tls_version[0] != '\0' ?
             result->tls_version : "(TLS version not exposed)",
         result->connect_us, result->appconnect_us, result->total_us,
         result->effective_url != NULL ? result->effective_url : "",
         result->error != NULL ? " error: " : "",
         result->error != NULL ? result->error : "");
}

/* See description in tapi_quic.h */
void
tapi_quic_result_free(tapi_quic_result *result)
{
    free(result->alpn);
    free(result->tls_version);
    free(result->effective_url);
    free(result->error);
    memset(result, 0, sizeof(*result));
}
