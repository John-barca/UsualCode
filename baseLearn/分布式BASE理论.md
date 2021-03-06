# 分布式系统 BASE 理论

## 基本可用
基本可用主要包括三点:
1. 目前系统能承受的压力范围内的请求是正常处理的，超过范围的可能处理不正常，但是不能因为这些请求影响正常的请求。
2. 响应时间正常，在压力大的时候，响应时间可能增长，但是不能超过一定时间。
3. 可以忍受一定的功能损失，在某些系统出问题的时候，其他系统应该保证本身业务接口正常的运行。

### 目前系统能承受的压力范围内的请求是正常处理的
在双 11 的时候，用户量会突然增高，并且这个量很难预测，尤其是商品下单的业务压力会增大很多，可能超过系统的极限。在超过系统极限的时候，可能会导致整个系统都不可用（例如超过超时极限导致很多请求在队列排队导致后续请求也排队从而雪崩），这是不可取的。
所以，一般会考虑动态扩容，根据业务压力，进行相应的系统微服务的扩容。并且由于动态扩容一般具有滞后性，还需要加上限流器，对某些敏感业务请求根据微服务数量进行动态限制，防止雪崩的出现。

### 响应时间正常
在闲时，用户请求下单可能是毫秒级返回，在系统压力大的时候，这个响应时间可以适当加大，例如 3s 返回。**但是这个响应超时时间不要设置太长**，第一影响用户体验，第二会导致更多请求排队。
**对于不同的业务接口最好设置不同的响应超时时间，**例如查询账户余额超时时间短一些，查看用户订单可以稍微长一些。
可以根据系统接口的平均响应时间，请求的个数，制定系统扩容策略。

### 可以忍受一定的功能损失
这个主要在于微服务的拆分，以及服务降级策略
比如现在商品微服务因为压力过大挂了，或者重启，这时候的业务：
1. 查询账户余额(涉及账户微服务)，这个由于微服务隔离不受影响
2. 查询商品剩余库存（涉及商品微服务），这个系统挂了，需要服务降级，例如返回商品库存加载中（前端定时查询），或者商品库存为 0。
3. 商品下单，目前不可用
4. 查看用户的订单，可能里面的商品信息显示不出来(服务降级)，但是订单列表，以及其中的订单金额、交易时间信息等等应该可以展示，而不是整个接口报异常。

## 软状态
指的是允许系统中的数据存在中间状态，并认为该状态不影响系统的整体可用性，即允许系统在多个不同节点的数据副本存在数据延时。

例如下单，订单可以有 初始化 -> 已冻结金额 -> 已扣库存 -> 已扣款 -> 下单成功 这几个状态，如果扣库存失败则变为 待解冻金额 -> 下单失败 状态，分别对应下单的几个中间逻辑。除了下单成功与下单失败以外，其他的都是软状态，并不是最终状态。基本上所有的软状态都需要对应的补偿任务。有了这些中间状态，即使系统重启，也可以通过这些中间状态补偿重试将下单逻辑走下去。

查询订单的时候，在已冻结金额状态之后的订单就可以展示给用户看了，在下单成功之前，都可以显示为处理中。在扣库存失败之后，可以显示为退款中。

## 最终一致性
上面提到的软状态，并不会一直保持在这个软状态，而是最终通过系统正常逻辑或者补偿逻辑走向最终状态，各个节点也会同步这个最终状态，这就是最终一致性。
五种实现最终一致性的不同场景方法，分别是：
- Causal consistency (因果一致性)
- Read-your-writes consistency (读你所写一致性)
- Session consistency (会话级别一致性)
- Monotonic read consistency (单词读一致性)
- Monotonic write consistency (单词写一致性)
在实际的实践中，这 5 种设计方案往往会互相结合使用，以构建一个具有最终一致性的分布式系统。

### Causal consistency (因果一致性)
因果关系的意思是进程 A 的修改会通知进程 B，没有因果一致性的就是进程 A 的修改不会通知进程 C。对于有因果关系的进程，他们访问的数据应该是强一致性的，没有这个关系的走的是最终一致性。
举一个例子，用户查看商品库存的请求是非常巨大的，并且能接受一点时间内的不一致。假设为了应对这么大的请求，这个库存在商品微服务实例本地也缓存了一份，并且会随着收到下单请求就会将缓存失效重新获取扣库存后最新的库存量，同时这个缓存 5s 后一定会过期（真正扣库存不会使用这个缓存，这个仅仅展示用）。那么下单请求通过订单微服务 RPC 请求的商品微服务实例上面的库存就是最新的库存，其他实例的库存可能有点滞后性。

### Read-your-writes consistency（读你所写一致性）
进程 A 在更新了数据项之后，总是访问更新的值，永远不会看到旧的值。这是因果一致性模型的一个特例。

### Session consistency（会话级别一致性）
在同一个会话内的修改，是可以立刻读取到最终修改的。在其他会话可能不会立刻读取到。

例如数据库的主从同步，请求在这个会话内写入主数据库，那么主数据库会话能立刻读取到这个修改，根据数据库的双 1 同步配置，默认情况下，只有同步完成后才能读取到这个修改。

### Monotonic read consistency（单调读一致性）
如果一个节点从系统中读取出一个数据项的某个值后，那么系统对于该节点后续的任何数据访问都不应该返回更旧版本的值。

###  Monotonic write consistency（单调写一致性）
系统要保证同一个节点的写操作是顺序执行的。