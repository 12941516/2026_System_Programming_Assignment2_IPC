#include <linux/fs.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#include "../include/abi.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("");
MODULE_DESCRIPTION("");
MODULE_VERSION("");

#define DRIVER_NAME "simipc"

// 주요 자료구조로 Kernel의 Wait Queue를 사용
// Client와 Server의 프로세스 blocking 및 wake up 구현을 위한 Synchronous 매커니즘
static DECLARE_WAIT_QUEUE_HEAD(server_waitq);
static DECLARE_WAIT_QUEUE_HEAD(client_waitq);

// Driver 내부의 공유 자원들을 기록하기 위한 Flag
static char bound_endpoint[SIMPLE_IPC_NAME_MAX];    // current server name
static bool server_bound = false;
static struct ipc request_msg;                      // Request from client to server                      
static struct ipc reply_msg;                        // Reply from server to client
static bool request_ready = false;                  // There be some requests server to read
static bool reply_ready = false;                    // There be some replies client to read

// 주요 함수: simple_ipc_ioctl(User space에서 호출한 ioctl() System call을 입력받아 각 Command 별 IPC 로직을 수행)
// copy_to_user와 copy_from_user System call을 사용하여 유저 영역의 데이터를 커널 영역으로 메모리 수준에서 복사한다.
static long simple_ipc_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case SIMPLE_IPC_IOC_BIND_SERVER:
        {
            char name[SIMPLE_IPC_NAME_MAX];

            // 안전하게 유저 메모리를 복사하기 위한 System call
            if(copy_from_user(name, (void __user *)arg, sizeof(name)))
                return -EFAULT;
            strncpy(bound_endpoint, name, SIMPLE_IPC_NAME_MAX);
            bound_endpoint[SIMPLE_IPC_NAME_MAX - 1] = '\0';
            server_bound = true;
            pr_info("BIND %s\n", bound_endpoint);
            return 0;
        }
        case SIMPLE_IPC_IOC_UNBIND_SERVER:
        {
            char name[SIMPLE_IPC_NAME_MAX];
            if(copy_from_user(name, (void __user *)arg, sizeof(name)))
                return -EFAULT;
            server_bound = false;
            pr_info("UNBIND %s\n", name);
            memset(bound_endpoint, 0, sizeof(bound_endpoint));
            return 0;
        }
        case SIMPLE_IPC_IOC_SEND_REQUEST:
        {
            if(!server_bound)
                return -ENODEV;
            if(copy_from_user(&request_msg, (void __user *)arg, sizeof(struct ipc)))
                return -EFAULT;
            request_ready = true;

            // RECV_REQUEST 상태로 wait Queue에서 대기 중인 server process를 깨운다.
            wake_up_interruptible(&server_waitq);
            return 0;
        }
        case SIMPLE_IPC_IOC_SEND_REPLY:
        {
            if(copy_from_user(&reply_msg, (void __user *)arg, sizeof(struct ipc)))
                return -EFAULT;
            reply_ready = true;

            // RECV_REPLY 상태로 wait Queue에서 대기 중인 client process를 깨운다.
            wake_up_interruptible(&client_waitq);
            return 0;
        }
        case SIMPLE_IPC_IOC_RECV_REQUEST:
        {
            int ret;

            // 처리할 request가 올 때까지 server를 blocking시킨다.
            ret = wait_event_interruptible(server_waitq, request_ready);
            if(ret)
                return ret;
            if(copy_to_user((void __user *)arg, &request_msg, sizeof(struct ipc)))
                return -EFAULT;
            request_ready = false;
            return 0;
        }
        case SIMPLE_IPC_IOC_RECV_REPLY:
        {
            int ret;

            // 서버에서 reply가 올 때까지 client를 blocking시킨다.
            ret = wait_event_interruptible(client_waitq, reply_ready);
            if(ret)
                return ret;
            if(copy_to_user((void __user *)arg, &reply_msg, sizeof(struct ipc)))
                return -EFAULT;
            reply_ready = false;
            return 0;
        }
        default:
            return -ENOTTY;
    }
}

// 파일 Operations 구조체를 등록하는 부분
static const struct file_operations simple_ipc_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = simple_ipc_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = simple_ipc_ioctl,
#endif
};

// Miscellaneous Device 구조체를 정의하는 부분
static struct miscdevice simple_ipc_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DRIVER_NAME,
    .fops = &simple_ipc_fops,
    .mode = 0666,
};

// 모듈을 로드할 때 초기화하는 함수
static int __init simple_ipc_init(void) {
    int ret;

    ret = misc_register(&simple_ipc_dev);
    if (ret)
        return ret;

    pr_info("simple_ipc: module loaded\n");
    return 0;
}

// 모듈을 언로드할 때 정리하는 함수
static void __exit simple_ipc_exit(void) {
    misc_deregister(&simple_ipc_dev);
    pr_info("simple_ipc: module unloaded\n");
}

module_init(simple_ipc_init);
module_exit(simple_ipc_exit);