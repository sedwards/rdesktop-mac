#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nlav2.h"

int main(void)
{
	printf("Starting NLAv2 Integration Test...\n");

	// Dummy public key (representing SubjectPublicKeyInfo from server certificate)
	unsigned char pubkey[64];
	for (int i = 0; i < 64; i++)
	{
		pubkey[i] = (unsigned char)(i ^ 0x5A);
	}

	// 1. Create client and server contexts
	nlav2_client_context_t* client = nlav2_client_new("localhost", "testuser", "testpassword", "testdomain");
	if (!client)
	{
		printf("Failed to create client context\n");
		return 1;
	}

	// Create mock SAM database file
	FILE* f_sam = fopen("sam_db.txt", "w");
	if (f_sam)
	{
		fprintf(f_sam, "testuser:testdomain::D32A2901011176349B41D406DCC95A90:\n");
		fclose(f_sam);
	}
	else
	{
		printf("Failed to create mock SAM file\n");
		nlav2_client_free(client);
		return 1;
	}

	nlav2_server_context_t* server = nlav2_server_new(pubkey, sizeof(pubkey));
	if (!server)
	{
		printf("Failed to create server context\n");
		remove("sam_db.txt");
		nlav2_client_free(client);
		return 1;
	}

	// Associate mock SAM database with server
	if (nlav2_server_set_sam_file(server, "sam_db.txt") != 0)
	{
		printf("Failed to set SAM file on server\n");
		remove("sam_db.txt");
		nlav2_client_free(client);
		nlav2_server_free(server);
		return 1;
	}

	// Set server public key in client
	if (nlav2_client_set_server_public_key(client, pubkey, sizeof(pubkey)) != 0)
	{
		printf("Failed to set server public key in client\n");
		remove("sam_db.txt");
		nlav2_client_free(client);
		nlav2_server_free(server);
		return 1;
	}

	unsigned char client_buf[8192];
	unsigned char server_buf[8192];
	int client_len = 0;
	int server_len = 0;

	// 2. Client starts: generate initial token
	client_len = nlav2_client_get_output_token(client, client_buf, sizeof(client_buf));
	if (client_len <= 0)
	{
		printf("Failed to get initial client token\n");
		nlav2_client_free(client);
		nlav2_server_free(server);
		return 1;
	}
	printf("Step 1: Client generated initial token (%d bytes)\n", client_len);

	// 3. Server processes initial client token
	server_len = nlav2_server_process_token(server, client_buf, client_len, server_buf, sizeof(server_buf));
	if (server_len < 0)
	{
		printf("Server failed to process initial client token\n");
		nlav2_client_free(client);
		nlav2_server_free(server);
		return 1;
	}
	printf("Step 2: Server processed token, generated response (%d bytes)\n", server_len);

	// 4. Client processes server response
	client_len = nlav2_client_process_token(client, server_buf, server_len, client_buf, sizeof(client_buf));
	if (client_len < 0)
	{
		printf("Client failed to process server response\n");
		nlav2_client_free(client);
		nlav2_server_free(server);
		return 1;
	}
	printf("Step 3: Client processed response, generated next token (%d bytes)\n", client_len);

	// 5. Server processes second client token (public key auth)
	server_len = nlav2_server_process_token(server, client_buf, client_len, server_buf, sizeof(server_buf));
	if (server_len < 0)
	{
		printf("Server failed to process client pubkey auth token\n");
		nlav2_client_free(client);
		nlav2_server_free(server);
		return 1;
	}
	printf("Step 4: Server processed pubkey auth, generated response (%d bytes)\n", server_len);

	// 6. Client processes server response
	client_len = nlav2_client_process_token(client, server_buf, server_len, client_buf, sizeof(client_buf));
	if (client_len < 0)
	{
		printf("Client failed to process server pubkey response\n");
		nlav2_client_free(client);
		nlav2_server_free(server);
		return 1;
	}
	printf("Step 5: Client processed server pubkey response, generated next token (credentials) (%d bytes)\n", client_len);

	// 7. Server processes third client token (credentials)
	server_len = nlav2_server_process_token(server, client_buf, client_len, server_buf, sizeof(server_buf));
	if (server_len < 0)
	{
		printf("Server failed to process client credentials token\n");
		nlav2_client_free(client);
		nlav2_server_free(server);
		return 1;
	}
	printf("Step 6: Server processed client credentials token, result = %d\n", server_len);

	// 8. Retrieve server-side authenticated identity
	char username[256] = { 0 };
	char domain[256] = { 0 };
	if (nlav2_server_get_identity(server, username, sizeof(username), domain, sizeof(domain)) != 0)
	{
		printf("Failed to retrieve authenticated identity from server\n");
		nlav2_client_free(client);
		nlav2_server_free(server);
		return 1;
	}
	printf("Step 7: Server authenticated identity: User=%s, Domain=%s\n", username, domain);

	if (strcmp(username, "testuser") != 0 || strcmp(domain, "testdomain") != 0)
	{
		printf("Error: Authenticated identity does not match expected credentials!\n");
		nlav2_client_free(client);
		nlav2_server_free(server);
		return 1;
	}

	printf("NLAv2 Handshake Completed Successfully! Security verified.\n");

	// Clean up
	remove("sam_db.txt");
	nlav2_client_free(client);
	nlav2_server_free(server);
	return 0;
}
