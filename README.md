# axnet

- A C++ non-blocking network library and it's is just my personal practice work
- Reactor model, i.e., one main reactor for accepting new connection and several sub-reactors(configurable) for handling accepted connections, plus thread pool support for CPU-consuming tasks (Use level-triggered epoll. Use eventfd for asynchronous wakeup of threads)
- Implement both TcpServer and TcpClient (Use std::enable_shared_from_this for life cycle management)
- Implement Thread pool
- Implement a simple and easy-to-use buffer class (Its interface is not perfect yet)
- Use boost.log for logging (It's too slow. At first I chose it just for trying. Now it need to be replaced by another logging library or manually implemented)

## Performance Tests

Tests show that axnet and muduo performs almost the same. Here is the detailed test information:

Test environment:

- VMware Workstation 12 Pro
- Intel(R) Core(TM) i3-6100
- 4G RAM + 4G swap
- Linux ubuntu 4.13.0-39-generic #44~16.04.1-Ubuntu SMP

pingpong test:

With 1 server thread(only 1 main reactor), 1 client thread, message block size of 4096 bytes, test results of different concurrent connection number:

Concurrent connections: 1

```
axnet - 52.1479 MiB/s
muduo - 53.3362630208 MiB/s
```

Concurrent connections: 10

```
axnet - 668.284 MiB/s
muduo - 727.059960938 MiB/s
```

Concurrent connections: 100

```
axnet - 626.058 MiB/s
muduo - 644.079492188 MiB/s
```

With 3 server threads(1 main reactor + 2 sub-reactors), 2 clients threads, message block size of 4096 bytes, test results of different concurrent connection number:

Concurrent connections: 10

```
axnet - 882.598 MiB/s
muduo - 953.319466146 MiB/s
```

Concurrent connections: 100

```
axnet - 860.353 MiB/s
muduo - 884.478059896 MiB/s
```

Fake http test (I don't want to write another http parser so I use a busy loop to pretend the http request parsing and return a fixed http response):

With 1 server thread(only one main loop), test results of different concurrent connection number:

concurrent connections: 100

```
axnet - 8837 req/s
muduo - 8855 req/s
```

concurrent connections: 1000

```
axnet - 5907 req/s
muduo - 6230 req/s
```

concurrent connections: 5000

```
axnet - 4232 req/s
muduo - 4174 req/s
```

With 4 server thread(one main reactor + 4 sub-reactors), test results of different concurrent connection number:

concurrent connections: 100

```
axnet - 14777 req/s
muduo - 14733 req/s
```

concurrent connections: 1000

```
axnet - 27512 req/s
muduo - 28028 req/s
```

concurrent connections: 5000

```
axnet - 23119 req/s
muduo - 22317 req/s
```

## TODO

- Replace boost.log with another logging library or implement a new one
- Complete the interface of the Buffer class
- Timer
