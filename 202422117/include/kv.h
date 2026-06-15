#ifndef SIMPLE_IPC_H
#define SIMPLE_IPC_H

#include <stdint.h>

#define KV_OP_PUT 1
#define KV_OP_GET 2

#define KV_KEY_MAX   32
#define KV_VALUE_MAX 64

struct kv_payload {
    uint32_t op;
    int32_t status;
    char key[KV_KEY_MAX];
    char value[KV_VALUE_MAX];
};

#endif