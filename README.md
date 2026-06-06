# tsf-quic

Driving an HTTP/3 (QUIC) request from a Test Agent, packaged as an
external Test Environment (TE) repository (consumed with the
`TE_EXT_REPO` builder directive). It speaks HTTP/3 over a real
library — **libcurl, no Python, nothing spawned** — and reports which
HTTP version actually answered, so a test can assert that HTTP/3 was
really used, not merely offered.

Three libraries:

- `ta_quic` — agent side. An HTTP/3 client over **libcurl**
  (`curl/curl.h`, `-lcurl`): fetch a URL preferring HTTP/3
  (`CURL_HTTP_VERSION_3`, with fallback) or demanding it
  (`CURL_HTTP_VERSION_3ONLY`), and read back the version used, the
  status, the ALPN/TLS version, and the connect and handshake timings
  with `curl_easy_getinfo()`. The body is discarded — the transport is
  the subject. The agent and its RPC server both link it.
- `rpcs_quic` — the `quic_*` RPCs for the RPC server of the agent, thin
  wrappers over `ta_quic`. The request originates on the agent, where
  the endpoint is reachable.
- `tapi_quic` — engine side. `tapi_quic.h` gives a test
  `tapi_quic_available()` and `tapi_quic_get()` returning a structured
  `tapi_quic_result` (HTTP version, status, `used_h3`, ALPN, TLS
  version, timings, final URL, error); `tapi_quic_rpc.h` is the
  one-per-RPC layer beneath. No tsf-cybersec dependency — this is a
  functional transport TAPI.

TE has no HTTP/3 or QUIC client of its own (`tsf-http` is HTTP/1 and /2,
`tsf-tls` is the TLS handshake).

## What it reports

```c
tapi_quic_result result;
tapi_quic_opts opts = TAPI_QUIC_OPTS_INIT;

if (!tapi_quic_available(rpcs, NULL))
    TEST_SKIP("The agent's libcurl has no HTTP/3");

opts.h3_only = true;                 /* demand HTTP/3, no fallback */
CHECK_RC(tapi_quic_get(rpcs, "https://example.org/", &opts, &result));
if (!result.used_h3)
    TEST_VERDICT("the endpoint did not answer over HTTP/3");
RING("h3 status %d in %d us (handshake %d us), ALPN %s",
     result.status, result.total_us, result.appconnect_us, result.alpn);
tapi_quic_result_free(&result);
```

`used_h3` is the honest check: with a preferred-but-not-forced request
libcurl may fall back to HTTP/2 over TCP, and the result then says so
(`http_version` 20, `used_h3` false). `h3_only` makes a non-HTTP/3
endpoint an error instead.

## The library is linked, not a program

`ta_quic` does not run `curl --http3`. It links libcurl and drives the
easy interface in the agent's RPC server process: `CURLOPT_HTTP_VERSION`
to force or prefer HTTP/3, `curl_easy_perform()`, and
`curl_easy_getinfo()` for `CURLINFO_HTTP_VERSION`,
`CURLINFO_RESPONSE_CODE` and the `CURLINFO_*_TIME_T` timings.

The ALPN and TLS version are reported as libcurl exposes them: for an
HTTP/3 answer they are `h3` and `TLS1.3` (QUIC mandates TLS 1.3), since
libcurl has no stable `getinfo` for the negotiated TLS protocol
version — so for an HTTP/2 or HTTP/1.1 answer the ALPN is still derived
from the version used but the TLS version is left empty rather than
guessed.

## Agent host requirements

- **libcurl with an HTTP/3 backend.** HTTP/3 is a build-time option in
  libcurl (ngtcp2+quictls/GnuTLS, quiche, or msh3); a stock distro
  libcurl often does *not* have it. `tapi_quic_available()` reports
  whether `curl_version_info()` advertises `CURL_VERSION_HTTP3`, and a
  test should skip when it does not. `CURL_HTTP_VERSION_3` needs
  libcurl ≥ 7.66, `CURL_HTTP_VERSION_3ONLY` ≥ 7.88.
- The agent needs outbound UDP/443 to the endpoint under test.

## Usage

Declare the repository in an external libraries catalog and pass it to
`dispatcher.sh --external=<catalog.yml>`:

```yaml
repositories:
  - name: tsf_quic
    url: https://github.com/interpretica-io/tsf-quic.git
    ref: <tag>
    libs:
      - ta_quic
      - rpcs_quic
      - tapi_quic
```

In `builder.conf`, bind `tapi_quic` to the engine, list `ta_quic` and
`rpcs_quic` among the RPC server's libraries, and add the RPC
definitions to both platforms:

```
TE_EXT_REPO_USE([tsf_quic], [ta_quic rpcs_quic], [tapi_quic])

TE_LIB_PARMS([rpcxdr], [${TE_HOST}], [],
             [--with-rpcdefs=tarpc_job.x.m4,../ta_quic/quic_rpc.x.m4])
TE_LIB_PARMS([rpcxdr], [${TE_TA_TYPE}], [],
             [--with-rpcdefs=tarpc_job.x.m4,../ta_quic/quic_rpc.x.m4])
```

Then add `tapi_quic` to `te_libs` in the suite's `meson.build`. The RPC
program number is **29** (20–28 are taken by the other tsf agent RPCs);
change it in `quic_rpc.x.m4` if it ever collides.

## What was verified, and what was not

**The C was not compiled here** — no TE toolchain. `ta_quic.c`,
`rpcs_quic.c`, `tapi_quic*.c` and the TE integration (the three
`meson.build`s, `quic_rpc.x.m4`, `TE_EXT_REPO` wiring) were written to
the tsf-usb template but not built.

**The libcurl usage was checked against the real library.** Every
libcurl call, option and info `ta_quic.c` uses — `curl_version_info`
with `CURL_VERSION_HTTP3`, `CURLOPT_HTTP_VERSION` with
`CURL_HTTP_VERSION_3` / `CURL_HTTP_VERSION_3ONLY`, the write-callback
and `CURLOPT_POSTFIELDS*`, `curl_easy_perform`, and
`curl_easy_getinfo` for `CURLINFO_HTTP_VERSION`, `CURLINFO_RESPONSE_CODE`,
`CURLINFO_TOTAL_TIME_T` / `CONNECT_TIME_T` / `APPCONNECT_TIME_T` and
`CURLINFO_EFFECTIVE_URL` — was compiled against the installed libcurl
(8.7.1) headers (`-fsyntax-only`) and type-checks. What was **not**
exercised: a live HTTP/3 request (the macOS libcurl here is not built
with an HTTP/3 backend), the RPC marshalling, and the behaviour of a
real `h3_only` fallback-refusal. The first suite to build tsf-quic
should expect the ordinary first-build fixes.

## Scope

- **The transport, not the content.** The response body is discarded;
  tsf-quic reports which protocol answered and how fast it got there,
  not what came back. For body assertions use `tsf-http`.
- **HTTP/3 must exist on the agent.** tsf-quic does not build or install
  libcurl; it drives whatever libcurl the agent has, and
  `tapi_quic_available()` skips cleanly when that libcurl lacks HTTP/3.
