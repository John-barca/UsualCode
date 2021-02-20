# Redis 相关学习
## Redis 为什么是单线程
Redis是单线程，主要是指 Redis 的网络 IO 和键值对读写是由一个线程来完成的，这也是 Redis 对外提供键值存储服务的主要流程。但是 redis 的其他功能，比如持久化、异步删除、集群数据同步等，其实是由额外的线程执行的。通常来说，单线程的处理能力要比多线程差很多，但是 Redis 却能使用单线程模型达到每秒数十万级别的处理能力，这是 redis 多方面设计选择的一个综合结果。
1. 纯内存操作
2. 高效的数据结构，例如哈希表和跳表
3. 多路复用机制
cpu 以及内存读写的速度足够快，以至于一个线程便支持了 100W QPS。很多时候，瓶颈是内存大小及带宽。也就是说，即便支持了多线程，支持了更高的 QPS，更有可能的是你的网卡带宽先被打满了。
单线程机制在进行 sunion 之类的比较耗时的命令时会使 redis 的并发下降。因为是单一线程，所以同一时刻只有一个操作在进行，所以，耗时的命令会导致并发的下降，不只是读并发，写并发也会下降。
**为什么说 Redis 是单线程的以及 Redis 为什么这么快！**我们不能任由操作系统负载均衡，因为我们自己更了解自己的程序，所以，我们可以手动地为其分配 CPU 核，而不会过多的占用 CPU。单线程也有单线程的好处，比如可以基于 redis 实现自增 id 服务。

## Redis 设计与实现
1. Redis 的五种数据类型分别是由什么数据结构实现的?
2. Redis 的字符串数据类型既可以储存字符串（比如 “hello world” ）， 又可以储存整数和浮点数（比如 10086 和 3.14 ）， 甚至是二进制位（使用 SETBIT 等命令）， Redis 在内部是怎样储存这些不同的值的
3. Redis 的一部分命令只能对特定数据类型执行（比如 APPEND 只能对字符串执行， HSET 只能对哈希表执行）， 而另一部分命令却可以对所有数据类型执行（比如 DEL 、 TYPE 和 EXPIRE ）， 不同的命令在执行时是如何进行类型检查的？ Redis 在内部是否实现了一个类型系统
4. Redis 的数据库是怎样储存各种不同数据类型的键值对的？ 数据库里面的过期键又是怎样实现自动删除的?
5. 除了数据库之外，Redis 还拥有发布与订阅、脚本、事务等特性， 这些特性又是如何实现的
6. Redis 使用什么模型或者模式来处理客户端的命令请求？ **一条命令请求从发送到返回需要经过什么步骤？**

## RESP 协议
Redis 协议在以下三个目标之间进行折中：
1. 易于实现
2. 可以高效地被计算机分析(parse)
3. 可以很容易地被人类读懂

Redis 客户端和服务端之间使用一种名为 RESP 的二进制安全文本协议进行通信。
客户端和服务器通过 TCP 连接来进行数据交互，服务器默认的端口号为 6379。客户端和服务器发送的命令或数据一律以 \r\n (CRLF) 结尾
**请求**
请求协议的一般形式
```
*<参数数量> CR LF
$<参数 1 的字节数量> CR LF
<参数 1 的数据> CR LF
...
$<参数 N 的字节数量> CR LF
<参数 N 的数据> CR LF
```
比如对于命令 set mykey myvalue，实际协议值 "*3\r\n$3\r\nSET\r\n$5\r\nmykey\r\n$7\r\nmyvalue\r\n"
print出来是
```
*3          // 表示3个参数
$3          // 表示第一个参数长度为3
SET         // set 命令
$5          // 表示第二个参数长度为5
mykey       // 第二个参数值
$7          // 表示第三个参数长度为7
myvalue     // 表示第三个参数值
```
**回复**
多条批量回复是由多个回复组成的数组，数组中的每个元素都可以是任意类型的回复，包括多条批量回复本身。

## 数据结构

**redis 是一个内存数据库、非关系型数据库、支持多种数据结构**，丰富的数据类型，带来丰富的命令，以字符串为例，一般编程语言对字符串提供多少方法，redis就大体支持多少字符串操作命令，比如 append 等，就好像本地内存提供的操作能力一样（实际上更多，比如过期、订阅等），除了 redisTemplate 封了一下网络访问外没啥区别
```
String msg = "Hello world";
String msg = redisTemplate.opsForValue().get("msg");
String msg = "hello " + "world";
redisTemplate.opsForValue().append(msg,"world");
int msg = 1;
String msg = redisTemplate.opsForValue().get("msg");
int msg = msg + 1;
int msg = redisTemplate.opsForValue().increment("msg",1);
```
命令与数据类型有对应（多对多）关系（比如set处理不了集合），数据类型根据不同情况可以使用不同的数据结构来存储。
list 的实现方式是在代码中直接指定的。而 redis 的 list 会自动根据元素特点来决定使用 ziplist 或 linkedlist

说到 Redis 的数据结构，一般指 Redis 的5种常见数据结构：字符串(String)、列表(List)、散列(Hash)、集合(Set)、有序集合(Sorted Set)，以及他们的特点和运用场景。不过它们是 Redis 对外暴露的数据结构，用于 API 的操作，下面来看一看组成它们的底层数据结构
- 简单动态字符串(SDS)
- 链表
- 字典
- 跳跃表
- 整数集合
- 压缩列表

### 简单动态字符串(SDS
Redis 使用 C 语言实现的，但是 Redis 并没有使用 C 的字符串表示 (C字符串是以 \0 空字符结尾的字符数组)，而是自己在底层构建了一种简单动态字符串(simple dynamic string, SDS)的抽象类型，并作为 Redis 的默认字符串表示

在 Redis 中，包含字符串值的键值对底层都是用 SDS 实现的。

#### SDS的定义

SDS的结构定义在 sds.h 文件中，SDS的定义在 Redis 3.2 版本之后有一些改变，由一种数据结构变成了5种数据结构，会根据 SDS 存储的内容长度来选择不同的结构，以达到节省内存的效果，具体结构定义，我们看以下代码：
```
// 3.0 
struct sdshdr {
    // 记录 buf 数组中已使用字节的数量，即 SDS 所保存字符串的长度
    unsigned int len;
    // 记录 buf 数组中未使用的字节数量
    unsigned int free;
    // 字节数组，用于保存字符串
    char buf[];
};

// 3.2
struct __attribute__ ((__packed__)) sdshdr5 {
    unsigned char flags;
    char buf[];
};

struct __attribute__ ((__packed__)) sdshdr8 {
    uint8_t len;
    uint8_t alloc;
    unsigned char flags;
    char buf[];
};

struct __attribute__ ((__packed__)) sdshdr16 {
    uint16_t len;
    uint16_t alloc;
    unsigned char flags;
    char buf[];
};

struct __attribute__ ((__packed__)) sdshdr32 {
    uint32_t len;
    uint16_t alloc;
    unsigned char flags;
    char buf[];
};

struct __attribute__ ((__packed__)) sdshdr64 {
    uint32_t len;
    uint16_t alloc;
    unsigned char flags;
    char buf[];
};
```
3.2版本之后，会根据字符串的长度来选择对应的数据结构
```
static inline char sdsReqType(size_t string_size) {
    if (string_size < 1 << 5) 
        return SDS_TYPE_5;
    if (string_size < 1 << 8) 
        return SDS_TYPE_8;
    if (string_size < 1 << 16) 
        return SDS_TYPE_16;
    if (string_size < 1 << 32) 
        return SDS_TYPE_32;
    return SDS_TYPE_64;
}
```
下面是3.2版本的 sdshdr8 一个示例: len为5，alloc为8，flags为1，buf[]为|'R'|'e'|'d'|'i'|'s'|'\0'| | | |。空白处表示未使用空间，是 Redis 优化的空间策略，给字符串的操作留有余地，保证安全提高效率。
- len：记录当前已使用的字节数(不包括'\0')，获取 SDS 长度的复杂度 O(1)
- alloc：记录当前字节数组总共分配的字节数量(不包括'\0')
- flags：标记当前字节数组的属性，是 sdshdr8 还是 sdshdr16 等，flags 值的定义可以看下面代码
- buf：字节数组，用于保存字符串，包括结尾空白字符 '\0'
```
// flags 值定义
#define SDS_TYPE_5 0
#define SDS_TYPE_8 1
#define SDS_TYPE_16 2
#define SDS_TYPE_32 3
#define SDS_TYPE_64 4
```

#### SDS 与 C 字符串的区别
C语言使用长度为 N + 1 的字符数组来表示长度为 N 的字符串，字符数组的最后一个元素为空字符'\0'，但是这种简单的