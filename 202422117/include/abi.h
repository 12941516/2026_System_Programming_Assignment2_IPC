#ifndef SIMPLE_IPC_ABI_H
#define SIMPLE_IPC_ABI_H

/*
 * Simple-IPC ABI
 *
 * This header defines the user/kernel ABI shared by the kernel module,
 * server process, and client process. The kernel module must treat payload
 * bytes as opaque data and must not interpret application-level semantics.
 */

#ifdef __KERNEL__
    #include <linux/types.h>
    #include <linux/ioctl.h>
    typedef   __u64     u64;
    typedef   __u32     u32;
    typedef   __u8      u8;
#else
    #include <stdint.h>
    #include <sys/ioctl.h>
    typedef   uint64_t  u64;
    typedef   uint32_t  u32;
    typedef   uint8_t   u8;
#endif

// Device Driver 경로와 Magic number 정의
#define SIMPLE_IPC_DEVICE_PATH "/dev/simipc"
#define SIMPLE_IPC_MAGIC       's'
#define SIMPLE_IPC_NAME_MAX    32
#define SIMPLE_IPC_PAYLOAD_MAX 128

// 주요 자료구조 : ipc
// 통신 프로세스 간에 데이터를 주고받기 위한 IPC 메시지 패킷 구조체
struct ipc {
    char endpoint[SIMPLE_IPC_NAME_MAX]; // 서버를 식별하기 위한 문자 기반 식별자
    u32 msg_id;                         // 메시지 식별을 위한 ID
    u32 payload_len;                    // payload 데이터의 Byte 길이
    u8 payload[SIMPLE_IPC_PAYLOAD_MAX]; // 통신 시 데이터인 Key-Value 쌍이 저장되는 버퍼
};

// 주요 ioctl Command를 정의한 것
// _IOW, _IOR 매크로를 사용해 데이터 전송 방향과 크기를 명시한 System call
#define SIMPLE_IPC_IOC_BIND_SERVER    _IOW(SIMPLE_IPC_MAGIC, 1, char[SIMPLE_IPC_NAME_MAX])  // server 프로세스 등록 및 binding
#define SIMPLE_IPC_IOC_UNBIND_SERVER  _IOW(SIMPLE_IPC_MAGIC, 2, char[SIMPLE_IPC_NAME_MAX])  // server 프로세스 해제 및 unbinding
#define SIMPLE_IPC_IOC_SEND_REQUEST   _IOW(SIMPLE_IPC_MAGIC, 3, struct ipc)                 // client to kernel request 전달
#define SIMPLE_IPC_IOC_RECV_REQUEST   _IOR(SIMPLE_IPC_MAGIC, 4, struct ipc)                 // server의 kernel request blocking
#define SIMPLE_IPC_IOC_SEND_REPLY     _IOW(SIMPLE_IPC_MAGIC, 5, struct ipc)                 // server to kernel reply 전달
#define SIMPLE_IPC_IOC_RECV_REPLY     _IOR(SIMPLE_IPC_MAGIC, 6, struct ipc)                 // client의 server reply blocking

#endif /* SIMPLE_IPC_ABI_H */
