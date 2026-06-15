#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../include/abi.h"
#include "../include/kv.h"

// 프로그램 수동 실행 시 가이드라인 출력
static void usage(const char *prog) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s <endpoint> PUT <key> <value>\n", prog);
    fprintf(stderr, "  %s <endpoint> GET <key>\n", prog);
}

// 사용자 명령을 parsing한 뒤, 알맞은 경우에 맞게 mapping하는 함수
static int parse_op(const char *op) {
    if (strcmp(op, "PUT") == 0)
        return KV_OP_PUT;

    if (strcmp(op, "GET") == 0)
        return KV_OP_GET;

    return -EINVAL;
}

int main(int argc, char **argv) {
    // Argument 유효성 검증
    if (argc < 4) {
        usage(argv[0]);
        return 1;
    }

    int op = parse_op(argv[2]);
    if(op < 0){
        fprintf(stderr, "Invalid operation\n");
        return 1;
    }
    
    // Character Device Driver 노드를 open
    int fd;
    fd = open(SIMPLE_IPC_DEVICE_PATH, O_RDWR);
    if(fd < 0){
        perror("open");
        return 1;
    }

    struct ipc msg;
    struct kv_payload kv;
    memset(&msg, 0, sizeof(msg));
    memset(&kv, 0, sizeof(kv));
    strncpy(msg.endpoint, argv[1], SIMPLE_IPC_NAME_MAX - 1);

    // 명령어의 종류에 따라 분류 후 데이터 구조체를 저장
    if(op == KV_OP_PUT){
        if(argc != 5){
            usage(argv[0]);
            return 1;
        }
        kv.op = KV_OP_PUT;
        strncpy(kv.key, argv[3], KV_KEY_MAX - 1);
        strncpy(kv.value, argv[4], KV_VALUE_MAX - 1);
    }
    else{
        if(argc != 4){
            usage(argv[0]);
            return 1;
        }
        kv.op = KV_OP_GET;
        strncpy(kv.key, argv[3], KV_KEY_MAX - 1);
    }
    memcpy(msg.payload, &kv, sizeof(kv));
    msg.payload_len = sizeof(kv);

    // 1. 통신 요청: Kernel module에 ioctl() System call로 Request 메시지 패킷을 전송
    if(ioctl(fd, SIMPLE_IPC_IOC_SEND_REQUEST, &msg) < 0){
        perror("send_reqiest");
        close(fd);
        return 1;
    }

    struct ipc reply;
    struct kv_payload reply_kv;

    // 2. 동기화 처리: 서버의 연산 완료 후 응답을 Kernel 버퍼에 반환하기까지 Blocking
    if(ioctl(fd, SIMPLE_IPC_IOC_RECV_REPLY, &reply) < 0){
        perror("recv_reply");
        close(fd);
        return 1;
    }

    // 수신이 성공한 뒤 Terminal에 표준 출력할 포맷을 결정
    memcpy(&reply_kv, reply.payload, sizeof(reply_kv));
    if(reply_kv.op == KV_OP_PUT){
        if(reply_kv.status == 0)
            // PUT 성공 시에는 단순히 줄만 바꾸고 아무것도 출력하지 않음
            printf("\n");
        else
            // GET 실패 시에는 아래 구문 출력
            printf("PUT FAILED\n");
    }
    else if(reply_kv.op == KV_OP_GET){
        if(reply_kv.status == 0)
            // GET 성공 시에는 Get한 value를 출력
            printf("%s\n", reply_kv.value);
        else
            // GET 실패 시에는 아래 구문 출력
            printf("NOT FOUND\n");
    }

    close(fd);
    return 0;
}