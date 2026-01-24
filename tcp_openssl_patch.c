/* OpenSSL replacement for GnuTLS functions in tcp.c
 * This file contains the OpenSSL-based replacements to be inserted into tcp.c
 */

// ==== REPLACE INCLUDES (lines 36-39) ====
// OLD:
// // Native macOS SSL - no GNUTLS includes needed
// #include "rdesktop.h"
// #include "macssl.h"
// #include "asn.h"

// NEW:
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include "rdesktop.h"
#include "asn.h"

// ==== REPLACE GLOBALS (lines 68, 79) ====
// OLD:
// static RD_BOOL g_ssl_initialized = False;
// static gnutls_session_t g_tls_session;

// NEW:
static RD_BOOL g_ssl_initialized = False;
static SSL_CTX *g_ssl_ctx = NULL;
static SSL *g_tls_session = NULL;

// ==== REPLACE tcp_send (around line 131-132) ====
// OLD:
//		if (g_ssl_initialized) {
//			sent = gnutls_record_send(g_tls_session, data, length);

// NEW:
		if (g_ssl_initialized && g_tls_session) {
			sent = SSL_write(g_tls_session, data, length);

// ==== REPLACE tcp_recv (around line 222-223) ====
// OLD:
//		if (g_ssl_initialized) {
//			rcvd = gnutls_record_recv(g_tls_session, data, length);

// NEW:
		if (g_ssl_initialized && g_tls_session) {
			rcvd = SSL_read(g_tls_session, data, length);

// ==== REPLACE tcp_recv select check (around line 205) ====
// OLD:
//		if ((!g_ssl_initialized || (gnutls_record_check_pending(g_tls_session) <= 0)) && g_run_ui)

// NEW:
		if ((!g_ssl_initialized || (SSL_pending(g_tls_session) <= 0)) && g_run_ui)

// ==== COMPLETE REPLACEMENT OF tcp_tls_connect() function (lines 343-445) ====
/* Establish a SSL/TLS 1.0 connection */
RD_BOOL
tcp_tls_connect(void)
{
	int ret;
	const SSL_METHOD *method;

	/* Initialize OpenSSL library */
	if (!g_ssl_initialized)
	{
		SSL_library_init();
		SSL_load_error_strings();
		OpenSSL_add_all_algorithms();

		g_ssl_initialized = True;
	}

	/* Create SSL context */
	method = TLS_client_method();  // Use TLS 1.0-1.3
	g_ssl_ctx = SSL_CTX_new(method);
	if (!g_ssl_ctx) {
		logger(Core, Error, "tcp_tls_connect(), Failed to create SSL context");
		goto fail;
	}

	/* Set TLS version if specified */
	if (g_tls_version[0] != 0) {
		if (!strcmp(g_tls_version, "1.0")) {
			SSL_CTX_set_min_proto_version(g_ssl_ctx, TLS1_VERSION);
			SSL_CTX_set_max_proto_version(g_ssl_ctx, TLS1_VERSION);
		} else if (!strcmp(g_tls_version, "1.1")) {
			SSL_CTX_set_min_proto_version(g_ssl_ctx, TLS1_1_VERSION);
			SSL_CTX_set_max_proto_version(g_ssl_ctx, TLS1_1_VERSION);
		} else if (!strcmp(g_tls_version, "1.2")) {
			SSL_CTX_set_min_proto_version(g_ssl_ctx, TLS1_2_VERSION);
			SSL_CTX_set_max_proto_version(g_ssl_ctx, TLS1_2_VERSION);
		} else if (!strcmp(g_tls_version, "1.3")) {
			SSL_CTX_set_min_proto_version(g_ssl_ctx, TLS1_3_VERSION);
			SSL_CTX_set_max_proto_version(g_ssl_ctx, TLS1_3_VERSION);
		} else {
			logger(Core, Error,
			       "tcp_tls_connect(), TLS version should be 1.0, 1.1, 1.2, or 1.3");
			goto fail;
		}
	}

	/* Load system CA certificates */
	if (!SSL_CTX_set_default_verify_paths(g_ssl_ctx)) {
		logger(Core, Warning, "tcp_tls_connect(), Could not load system trust database");
	}

	/* Set verification mode - don't verify for compatibility */
	SSL_CTX_set_verify(g_ssl_ctx, SSL_VERIFY_NONE, NULL);

	/* Create SSL connection */
	g_tls_session = SSL_new(g_ssl_ctx);
	if (!g_tls_session) {
		logger(Core, Error, "tcp_tls_connect(), Failed to create SSL connection");
		goto fail;
	}

	/* Attach socket to SSL */
	if (!SSL_set_fd(g_tls_session, g_sock)) {
		logger(Core, Error, "tcp_tls_connect(), Failed to attach socket to SSL");
		goto fail;
	}

	/* Perform TLS handshake */
	ret = SSL_connect(g_tls_session);
	if (ret != 1) {
		int ssl_error = SSL_get_error(g_tls_session, ret);
		char err_buf[256];
		ERR_error_string_n(ERR_get_error(), err_buf, sizeof(err_buf));

		logger(Core, Error, "tcp_tls_connect(), TLS handshake failed: %s (error code: %d)",
		       err_buf, ssl_error);
		goto fail;
	}

	/* Log connection details */
	logger(Core, Verbose, "TLS Session info: %s", SSL_get_cipher(g_tls_session));

	return True;

fail:
	if (g_tls_session) {
		SSL_free(g_tls_session);
		g_tls_session = NULL;
	}
	if (g_ssl_ctx) {
		SSL_CTX_free(g_ssl_ctx);
		g_ssl_ctx = NULL;
	}
	g_ssl_initialized = False;

	return False;
}

// ==== COMPLETE REPLACEMENT OF tcp_tls_get_server_pubkey() function (lines 448-530) ====
/* Get public key from server of TLS 1.x connection */
STREAM
tcp_tls_get_server_pubkey()
{
	X509 *cert = NULL;
	EVP_PKEY *pkey = NULL;
	RSA *rsa = NULL;
	const BIGNUM *n, *e;
	int pk_size;
	uint8_t pk_data[1024];
	STREAM s = NULL;

	/* Get server certificate */
	cert = SSL_get_peer_certificate(g_tls_session);
	if (!cert) {
		logger(Core, Error, "%s:%s:%d Failed to get peer certificate\n",
		       __FILE__, __func__, __LINE__);
		goto out;
	}

	/* Get public key from certificate */
	pkey = X509_get_pubkey(cert);
	if (!pkey) {
		logger(Core, Error, "%s:%s:%d Failed to get public key from certificate\n",
		       __FILE__, __func__, __LINE__);
		goto out;
	}

	/* Get RSA key */
	rsa = EVP_PKEY_get1_RSA(pkey);
	if (!rsa) {
		logger(Core, Error, "%s:%s:%d Certificate is not RSA\n",
		       __FILE__, __func__, __LINE__);
		goto out;
	}

	/* Get modulus and exponent */
	RSA_get0_key(rsa, &n, &e, NULL);

	if (!n || !e) {
		logger(Core, Error, "%s:%s:%d Failed to get RSA parameters\n",
		       __FILE__, __func__, __LINE__);
		goto out;
	}

	/* Encode public key in format expected by RDP */
	pk_size = 0;

	/* Magic (0x31) */
	pk_data[pk_size++] = 0x31;

	/* Modulus length */
	int mod_len = BN_num_bytes(n);
	pk_data[pk_size++] = (mod_len >> 8) & 0xff;
	pk_data[pk_size++] = mod_len & 0xff;

	/* Bit length */
	int bit_len = BN_num_bits(n);
	pk_data[pk_size++] = (bit_len >> 8) & 0xff;
	pk_data[pk_size++] = bit_len & 0xff;

	/* Exponent length (should be 4 for 0x010001) */
	int exp_len = BN_num_bytes(e);
	pk_data[pk_size++] = (exp_len >> 8) & 0xff;
	pk_data[pk_size++] = exp_len & 0xff;

	/* Modulus (big-endian) */
	BN_bn2bin(n, &pk_data[pk_size]);
	pk_size += mod_len;

	/* Exponent (big-endian) */
	BN_bn2bin(e, &pk_data[pk_size]);
	pk_size += exp_len;

	/* Create stream with public key data */
	s = s_alloc(pk_size);
	out_uint8p(s, pk_data, pk_size);
	s_mark_end(s);
	s_seek(s, 0);

out:
	if (rsa) RSA_free(rsa);
	if (pkey) EVP_PKEY_free(pkey);
	if (cert) X509_free(cert);

	return s;
}

// ==== ADD CLEANUP FUNCTION (call from tcp_disconnect) ====
void tcp_tls_disconnect(void)
{
	if (g_tls_session) {
		SSL_shutdown(g_tls_session);
		SSL_free(g_tls_session);
		g_tls_session = NULL;
	}
	if (g_ssl_ctx) {
		SSL_CTX_free(g_ssl_ctx);
		g_ssl_ctx = NULL;
	}
}
