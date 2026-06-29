# thread_pool_server (modernized)

This workspace contains:

- `linux_network`: original project (kept intact)
- `thread_pool_server`: a modern C++ thread-pool TCP server example
- `linux_client`: a small client to test the server

Build:

```bash
cd /home/dhzb/worksapce/git_program/linux_network
cmake .
make -j
```

Run thread-pool server:

```bash
./thread_pool_server 127.0.0.1 8888
```

Run client:

```bash
./linux_client/linux_client 127.0.0.1 8888 "hello"
```

Notes:

- The server uses a simple model: main thread accepts connections and hands each socket
	to the thread pool. Each worker drains available data from the socket and echoes it back.
- For production long-lived connections, an `epoll`-based design that reuses worker threads
	for readiness notifications is recommended.
