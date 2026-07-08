#ifndef NLAV2_H
#define NLAV2_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
  #ifdef NLAV2_EXPORTS
    #define NLAV2_API __declspec(dllexport)
  #else
    #define NLAV2_API __declspec(dllimport)
  #endif
#else
  #define NLAV2_API __attribute__((visibility("default")))
#endif

typedef struct nlav2_client_context nlav2_client_context_t;
typedef struct nlav2_server_context nlav2_server_context_t;

/* Client-Side API */

NLAV2_API nlav2_client_context_t* nlav2_client_new(const char* hostname, const char* username, const char* password, const char* domain);

NLAV2_API int nlav2_client_set_server_public_key(nlav2_client_context_t* ctx, const unsigned char* pubkey, unsigned int pubkey_len);

NLAV2_API int nlav2_client_get_output_token(nlav2_client_context_t* ctx, unsigned char* out_buf, unsigned int max_out_len);

NLAV2_API int nlav2_client_process_token(nlav2_client_context_t* ctx, const unsigned char* in_buf, unsigned int in_len, unsigned char* out_buf, unsigned int max_out_len);

NLAV2_API void nlav2_client_free(nlav2_client_context_t* ctx);


/* Server-Side API */

NLAV2_API nlav2_server_context_t* nlav2_server_new(const unsigned char* server_pubkey, unsigned int server_pubkey_len);

NLAV2_API int nlav2_server_process_token(nlav2_server_context_t* ctx, const unsigned char* in_buf, unsigned int in_len, unsigned char* out_buf, unsigned int max_out_len);

NLAV2_API int nlav2_server_get_identity(nlav2_server_context_t* ctx, char* username, unsigned int max_user_len, char* domain, unsigned int max_domain_len);

NLAV2_API int nlav2_server_set_sam_file(nlav2_server_context_t* ctx, const char* sam_file_path);

NLAV2_API void nlav2_server_free(nlav2_server_context_t* ctx);

#ifdef __cplusplus
}
#endif

#endif /* NLAV2_H */
