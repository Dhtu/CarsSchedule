# 设计文档
## 基本架构
1. 初始化资源，互斥锁
2. 获取输入，每得到一个输入，就在对应方向上的队列push，pop出来以后，调用一个线程。
3. 如果资源都分配出去，就检测死锁，然后北向优先，以避免死锁
## 线程设计
1. 如果右面有车，就让右边先开：右面没车||先行信号
2. 要先获取道路资源，才能继续开
3. 开完释放资源，让左面车先行，以避免饥饿，将队列的下一辆车加进来