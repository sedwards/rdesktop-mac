/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop RDP_Protocol client.
   CredSSP layer and Kerberos support.
   Copyright 2012-2017 Henrik Andersson <hean01@cendio.se> for Cendio AB
   Copyright 2026 Antigravity Team / sedwards

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "rdesktop.h"
#include "deps/nla2/nlav2.h"

static STREAM
cssp_read_tsrequest_raw(void)
{
	STREAM s, out;
	int length;
	int tagval;

	/* Read first 4 bytes of ASN.1 header into g_in static stream */
	s = tcp_recv(NULL, 4);
	if (s == NULL)
		return NULL;

	/* Parse the header to find the sequence length */
	if (!ber_in_header(s, &tagval, &length) ||
	    tagval != (BER_TAG_SEQUENCE | BER_TAG_CONSTRUCTED))
	{
		return NULL;
	}

	/* Calculate remaining bytes in sequence and read them into g_in */
	length -= s_remaining(s);
	s = tcp_recv(s, length);
	if (s == NULL)
		return NULL;

	/* Total length is simply the accumulated length in the stream g_in */
	int total_len = s_length(s);
	out = s_alloc(total_len);
	if (out == NULL)
		return NULL;

	/* Copy the entire raw TSRequest sequence from g_in (s) to the new stream */
	s_seek(s, 0);
	out_uint8stream(out, s, total_len);
	s_mark_end(out);
	s_seek(out, 0);

	return out;
}

RD_BOOL
cssp_connect(char *server, char *user, char *domain, char *password, STREAM s)
{
	UNUSED(s);
	nlav2_client_context_t* client = NULL;
	STREAM pubkey = NULL;
	STREAM server_resp = NULL;
	unsigned char* pubkey_data = NULL;
	unsigned int pubkey_len = 0;
	unsigned char out_buf[8192];
	int client_len = 0;
	int server_len = 0;
	RD_BOOL success = False;

	logger(Core, Notice, "cssp_connect(): initiating TLS connection to %s", server);

	/* 1. Establish TLS connection to server */
	if (!tcp_tls_connect())
	{
		logger(Core, Error, "cssp_connect(): failed to establish TLS connection");
		return False;
	}

	/* 2. Retrieve server public key (needed for CredSSP binding) */
	pubkey = tcp_tls_get_server_pubkey();
	if (pubkey == NULL)
	{
		logger(Core, Error, "cssp_connect(): failed to get server public key");
		return False;
	}
	pubkey_data = pubkey->data;
	pubkey_len = s_length(pubkey);

	logger(Core, Notice, "cssp_connect(): retrieved server public key (%u bytes)", pubkey_len);

	/* 3. Initialize nlav2 client context */
	client = nlav2_client_new(
		server ? server : "",
		user ? user : "",
		password ? password : "",
		domain ? domain : ""
	);
	if (!client)
	{
		logger(Core, Error, "cssp_connect(): failed to create nlav2 client context");
		goto cleanup;
	}

	if (nlav2_client_set_server_public_key(client, pubkey_data, pubkey_len) != 0)
	{
		logger(Core, Error, "cssp_connect(): failed to set server public key on client");
		goto cleanup;
	}

	/* 4. Generate initial NLA Client Hello token */
	client_len = nlav2_client_get_output_token(client, out_buf, sizeof(out_buf));
	if (client_len <= 0)
	{
		logger(Core, Error, "cssp_connect(): failed to generate initial token");
		goto cleanup;
	}

	/* 5. Handshake negotiation loop */
	int handshake_step = 0;
	while (client_len > 0)
	{
		handshake_step++;

		/* Send outgoing PDU to server */
		STREAM out_stream = tcp_init(client_len);
		out_uint8a(out_stream, out_buf, client_len);
		s_mark_end(out_stream);
		tcp_send(out_stream);

		logger(Core, Notice, "cssp_connect(): sent token (%d bytes)", client_len);

		/* If we just sent the final credentials token (step 3), the NLA handshake is complete! */
		if (handshake_step == 3)
		{
			success = True;
			break;
		}

		/* Read raw incoming PDU from server */
		server_resp = cssp_read_tsrequest_raw();
		if (!server_resp)
		{
			logger(Core, Error, "cssp_connect(): failed to read server response");
			goto cleanup;
		}

		server_len = s_length(server_resp);
		logger(Core, Notice, "cssp_connect(): received server response (%d bytes)", server_len);

		/* Feed response into nlav2 engine to compute next step */
		client_len = nlav2_client_process_token(
			client, 
			server_resp->data, 
			server_len, 
			out_buf, 
			sizeof(out_buf)
		);

		s_free(server_resp);
		server_resp = NULL;

		if (client_len < 0)
		{
			logger(Core, Error, "cssp_connect(): NLA handshake processing failed");
			goto cleanup;
		}
	}

	logger(Core, Notice, "CredSSP v2 (NLA v2) Handshake Completed Successfully!");
	success = True;

cleanup:
	if (client)
		nlav2_client_free(client);
	if (pubkey)
		s_free(pubkey);
	if (server_resp)
		s_free(server_resp);

	return success;
}
