# 一条 SQL 的执行过程

主要总结系统到底如何和MySQL交互，MySQL在接受到我们发送的 SQL 语句时分别做了哪些事情

## MySQL 驱动

当我们系统在和 MySQL 进行通信的时候，MySQL 驱动在底层帮助我们做了对数据库的连接，才有了后面的交互。
这样的话，在系统和 MySQL 进行交互之前，MySQL 驱动会帮我们建立好连接，然后我们只需要发送 SQL 语句就可以执行 CRUD 了。一次 SQL 请求就会建立一个连接，多个请求就会建立多个连接。
我们系统肯定是存在多个请求同时争抢连接的情况，我们的 web 系统一般都是部署在 tomcat 容器中，而 tomcat 是可以并发处理多个请求，这就会导致多个请求建立多个连接，然后使用完再都去关闭。
许多系统在通过 MySQL 驱动和 MySQL 数据库连接的时候是基于 TCP/IP 协议的，如果每个请求都是新建连接和销毁连接，那这样势必会造成不必要的浪费和性能的下降，也就是说多线程请求的时候频繁的创建和销毁连接显然是不合理的，必然会降低我们系统的性能，但是如果提供一些固定的用来连接的线程，就不需要反复的创建和销毁线程了，也就是下面提到的数据库连接池。

## 数据库连接池

数据库连接池：维护一定的连接数，方便系统获取连接，使用时就去池子中获取，用完放回池子中，我们不需要关心连接的创建与销毁，也不需要关心线程池是怎么维护这些连接
