#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../include/abi.h"
#include "../include/kv.h"

#define KV_TABLE_MAX 32

// 주요 자료구조로 kv_entry를 사용
// server 내부 메모리에 Key-Value 쌍의 데이터를 보관하기 위한 Record table 구조체
struct kv_entry {
    int used;                   // 해당 slot의 유효성 여부를 판단하는 flag
    char key[KV_KEY_MAX];       // key 문자열을 저장하는 배열
    char value[KV_VALUE_MAX];   // value 문자열을 저장하는 배열
};

// 전역으로 선언된 Key-Value 데이터 메모리 저장소
static struct kv_entry table[KV_TABLE_MAX];

// signal 핸들러 및 안전한 unbinding을 위한 전역 driver file descripter 변수
static int g_fd = -1;
static char g_endpoint[SIMPLE_IPC_NAME_MAX];

// 주요 함수: kv_put(client의 PUT 명령을 받아, 중복 키인 경우엔 overwrite하고 새 키이면 빈 slot에 저장)
static void kv_put(const char *key, const char *value){
    int i;
    
    // 1. 이미 존재하는 Key의 경우, 새 정보로 value를 갱신
    for(i=0; i<KV_TABLE_MAX; i++){
        if(table[i].used && strcmp(table[i].key, key) == 0){
            strncpy(table[i].value, value, KV_VALUE_MAX - 1);
            table[i].value[KV_VALUE_MAX - 1] = '\0';
            return;
        }
    }
    
    // 2. 새 Key의 경우, table 내부의 빈 slot을 찾아 value를 저장
    for(i=0; i<KV_TABLE_MAX; i++){
        if(!table[i].used){
            table[i].used = 1;
            strncpy(table[i].key, key, KV_KEY_MAX - 1);
            strncpy(table[i].value, value, KV_VALUE_MAX - 1);
            table[i].key[KV_KEY_MAX - 1] = '\0';
            table[i].value[KV_VALUE_MAX - 1] = '\0';
            return;
        }
    }
}

// 주요 함수: kv_get(Key 문자열을 table에서 탐색하여 대응되는 Value의 버퍼 포인터를 반환)
static const char *kv_get(const char *key){
    int i;
    for(i=0; i<KV_TABLE_MAX; i++){
        if(table[i].used && strcmp(table[i].key, key) == 0)
            return table[i].value;
    }
    return NULL;
}

// 주요 함수: cleanup_server(프로그램이 종료될 때 UNBIND ioctl을 호출하여 상태 해제)
static void cleanup_server(void) {
    if(g_fd >= 0){
        ioctl(g_fd, SIMPLE_IPC_IOC_UNBIND_SERVER, g_endpoint);
        close(g_fd);
        g_fd = -1;
    }
}

// Signal Handler : 프로세스 종료 시 exit(0)을 유도하여 atexit 트리거 발행
static void handle_signal(int signo) {
    (void)signo;
    exit(0);
}

int main(int argc, char **argv) {
    struct sigaction sa;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <endpoint>\n", argv[0]);
        return 1;
    }

    // signal 처리용 구성
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // 프로그램 종료 시 자동으로 Unbinding 실행
    atexit(cleanup_server);

    // Argument로 받은 식별자를 등록하고 Device를 open
    strncpy(g_endpoint, argv[1], SIMPLE_IPC_NAME_MAX);
    g_endpoint[SIMPLE_IPC_NAME_MAX - 1] = '\0';
    g_fd = open(SIMPLE_IPC_DEVICE_PATH, O_RDWR);
    if(g_fd < 0){
        perror("open");
        return 1;
    }

    // 식별자를 kernel module의 서버에 Binding
    if(ioctl(g_fd, SIMPLE_IPC_IOC_BIND_SERVER, g_endpoint) < 0){
        perror("bind");
        return 1;
    }

    // IPC Request를 처리하는 Loop
    while(1){
        struct ipc msg;
        struct kv_payload kv;

        // Request 도달 후 Driver가 server를 깨워줄 때까지 무한정 Blocking하는 부분
        if(ioctl(g_fd, SIMPLE_IPC_IOC_RECV_REQUEST, &msg) < 0){
            perror("recv_request");
            continue;
        }

        memcpy(&kv, msg.payload, sizeof(kv));
        struct ipc reply;
        struct kv_payload reply_kv;
        memset(&reply, 0, sizeof(reply));
        memset(&reply_kv, 0, sizeof(reply_kv));
        strncpy(reply.endpoint, msg.endpoint, SIMPLE_IPC_NAME_MAX - 1);
        
        // User Space의 application의 로직 수행
        if(kv.op == KV_OP_PUT){
            kv_put(kv.key, kv.value);
            printf("PUT K: %s V: %s\n", kv.key, kv.value);
            reply_kv.op = KV_OP_PUT;
            reply_kv.status = 0;
        }
        else if(kv.op == KV_OP_GET){
            const char *value;
            value = kv_get(kv.key);
            if(value){
                printf("GET K: %s V: %s\n", kv.key, value);
                reply_kv.op = KV_OP_GET;
                reply_kv.status = 0;
                strncpy(reply_kv.key, kv.key, KV_KEY_MAX - 1);
                strncpy(reply_kv.value, value, KV_VALUE_MAX - 1);
            }
            else{   // 데이터가 없을 시 에러를 반환하기 위한 구문
                printf("GET K: %s NOT FOUND\n", kv.key);
                reply_kv.op = KV_OP_GET;
                reply_kv.status = -ENOENT;
                strncpy(reply_kv.key, kv.key, KV_KEY_MAX - 1);
            }
        }
        else
            continue;
        
        // 처리 결과 패키징하여 client의 wait Queue에 전송
        memcpy(reply.payload, &reply_kv, sizeof(reply_kv));
        reply.payload_len = sizeof(reply_kv);
        if(ioctl(g_fd, SIMPLE_IPC_IOC_SEND_REPLY, &reply) < 0)
            perror("send_reply");
    }

    return 0;
}
