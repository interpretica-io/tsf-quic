/* SPDX-License-Identifier: MIT */
/* Copyright (C) 2026 Interpretica, Unipessoal Lda. All rights reserved. */
/** @file
 * @brief Agent-side QUIC / HTTP-3 client over libcurl
 *
 * Written against the libcurl easy interface. The request forces or
 * prefers HTTP/3, discards the body, and reads back which HTTP version
 * answered and the handshake timings with curl_easy_getinfo(). The
 * curl program is not run; libcurl is linked and called here.
 */

#define TE_LGR_USER     "TA QUIC"

#include "te_config.h"

#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "te_defs.h"
#include "te_errno.h"
#include "te_str.h"
#include "te_string.h"
#include "logger_api.h"

#include "ta_quic.h"

/** Discard a response body; this library cares about the transport. */
static size_t
quic_sink(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    UNUSED(ptr);
    UNUSED(userdata);
    return size * nmemb;
}

/** A libcurl easy code turned into a TE status. */
static te_errno
quic_rc(CURLcode code)
{
    switch (code)
    {
        case CURLE_OK:
            return 0;
        case CURLE_URL_MALFORMAT:
        case CURLE_UNSUPPORTED_PROTOCOL:
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        case CURLE_OPERATION_TIMEDOUT:
            return TE_RC(TE_TA_UNIX, TE_ETIMEDOUT);
        default:
            return TE_RC(TE_TA_UNIX, TE_ECOMM);
    }
}

/** libcurl's HTTP-version enum to a normalized TA_QUIC_HTTP_* value. */
static int
quic_http_version(long v)
{
    switch (v)
    {
        case CURL_HTTP_VERSION_1_0: return TA_QUIC_HTTP_1_0;
        case CURL_HTTP_VERSION_1_1: return TA_QUIC_HTTP_1_1;
        case CURL_HTTP_VERSION_2_0: return TA_QUIC_HTTP_2;
        case CURL_HTTP_VERSION_3:   return TA_QUIC_HTTP_3;
        default:                    return 0;
    }
}

/** The ALPN token for a normalized HTTP version. */
static const char *
quic_alpn(int http_version)
{
    switch (http_version)
    {
        case TA_QUIC_HTTP_3: return "h3";
        case TA_QUIC_HTTP_2: return "h2";
        case TA_QUIC_HTTP_1_1:
        case TA_QUIC_HTTP_1_0: return "http/1.1";
        default: return "";
    }
}

/* See description in ta_quic.h */
te_errno
ta_quic_available(te_bool *has_http3, te_string *version)
{
    const curl_version_info_data *info = curl_version_info(CURLVERSION_NOW);

    if (info == NULL)
        return TE_RC(TE_TA_UNIX, TE_EFAIL);

    *has_http3 = (info->features & CURL_VERSION_HTTP3) != 0;
    if (version != NULL && info->version != NULL)
        te_string_append(version, "%s", info->version);

    return 0;
}

/* See description in ta_quic.h */
te_errno
ta_quic_get(const char *url, te_bool h3_only, te_bool head, const char *body,
            int timeout_ms, int *http_version, int *status, te_bool *used_h3,
            te_string *alpn, te_string *tls_version, int *total_us,
            int *connect_us, int *appconnect_us, te_string *effective_url,
            te_string *error)
{
    CURL *handle;
    CURLcode code;
    long raw_version = 0;
    long raw_status = 0;
    curl_off_t total = 0;
    curl_off_t connect = 0;
    curl_off_t appconnect = 0;
    const char *final_url = NULL;
    te_bool have_http3 = false;
    te_errno rc;

    *http_version = 0;
    *status = 0;
    *used_h3 = false;
    *total_us = 0;
    *connect_us = 0;
    *appconnect_us = 0;

    /* No HTTP/3 in this libcurl at all: say so rather than fall back. */
    rc = ta_quic_available(&have_http3, NULL);
    if (rc != 0)
        return rc;
    if (!have_http3)
    {
        ERROR("libcurl on this agent has no HTTP/3 support");
        if (error != NULL)
            te_string_append(error, "libcurl built without HTTP/3");
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    handle = curl_easy_init();
    if (handle == NULL)
    {
        curl_global_cleanup();
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_HTTP_VERSION,
                     h3_only ? (long)CURL_HTTP_VERSION_3ONLY :
                               (long)CURL_HTTP_VERSION_3);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, quic_sink);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, NULL);
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "tsf-quic");
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 0L);
    if (timeout_ms > 0)
        curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, (long)timeout_ms);
    if (head)
        curl_easy_setopt(handle, CURLOPT_NOBODY, 1L);
    if (body != NULL)
    {
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, body);
        curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, (long)strlen(body));
    }

    code = curl_easy_perform(handle);

    /* The info is read whatever the outcome - a failed h3-only attempt
     * still has a version and timings worth logging. */
    curl_easy_getinfo(handle, CURLINFO_HTTP_VERSION, &raw_version);
    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &raw_status);
    curl_easy_getinfo(handle, CURLINFO_TOTAL_TIME_T, &total);
    curl_easy_getinfo(handle, CURLINFO_CONNECT_TIME_T, &connect);
    curl_easy_getinfo(handle, CURLINFO_APPCONNECT_TIME_T, &appconnect);
    curl_easy_getinfo(handle, CURLINFO_EFFECTIVE_URL, &final_url);

    *http_version = quic_http_version(raw_version);
    *status = (int)raw_status;
    *used_h3 = (*http_version == TA_QUIC_HTTP_3);
    *total_us = (int)total;
    *connect_us = (int)connect;
    *appconnect_us = (int)appconnect;

    if (alpn != NULL)
        te_string_append(alpn, "%s", quic_alpn(*http_version));
    /* QUIC mandates TLS 1.3; libcurl has no stable getinfo for the
     * negotiated TLS protocol version, so it is inferred for HTTP/3 and
     * left empty otherwise rather than guessed. */
    if (tls_version != NULL && *used_h3)
        te_string_append(tls_version, "TLS1.3");
    if (effective_url != NULL && final_url != NULL)
        te_string_append(effective_url, "%s", final_url);

    rc = quic_rc(code);
    if (code != CURLE_OK)
    {
        ERROR("QUIC request to %s failed: %s", url, curl_easy_strerror(code));
        if (error != NULL)
            te_string_append(error, "%s", curl_easy_strerror(code));
    }

    curl_easy_cleanup(handle);
    curl_global_cleanup();

    return rc;
}
