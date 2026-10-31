#ifndef TRUST_WORK_CHAIN_RPC_H
#define TRUST_WORK_CHAIN_RPC_H

#include <stddef.h>

int chain_rpc_post(const char *url, const char *body, char *response, size_t response_size);

#endif
