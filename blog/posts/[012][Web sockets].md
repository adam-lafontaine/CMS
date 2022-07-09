# Web sockets
## TODO

Windows

```cpp
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#pragma comment (lib,"ws2_32.lib")

using socket_t = SOCKET;
using addr_t = SOCKADDR;
```

Send bytes

```cpp
inline int os_socket_receive_buffer(socket_t socket, char* dst, int n_bytes)
{
	return recv(socket, dst, n_bytes, 0);
}
```

Receive bytes

```cpp
inline int os_socket_send_buffer(socket_t socket, const char* src, int n_bytes)
{
	return send(socket, src, n_bytes, 0);
}
```

Close socket

```cpp
inline void os_socket_close(socket_t socket)
{
	closesocket(socket);
}
```

Cleanup socket resources

```cpp
inline void os_socket_cleanup()
{
	WSACleanup();
}
```


Linux

```cpp
#include <unistd.h>
#include <netdb.h>
#include <ifaddrs.h>

using socket_t = int;
using addr_t = struct sockaddr;


inline int os_socket_receive_buffer(socket_t socket, char* dst, int n_bytes)
{
	return read(socket, dst, n_bytes - 1);
}


inline int os_socket_send_buffer(socket_t socket, const char* src, int n_bytes)
{
	return write(socket, src, n_bytes);
}


inline void os_socket_close(socket_t socket)
{
	close(socket);
}


inline void os_socket_cleanup()
{
	// Do nothing
    // Linux has no socket cleanup
}
```

Cross platform API

```cpp
#if defined(_WIN32)

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#pragma comment (lib,"ws2_32.lib")

using socket_t = SOCKET;
using addr_t = SOCKADDR;

#else

#include <unistd.h>
#include <netdb.h>
#include <ifaddrs.h>

using socket_t = int;
using addr_t = struct sockaddr;

#endif


inline int os_socket_receive_buffer(socket_t socket, char* dst, int n_bytes)
{
#if defined(_WIN32)

	return recv(socket, dst, n_bytes, 0);

#else

	return read(socket, dst, n_bytes - 1);

#endif
}


inline int os_socket_send_buffer(socket_t socket, const char* src, int n_bytes)
{
#if defined(_WIN32)

	return send(socket, src, n_bytes, 0);

#else

	return write(socket, src, n_bytes);

#endif
}


inline void os_socket_close(socket_t socket)
{
#if defined(_WIN32)

	closesocket(socket);

#else

	close(socket);

#endif
}


inline void os_socket_cleanup()
{
#if defined(_WIN32)

	WSACleanup();

#else

	// Do nothing
	// Linux has no socket cleanup

#endif
}
```

### Server

```cpp
class ServerSocketInfo
{
public:

	socket_t server_socket = NULL;
	socket_t client_socket = NULL;

	struct sockaddr_in server_addr = { 0 };
	struct sockaddr_in client_addr = { 0 };

	socklen_t client_len;

	bool bind = false;
	bool listen = false;

	bool server_running = false;
	bool client_connected = false;

	const char* ip_address = "";


	ServerSocketInfo(int port)
	{
		server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		server_addr = { 0 };
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = INADDR_ANY;
		server_addr.sin_port = htons(port);
	}
};
```

Bind

```cpp
inline bool os_socket_bind(ServerSocketInfo& socket_info)
{
	auto socket = socket_info.server_socket;
	auto addr = (addr_t*)&socket_info.server_addr;
	int size = sizeof(socket_info.server_addr);

#if defined(_WIN32)

	socket_info.bind = bind(socket, addr, size) != SOCKET_ERROR;

#else

	socket_info.bind = bind(socket, addr, size) >= 0;

#endif

	return socket_info.bind;
}
```

Listen

```cpp
inline bool os_socket_listen(ServerSocketInfo& socket_info)
{
	auto socket = socket_info.server_socket;
	int backlog = 1;

#if defined(_WIN32)

	socket_info.listen = listen(socket, backlog) != SOCKET_ERROR;

#else

	socket_info.listen = listen(socket, backlog) >= 0;

#endif

	return socket_info.listen;
}
```

Accept

```cpp
inline bool os_socket_accept(ServerSocketInfo& socket_info)
{
	socket_info.client_len = sizeof(socket_info.client_addr);

	socket_info.client_connected = false;

	auto srv_socket = socket_info.server_socket;
	auto cli_addr = (addr_t*)&socket_info.client_addr;
	auto cli_len = &socket_info.client_len;

#if defined(_WIN32)

	socket_info.client_socket = SOCKET_ERROR;

	while (socket_info.client_socket == SOCKET_ERROR)
	{
		socket_info.client_socket = accept(srv_socket, cli_addr, cli_len);
	}

	socket_info.client_connected = true;

#else

	// waits for client to connect        
	socket_info.client_socket = accept(srv_socket, cli_addr, cli_len);

	socket_info.client_connected = socket_info.client_socket >= 0;

#endif

	return socket_info.client_connected;
}
```