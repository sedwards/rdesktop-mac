#include "nlav2.h"
#include <freerdp/client.h>
#include <freerdp/peer.h>
#include <freerdp/settings.h>
#include <winpr/sspi.h>
#include <winpr/synch.h>
#include <winpr/thread.h>
#include <winpr/stream.h>
#include <winpr/crt.h>
#include <winpr/ssl.h>

#include "nla.h"
#include "transport.h"
#include "settings.h"

#include <stdlib.h>
#include <string.h>

struct rdp_transport
{
	int layer;
	void* frontBio;
	void* rdg;
	void* tsg;
	void* wst;
	void* tls;
	rdpContext* context;
	rdpNla* nla;
	void* ReceiveExtra;
	wStream* ReceiveBuffer;
	void* ReceiveCallback;
	wStreamPool* ReceivePool;
	HANDLE connectedEvent;
	BOOL NlaMode;
	BOOL RdstlsMode;
	BOOL AadMode;
	BOOL blocking;
	BOOL GatewayEnabled;
	BOOL haveReadLock;
	CRITICAL_SECTION ReadLock;
	BOOL haveWriteLock;
	CRITICAL_SECTION WriteLock;
	UINT64 written;
	HANDLE rereadEvent;
	BOOL haveMoreBytesToRead;
	wLog* log;
	rdpTransportIo io;
	HANDLE ioEvent;
	BOOL useIoEvent;
	BOOL earlyUserAuth;
};

/* Client Context Structure */
struct nlav2_client_context
{
	rdpContext* rdpcontext;
	rdpTransport* transport;
	rdpNla* nla;

	unsigned char* server_pubkey;
	unsigned int server_pubkey_len;

	unsigned char* out_token;
	unsigned int out_token_len;
	unsigned int out_token_cap;
};

/* Client Write PDU Callback */
static int client_write_pdu(rdpTransport* transport, wStream* s)
{
	rdpContext* rdpcontext = transport->context;
	nlav2_client_context_t* ctx = (nlav2_client_context_t*)freerdp_get_io_callback_context(rdpcontext);
	if (!ctx)
		return -1;

	size_t len = Stream_GetPosition(s);
	if (len > ctx->out_token_cap)
	{
		ctx->out_token = realloc(ctx->out_token, len);
		ctx->out_token_cap = len;
	}
	Stream_ResetPosition(s);
	memcpy(ctx->out_token, Stream_Buffer(s), len);
	ctx->out_token_len = (unsigned int)len;

	return (int)len;
}

/* Client Get Public Key Callback */
static BOOL client_get_public_key(rdpTransport* transport, const BYTE** data, DWORD* length)
{
	rdpContext* rdpcontext = transport->context;
	nlav2_client_context_t* ctx = (nlav2_client_context_t*)freerdp_get_io_callback_context(rdpcontext);
	if (!ctx || !ctx->server_pubkey)
		return FALSE;

	*data = ctx->server_pubkey;
	*length = ctx->server_pubkey_len;
	return TRUE;
}

/* Client API Implementation */

nlav2_client_context_t* nlav2_client_new(const char* hostname, const char* username, const char* password, const char* domain)
{
	winpr_InitializeSSL(WINPR_SSL_INIT_DEFAULT);

	nlav2_client_context_t* ctx = calloc(1, sizeof(nlav2_client_context_t));
	if (!ctx)
		return NULL;

	RDP_CLIENT_ENTRY_POINTS entry = { 0 };
	entry.Version = RDP_CLIENT_INTERFACE_VERSION;
	entry.Size = sizeof(RDP_CLIENT_ENTRY_POINTS_V1);
	entry.ContextSize = sizeof(rdpContext);

	ctx->rdpcontext = freerdp_client_context_new(&entry);
	if (!ctx->rdpcontext)
	{
		free(ctx);
		return NULL;
	}

	freerdp_set_io_callback_context(ctx->rdpcontext, ctx);

	rdpSettings* settings = ctx->rdpcontext->settings;
	freerdp_settings_set_bool(settings, FreeRDP_ServerMode, FALSE);
	freerdp_settings_set_string(settings, FreeRDP_ServerHostname, hostname);
	freerdp_settings_set_string(settings, FreeRDP_Username, username);
	freerdp_settings_set_string(settings, FreeRDP_Password, password);
	freerdp_settings_set_string(settings, FreeRDP_Domain, domain);

	ctx->transport = transport_new(ctx->rdpcontext);
	if (!ctx->transport)
	{
		freerdp_client_context_free(ctx->rdpcontext);
		free(ctx);
		return NULL;
	}

	ctx->transport->io.WritePdu = client_write_pdu;
	ctx->transport->io.GetPublicKey = client_get_public_key;

	ctx->nla = nla_new(ctx->rdpcontext, ctx->transport);
	if (!ctx->nla)
	{
		transport_free(ctx->transport);
		freerdp_client_context_free(ctx->rdpcontext);
		free(ctx);
		return NULL;
	}

	return ctx;
}

int nlav2_client_set_server_public_key(nlav2_client_context_t* ctx, const unsigned char* pubkey, unsigned int pubkey_len)
{
	if (!ctx || !pubkey)
		return -1;

	free(ctx->server_pubkey);
	ctx->server_pubkey = malloc(pubkey_len);
	if (!ctx->server_pubkey)
		return -1;

	memcpy(ctx->server_pubkey, pubkey, pubkey_len);
	ctx->server_pubkey_len = pubkey_len;
	return 0;
}

int nlav2_client_get_output_token(nlav2_client_context_t* ctx, unsigned char* out_buf, unsigned int max_out_len)
{
	if (!ctx)
		return -1;

	ctx->out_token_len = 0;

	int rc = nla_client_begin(ctx->nla);
	if (rc < 0)
		return -1;

	if (ctx->out_token_len > 0)
	{
		if (ctx->out_token_len > max_out_len)
			return -1;
		memcpy(out_buf, ctx->out_token, ctx->out_token_len);
		return (int)ctx->out_token_len;
	}

	return 0;
}

int nlav2_client_process_token(nlav2_client_context_t* ctx, const unsigned char* in_buf, unsigned int in_len, unsigned char* out_buf, unsigned int max_out_len)
{
	if (!ctx || !in_buf)
		return -1;

	ctx->out_token_len = 0;

	wStream sbuffer = { 0 };
	wStream* s = Stream_StaticConstInit(&sbuffer, in_buf, in_len);
	if (!s)
		return -1;

	int rc = nla_recv_pdu(ctx->nla, s);
	if (rc < 0)
		return -1;

	if (ctx->out_token_len > 0)
	{
		if (ctx->out_token_len > max_out_len)
			return -1;
		memcpy(out_buf, ctx->out_token, ctx->out_token_len);
		return (int)ctx->out_token_len;
	}

	if (nla_get_state(ctx->nla) >= NLA_STATE_AUTH_INFO)
		return 0; // Completed successfully

	return 0;
}

void nlav2_client_free(nlav2_client_context_t* ctx)
{
	if (!ctx)
		return;

	if (ctx->nla)
		nla_free(ctx->nla);

	if (ctx->transport)
		transport_free(ctx->transport);

	if (ctx->rdpcontext)
		freerdp_client_context_free(ctx->rdpcontext);

	free(ctx->server_pubkey);
	free(ctx->out_token);
	free(ctx);
}


/* Server Context Structure */
struct nlav2_server_context
{
	freerdp_peer* peer;
	rdpContext* rdpcontext;
	rdpTransport* transport;
	rdpNla* nla;

	unsigned char* server_pubkey;
	unsigned int server_pubkey_len;

	HANDLE thread;
	HANDLE sem_input_ready;
	HANDLE sem_output_ready;

	const unsigned char* input_token;
	unsigned int input_token_len;

	unsigned char* output_token;
	unsigned int output_token_len;
	unsigned int output_token_cap;

	BOOL auth_success;
	BOOL auth_failed;
};

/* Server Write PDU Callback */
static int server_write_pdu(rdpTransport* transport, wStream* s)
{
	rdpContext* rdpcontext = transport->context;
	nlav2_server_context_t* ctx = (nlav2_server_context_t*)freerdp_get_io_callback_context(rdpcontext);
	if (!ctx)
		return -1;

	size_t len = Stream_GetPosition(s);
	if (len > ctx->output_token_cap)
	{
		ctx->output_token = realloc(ctx->output_token, len);
		ctx->output_token_cap = len;
	}
	Stream_ResetPosition(s);
	memcpy(ctx->output_token, Stream_Buffer(s), len);
	ctx->output_token_len = (unsigned int)len;

	ReleaseSemaphore(ctx->sem_output_ready, 1, NULL);

	return (int)len;
}

/* Server Get Public Key Callback */
static BOOL server_get_public_key(rdpTransport* transport, const BYTE** data, DWORD* length)
{
	rdpContext* rdpcontext = transport->context;
	nlav2_server_context_t* ctx = (nlav2_server_context_t*)freerdp_get_io_callback_context(rdpcontext);
	if (!ctx || !ctx->server_pubkey)
		return FALSE;

	*data = ctx->server_pubkey;
	*length = ctx->server_pubkey_len;
	return TRUE;
}

/* Server Read PDU Callback */
static int server_read_pdu(rdpTransport* transport, wStream* s)
{
	rdpContext* rdpcontext = transport->context;
	nlav2_server_context_t* ctx = (nlav2_server_context_t*)freerdp_get_io_callback_context(rdpcontext);
	if (!ctx)
		return -1;

	WaitForSingleObject(ctx->sem_input_ready, INFINITE);

	if (ctx->auth_failed)
		return -1;

	if (!Stream_EnsureRemainingCapacity(s, ctx->input_token_len))
		return -1;

	Stream_Write(s, ctx->input_token, ctx->input_token_len);
	Stream_SealLength(s);
	Stream_SetPosition(s, 0);

	return (int)ctx->input_token_len;
}

/* Server Thread Entry Point */
static DWORD WINAPI server_auth_thread_func(LPVOID lpParam)
{
	nlav2_server_context_t* ctx = (nlav2_server_context_t*)lpParam;

	int rc = nla_authenticate(ctx->nla);
	if (rc == 1)
	{
		ctx->auth_success = TRUE;
	}
	else
	{
		ctx->auth_failed = TRUE;
	}

	ReleaseSemaphore(ctx->sem_output_ready, 1, NULL);

	return 0;
}

/* Server API Implementation */

nlav2_server_context_t* nlav2_server_new(const unsigned char* server_pubkey, unsigned int server_pubkey_len)
{
	winpr_InitializeSSL(WINPR_SSL_INIT_DEFAULT);

	nlav2_server_context_t* ctx = calloc(1, sizeof(nlav2_server_context_t));
	if (!ctx)
		return NULL;

	ctx->server_pubkey = malloc(server_pubkey_len);
	if (!ctx->server_pubkey)
	{
		free(ctx);
		return NULL;
	}
	memcpy(ctx->server_pubkey, server_pubkey, server_pubkey_len);
	ctx->server_pubkey_len = server_pubkey_len;

	ctx->peer = calloc(1, sizeof(freerdp_peer));
	if (!ctx->peer)
	{
		free(ctx->server_pubkey);
		free(ctx);
		return NULL;
	}

	ctx->peer->ContextSize = sizeof(rdpContext);
	if (!freerdp_peer_context_new(ctx->peer))
	{
		free(ctx->peer);
		free(ctx->server_pubkey);
		free(ctx);
		return NULL;
	}

	ctx->rdpcontext = ctx->peer->context;
	freerdp_set_io_callback_context(ctx->rdpcontext, ctx);

	rdpSettings* settings = ctx->rdpcontext->settings;
	freerdp_settings_set_bool(settings, FreeRDP_ServerMode, TRUE);

	ctx->transport = transport_new(ctx->rdpcontext);
	if (!ctx->transport)
	{
		freerdp_peer_context_free(ctx->peer);
		free(ctx->peer);
		free(ctx->server_pubkey);
		free(ctx);
		return NULL;
	}

	ctx->transport->io.WritePdu = server_write_pdu;
	ctx->transport->io.GetPublicKey = server_get_public_key;
	ctx->transport->io.ReadPdu = server_read_pdu;

	ctx->nla = nla_new(ctx->rdpcontext, ctx->transport);
	if (!ctx->nla)
	{
		transport_free(ctx->transport);
		freerdp_peer_context_free(ctx->peer);
		free(ctx->peer);
		free(ctx->server_pubkey);
		free(ctx);
		return NULL;
	}

	ctx->sem_input_ready = CreateSemaphore(NULL, 0, 1, NULL);
	ctx->sem_output_ready = CreateSemaphore(NULL, 0, 1, NULL);

	return ctx;
}

int nlav2_server_process_token(nlav2_server_context_t* ctx, const unsigned char* in_buf, unsigned int in_len, unsigned char* out_buf, unsigned int max_out_len)
{
	if (!ctx)
		return -1;

	if (!ctx->thread)
	{
		ctx->thread = CreateThread(NULL, 0, server_auth_thread_func, ctx, 0, NULL);
		if (!ctx->thread)
			return -1;
	}

	ctx->input_token = in_buf;
	ctx->input_token_len = in_len;
	ctx->output_token_len = 0;

	ReleaseSemaphore(ctx->sem_input_ready, 1, NULL);

	WaitForSingleObject(ctx->sem_output_ready, INFINITE);

	if (ctx->auth_failed)
		return -1;

	if (ctx->output_token_len > 0)
	{
		if (ctx->output_token_len > max_out_len)
			return -1;
		memcpy(out_buf, ctx->output_token, ctx->output_token_len);
		return (int)ctx->output_token_len;
	}

	if (ctx->auth_success)
		return 0;

	return -1;
}

int nlav2_server_get_identity(nlav2_server_context_t* ctx, char* username, unsigned int max_user_len, char* domain, unsigned int max_domain_len)
{
	if (!ctx || !ctx->auth_success)
		return -1;

	SecPkgContext_AuthIdentity identity = { 0 };
	SECURITY_STATUS status = nla_QueryContextAttributes(ctx->nla, SECPKG_ATTR_AUTH_IDENTITY, &identity);
	if (status != SEC_E_OK)
		return -1;

	strncpy(username, identity.User, max_user_len);
	username[max_user_len - 1] = '\0';

	strncpy(domain, identity.Domain, max_domain_len);
	domain[max_domain_len - 1] = '\0';

	return 0;
}

int nlav2_server_set_sam_file(nlav2_server_context_t* ctx, const char* sam_file_path)
{
	if (!ctx || !sam_file_path)
		return -1;

	rdpSettings* settings = ctx->rdpcontext->settings;
	return freerdp_settings_set_string(settings, FreeRDP_NtlmSamFile, sam_file_path) ? 0 : -1;
}

void nlav2_server_free(nlav2_server_context_t* ctx)
{
	if (!ctx)
		return;

	ctx->auth_failed = TRUE;
	ReleaseSemaphore(ctx->sem_input_ready, 1, NULL);

	if (ctx->thread)
	{
		WaitForSingleObject(ctx->thread, INFINITE);
		CloseHandle(ctx->thread);
	}

	if (ctx->sem_input_ready)
		CloseHandle(ctx->sem_input_ready);
	if (ctx->sem_output_ready)
		CloseHandle(ctx->sem_output_ready);

	if (ctx->nla)
		nla_free(ctx->nla);

	if (ctx->transport)
		transport_free(ctx->transport);

	if (ctx->peer)
	{
		freerdp_peer_context_free(ctx->peer);
		free(ctx->peer);
	}

	free(ctx->server_pubkey);
	free(ctx->output_token);
	free(ctx);
}
