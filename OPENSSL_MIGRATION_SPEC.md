# GnuTLS to OpenSSL Migration Specification

## Overview
Replacing GnuTLS with OpenSSL in rdesktop-mac to fix TLS handshake compatibility issues with XRDP server.

## Files to Modify

### 1. Makefile
**Changes:**
- Remove GnuTLS include paths
- Remove GnuTLS library links
- Add OpenSSL 3.0 include/lib paths
- Keep: nettle (for crypto), libtasn1 (for ASN.1), gmp (for bignum)

**Before:**
```makefile
-I.../gnutls/.../include -L.../gnutls/.../lib -lgnutls
```

**After:**
```makefile
-I/opt/homebrew/opt/openssl@3.0/include -L/opt/homebrew/opt/openssl@3.0/lib -lssl -lcrypto
```

---

### 2. tcp.c - TLS Session Management

#### Global Variables
**GnuTLS:**
```c
static gnutls_session_t g_tls_session;
static RD_BOOL g_ssl_initialized = False;
```

**OpenSSL:**
```c
static SSL_CTX *g_ssl_ctx = NULL;
static SSL *g_ssl = NULL;
static RD_BOOL g_ssl_initialized = False;
```

#### Initialization (tcp_tls_connect)

**GnuTLS (lines 352-407):**
```c
gnutls_global_init();
gnutls_init(&g_tls_session, GNUTLS_CLIENT);
gnutls_priority_set_direct(g_tls_session, priority, NULL);
gnutls_certificate_allocate_credentials(&xcred);
gnutls_credentials_set(g_tls_session, GNUTLS_CRD_CERTIFICATE, xcred);
gnutls_certificate_set_x509_system_trust(xcred);
gnutls_certificate_set_verify_function(xcred, cert_verify_callback);
gnutls_transport_set_int(g_tls_session, g_sock);
gnutls_handshake_set_timeout(g_tls_session, timeout);
gnutls_handshake(g_tls_session);
```

**OpenSSL:**
```c
SSL_library_init();
SSL_load_error_strings();
OpenSSL_add_all_algorithms();

g_ssl_ctx = SSL_CTX_new(TLS_client_method());
SSL_CTX_set_min_proto_version(g_ssl_ctx, TLS1_2_VERSION);
SSL_CTX_set_default_verify_paths(g_ssl_ctx);
SSL_CTX_set_verify(g_ssl_ctx, SSL_VERIFY_PEER, cert_verify_callback);

g_ssl = SSL_new(g_ssl_ctx);
SSL_set_fd(g_ssl, g_sock);
SSL_connect(g_ssl);
```

#### Send Data (tcp_send)

**GnuTLS (line 132):**
```c
sent = gnutls_record_send(g_tls_session, data, length);
if (gnutls_error_is_fatal(sent)) { error }
```

**OpenSSL:**
```c
sent = SSL_write(g_ssl, data, length);
if (sent <= 0) {
    int err = SSL_get_error(g_ssl, sent);
    if (err != SSL_ERROR_WANT_WRITE) { error }
}
```

#### Receive Data (tcp_recv)

**GnuTLS (lines 205, 223):**
```c
gnutls_record_check_pending(g_tls_session)
rcvd = gnutls_record_recv(g_tls_session, data, length);
if (gnutls_error_is_fatal(rcvd)) { error }
```

**OpenSSL:**
```c
SSL_pending(g_ssl)
rcvd = SSL_read(g_ssl, data, length);
if (rcvd <= 0) {
    int err = SSL_get_error(g_ssl, rcvd);
    if (err != SSL_ERROR_WANT_READ) { error }
}
```

#### Certificate Verification Callback

**GnuTLS (lines 272-344):**
```c
static int cert_verify_callback(gnutls_session_t session)
{
    gnutls_certificate_verify_peers2(session, &status);
    cert_list = gnutls_certificate_get_peers(session, &cert_list_size);
    gnutls_x509_crt_init(&cert);
    gnutls_x509_crt_import(cert, &cert_list[0], GNUTLS_X509_FMT_DER);
    // ... hostname verification
}
```

**OpenSSL:**
```c
static int cert_verify_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
    X509 *cert = X509_STORE_CTX_get_current_cert(ctx);
    SSL *ssl = X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
    // ... hostname verification using X509_check_host()
    return preverify_ok;
}
```

#### Disconnect (tcp_disconnect)

**GnuTLS:**
```c
if (g_ssl_initialized) {
    gnutls_bye(g_tls_session, GNUTLS_SHUT_WR);
    gnutls_deinit(g_tls_session);
}
```

**OpenSSL:**
```c
if (g_ssl) {
    SSL_shutdown(g_ssl);
    SSL_free(g_ssl);
    g_ssl = NULL;
}
if (g_ssl_ctx) {
    SSL_CTX_free(g_ssl_ctx);
    g_ssl_ctx = NULL;
}
```

---

### 3. ssl.h - Type Definitions

**GnuTLS:**
```c
#include <gnutls/x509.h>
#define RDSSL_CERT gnutls_x509_crt_t
```

**OpenSSL:**
```c
#include <openssl/x509.h>
#define RDSSL_CERT X509
```

---

### 4. ssl.c - Certificate Parsing

#### rdssl_cert_read()

**GnuTLS (lines 112-144):**
```c
RDSSL_CERT *rdssl_cert_read(uint8 *data, uint32 len)
{
    gnutls_x509_crt_t *cert = malloc(sizeof(*cert));
    gnutls_x509_crt_init(cert);

    gnutls_datum_t cert_data;
    cert_data.size = len;
    cert_data.data = data;

    gnutls_x509_crt_import(*cert, &cert_data, GNUTLS_X509_FMT_DER);
    return cert;
}
```

**OpenSSL:**
```c
RDSSL_CERT *rdssl_cert_read(uint8 *data, uint32 len)
{
    const unsigned char *p = data;
    X509 *cert = d2i_X509(NULL, &p, len);
    return cert;
}
```

#### rdssl_cert_free()

**GnuTLS:**
```c
void rdssl_cert_free(RDSSL_CERT *cert)
{
    gnutls_x509_crt_deinit(*cert);
    free(cert);
}
```

**OpenSSL:**
```c
void rdssl_cert_free(RDSSL_CERT *cert)
{
    X509_free(cert);
}
```

#### rdssl_cert_to_rkey()

**GnuTLS (lines 170-259):**
```c
RDSSL_RKEY *rdssl_cert_to_rkey(RDSSL_CERT *cert, uint32 *key_len)
{
    gnutls_datum_t m, e;
    gnutls_x509_crt_get_pk_rsa_raw(*cert, &m, &e);

    RDSSL_RKEY *pkey = malloc(sizeof(*pkey));
    rsa_public_key_init(pkey);
    mpz_import(pkey->n, m.size, 1, sizeof(m.data[0]), 0, 0, m.data);
    mpz_import(pkey->e, e.size, 1, sizeof(e.data[0]), 0, 0, e.data);
    rsa_public_key_prepare(pkey);
}
```

**OpenSSL:**
```c
RDSSL_RKEY *rdssl_cert_to_rkey(RDSSL_CERT *cert, uint32 *key_len)
{
    EVP_PKEY *pkey = X509_get_pubkey(cert);
    RSA *rsa = EVP_PKEY_get1_RSA(pkey);

    RDSSL_RKEY *rkey = malloc(sizeof(*rkey));
    rsa_public_key_init(rkey);

    const BIGNUM *n, *e;
    RSA_get0_key(rsa, &n, &e, NULL);

    // Export to GMP mpz_t for compatibility with existing code
    size_t n_len = BN_num_bytes(n);
    uint8_t *n_buf = malloc(n_len);
    BN_bn2bin(n, n_buf);
    mpz_import(rkey->n, n_len, 1, 1, 0, 0, n_buf);

    size_t e_len = BN_num_bytes(e);
    uint8_t *e_buf = malloc(e_len);
    BN_bn2bin(e, e_buf);
    mpz_import(rkey->e, e_len, 1, 1, 0, 0, e_buf);

    free(n_buf);
    free(e_buf);

    rsa_public_key_prepare(rkey);
    *key_len = rkey->size;

    RSA_free(rsa);
    EVP_PKEY_free(pkey);

    return rkey;
}
```

#### rdssl_cert_print_fp()

**GnuTLS:**
```c
int rdssl_cert_print_fp(FILE *fp, RDSSL_CERT *cert)
{
    gnutls_datum_t cinfo;
    gnutls_x509_crt_print(*cert, GNUTLS_CRT_PRINT_ONELINE, &cinfo);
    fprintf(fp, "\t%s\n", cinfo.data);
    gnutls_free(cinfo.data);
}
```

**OpenSSL:**
```c
int rdssl_cert_print_fp(FILE *fp, RDSSL_CERT *cert)
{
    X509_NAME *subject = X509_get_subject_name(cert);
    char *subj_str = X509_NAME_oneline(subject, NULL, 0);
    fprintf(fp, "\t%s\n", subj_str);
    OPENSSL_free(subj_str);
    return 0;
}
```

---

## Build Dependencies

**Remove:**
- gnutls (3.8.10)
- libidn2 (2.3.8)
- p11-kit (0.25.5)

**Keep:**
- nettle (3.10.2) - SHA1, MD5, RC4, HMAC
- libtasn1 (4.20.0) - ASN.1 parsing
- gmp - BigNum operations

**Add:**
- openssl@3.0 - TLS and X.509

---

## Error Handling Comparison

| Operation | GnuTLS | OpenSSL |
|-----------|--------|---------|
| Fatal error check | `gnutls_error_is_fatal(err)` | `SSL_get_error() != SSL_ERROR_WANT_*` |
| Get error string | `gnutls_strerror(err)` | `ERR_error_string(ERR_get_error())` |
| Want read | `err == GNUTLS_E_AGAIN` | `SSL_get_error() == SSL_ERROR_WANT_READ` |
| Want write | `err == GNUTLS_E_AGAIN` | `SSL_get_error() == SSL_ERROR_WANT_WRITE` |

---

## Priority String Mapping

**GnuTLS:**
```c
priority = "NORMAL:-VERS-SSL3.0:-VERS-TLS1.0:-VERS-TLS1.1";
```

**OpenSSL:**
```c
SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
// This automatically disables SSL3.0, TLS1.0, TLS1.1
```

---

## Testing Checklist

- [ ] TLS 1.2 handshake with XRDP server
- [ ] Certificate verification
- [ ] Data send/receive
- [ ] Connection shutdown
- [ ] Error handling
- [ ] Memory leak check with valgrind
