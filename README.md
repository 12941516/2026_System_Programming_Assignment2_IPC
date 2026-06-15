# System Programming Assignment3(IPC: Server-Client)
## Student: 소프트웨어학과 202422117 금민기
### 실행 과정들
### 1. Build
```bash
~$ cd 202422117
~$ make clean
~$ make
```

### 2. Module Load/Unload
```bash
~$ cd 202422117
~/202422117$ sudo insmod .build/simipc.ko   # Load
~/202422117$ sudo rmmod simipc              # Unload
```

### 3. Run Server/Client Processes
```bash
~$ cd 202422117
~/202422117$ .build/server KV_STORE
```
```bash
~$ cd 202422117
~/202422117$ .build/client KV_STORE PUT [Key] [Value]    # PUT
~/202422117$ .build/client KV_STORE GET [Key] [Value]    # GET
```

### 4. Examples

#### server 먼저 실행
```bash
~$ cd 202422117
~/202422117$ .build/server KV_STORE
```
#### client PUT 실행(Key: "name", Value: "kim")과 결과
```bash
~$ cd 202422117
~/202422117$ .build/client KV_STORE PUT name kim
```
#### server의 PUT Request 수신 결과
```bash
PUT K: name V: kim
```
#### client GET 실행(Key: "name")과 결과
```bash
~/202422117$ .build/client KV_STORE GET name
kim
```
#### server의 GET Request 수신 결과
```bash
GET K: name V: kim
```
