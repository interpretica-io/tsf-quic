/* SPDX-License-Identifier: MIT */
/* Copyright (C) 2026 Interpretica, Unipessoal Lda. All rights reserved. */
/** @file
 * @brief QUIC RPC server library
 *
 * The quic_* RPCs (see quic_rpc.x.m4) on top of ta_quic.
 * TARPC_FUNC_STATIC() binds an RPC to the function of the same name,
 * so each RPC has a plain C function first and the wrapper after it.
 */

#define TE_LGR_USER     "RPC QUIC"

#include "te_config.h"

#include <string.h>

#include "te_defs.h"
#include "te_errno.h"
#include "te_alloc.h"
#include "te_str.h"
#include "te_string.h"
#include "rpc_server.h"

#include "ta_quic.h"

/* Hand a te_string result over to an RPC string field (never NULL). */
static char *
take(te_string *str)
{
    return str->ptr != NULL ? str->ptr : TE_STRDUP("");
}

static te_errno
quic_available(te_bool *has_http3, char **version)
{
    te_string v = TE_STRING_INIT;
    te_errno rc = ta_quic_available(has_http3, &v);

    *version = take(&v);
    return rc;
}

TARPC_FUNC_STATIC(quic_available, {},
{
    te_bool has_http3 = false;

    MAKE_CALL(out->retval = func(&has_http3, &out->tls_version));
    out->has_http3 = has_http3;
    out->common.errno_changed = false;
})

static te_errno
quic_get(const char *url, te_bool h3_only, te_bool head, const char *body,
         int timeout_ms, int *http_version, int *status, te_bool *used_h3,
         char **alpn, char **tls_version, int *total_us, int *connect_us,
         int *appconnect_us, char **effective_url, char **error)
{
    te_string alpn_s = TE_STRING_INIT;
    te_string tls_s = TE_STRING_INIT;
    te_string url_s = TE_STRING_INIT;
    te_string err_s = TE_STRING_INIT;
    te_errno rc;

    rc = ta_quic_get(url, h3_only, head, body, timeout_ms, http_version,
                     status, used_h3, &alpn_s, &tls_s, total_us, connect_us,
                     appconnect_us, &url_s, &err_s);

    *alpn = take(&alpn_s);
    *tls_version = take(&tls_s);
    *effective_url = take(&url_s);
    *error = take(&err_s);
    return rc;
}

TARPC_FUNC_STATIC(quic_get, {},
{
    int http_version = 0;
    int status = 0;
    te_bool used_h3 = false;
    int total_us = 0;
    int connect_us = 0;
    int appconnect_us = 0;
    /* An empty POST body is distinct from no body (GET/HEAD). */
    const char *body = in->has_body ? in->body : NULL;

    MAKE_CALL(out->retval = func(in->url, in->h3_only, in->head, body,
                                 in->timeout_ms, &http_version, &status,
                                 &used_h3, &out->alpn, &out->tls_version,
                                 &total_us, &connect_us, &appconnect_us,
                                 &out->effective_url, &out->error));
    out->http_version = http_version;
    out->status = status;
    out->used_h3 = used_h3;
    out->total_us = total_us;
    out->connect_us = connect_us;
    out->appconnect_us = appconnect_us;
    out->common.errno_changed = false;
})
