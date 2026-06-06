/* SPDX-License-Identifier: MIT */
/* Copyright (C) 2026 Interpretica, Unipessoal Lda. All rights reserved. */
/** @file
 * @brief QUIC TAPI: RPC client wrappers
 *
 * The rcf_rpc_call() boilerplate behind tapi_quic. The RPCs return
 * te_errno; an RPC transport failure is mapped to TE_ECORRUPTED.
 */

#define TE_LGR_USER     "TAPI QUIC RPC"

#include "te_config.h"

#include <string.h>

#include "te_defs.h"
#include "te_errno.h"
#include "te_string.h"
#include "logger_api.h"
#include "tapi_rpc_internal.h"
#include "tarpc.h"

#include "tapi_quic_rpc.h"

#define CHECK_RPC_ERRNO_UNCHANGED(_func, _var) \
    CHECK_RETVAL_VAR_ERR_COND(_func, _var, false,                    \
                              TE_RC(TE_TAPI, TE_ECORRUPTED), false)

/* Append an RPC string result, when there is one. */
static void
take_string(te_string *dst, const char *src)
{
    if (dst != NULL && src != NULL)
        te_string_append(dst, "%s", src);
}

/* See description in tapi_quic_rpc.h */
te_errno
rpc_quic_available(rcf_rpc_server *rpcs, te_bool *has_http3,
                   te_string *version)
{
    tarpc_quic_available_in in;
    tarpc_quic_available_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rcf_rpc_call(rpcs, "quic_available", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(quic_available, out.retval);
    TAPI_RPC_LOG(rpcs, quic_available, "", "%r http3=%d", out.retval,
                 out.has_http3);

    if (out.retval == 0)
    {
        if (has_http3 != NULL)
            *has_http3 = out.has_http3;
        take_string(version, out.version);
    }
    RETVAL_TE_ERRNO(quic_available, out.retval);
}

/* See description in tapi_quic_rpc.h */
te_errno
rpc_quic_get(rcf_rpc_server *rpcs, const char *url, te_bool h3_only,
             te_bool head, const char *body, int timeout_ms,
             int *http_version, int *status, te_bool *used_h3,
             te_string *alpn, te_string *tls_version, int *total_us,
             int *connect_us, int *appconnect_us, te_string *effective_url,
             te_string *error)
{
    tarpc_quic_get_in in;
    tarpc_quic_get_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.url = (char *)(url != NULL ? url : "");
    in.h3_only = h3_only;
    in.head = head;
    in.has_body = (body != NULL);
    in.body = (char *)(body != NULL ? body : "");
    in.timeout_ms = timeout_ms;

    rcf_rpc_call(rpcs, "quic_get", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(quic_get, out.retval);
    TAPI_RPC_LOG(rpcs, quic_get, "%s%s", "%r ver=%d status=%d h3=%d",
                 url != NULL ? url : "", h3_only ? " [h3-only]" : "",
                 out.retval, out.http_version, out.status, out.used_h3);

    if (http_version != NULL)
        *http_version = out.http_version;
    if (status != NULL)
        *status = out.status;
    if (used_h3 != NULL)
        *used_h3 = out.used_h3;
    if (total_us != NULL)
        *total_us = out.total_us;
    if (connect_us != NULL)
        *connect_us = out.connect_us;
    if (appconnect_us != NULL)
        *appconnect_us = out.appconnect_us;
    /* The transport facts come back whatever the verdict; the caller
     * reads them even when retval is non-zero (a failed h3-only try
     * still has a version and an error worth logging). */
    take_string(alpn, out.alpn);
    take_string(tls_version, out.tls_version);
    take_string(effective_url, out.effective_url);
    take_string(error, out.error);

    RETVAL_TE_ERRNO(quic_get, out.retval);
}
