# Web sockets
## Create a basic cross platform socket API



Windows

```cpp
#include <WinSock2.h>
#pragma comment (lib,"ws2_32.lib")
```

On Windows we need to first initialize the library.

```cpp
bool os_socket_init()
{	
	WSAData wsa;
	int result = WSAStartup(MAKEWORD(2, 2), &wsa);

	return result == 0;
}
```

Create a socket endpoint for communication.

```cpp
bool os_socket_create(SOCKET& socket_handle)
{
	socket_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	return socket_handle != INVALID_SOCKET;
}
```

Make a socket ready to receive by providing a character buffer for it to write to.  By default the call will wait for a message to arrive and block the thread it is running on.  If the message is larger than the number of bytes provided, then the message will be truncated.

```cpp
inline bool os_socket_receive_buffer(SOCKET socket, char* dst, int n_bytes)
{
	return recv(socket, dst, n_bytes, 0) != SOCKET_ERROR;
}
```

Send bytes over a socket connection.  Can be called at any time as long as the socket is connected.  The recipient must ready to receive or the message will be missed.

```cpp
inline bool os_socket_send_buffer(SOCKET socket, const char* src, int n_bytes)
{
	return send(socket, src, n_bytes, 0) != SOCKET_ERROR;
}
```

Disonnect a socket connection and free all associated resources.

```cpp
void os_socket_close(SOCKET socket)
{
	closesocket(socket);
}
```

Cleanup the socket library when finished.

```cpp
void os_socket_cleanup()
{
	WSACleanup();
}
```


Linux has similar functions and uses a different convention for their return types.  There is also no need to initialize and cleanup the socket library.

```cpp
#include <unistd.h>
#include <netdb.h>
#include <ifaddrs.h>


bool os_socket_create(int& socket_handle)
{
	socket_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	return socket_handle >= 0;
}


inline bool os_socket_receive_buffer(int socket, char* dst, int n_bytes)
{
	return recv(socket, dst, n_bytes, 0) >= 0;
}


inline bool os_socket_send_buffer(int socket, const char* src, int n_bytes)
{
	return send(socket, src, n_bytes, 0) >= 0;
}


void os_socket_close(int socket)
{
	close(socket);
}
```

We can make all of these functions look identical for both operating systems with some strategic type aliasing preprocessor macros.

```cpp
#if defined(_WIN32)

#include <WinSock2.h>
#pragma comment (lib,"ws2_32.lib")

using socket_t = SOCKET;
using addr_t = SOCKADDR;
using socklen_t = int;

#else

#include <unistd.h>
#include <netdb.h>
#include <ifaddrs.h>

using socket_t = int;
using addr_t = struct sockaddr;

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

#endif


bool os_socket_init()
{	
#if defined(_WIN32)

	WSAData wsa;
	int result = WSAStartup(MAKEWORD(2, 2), &wsa);

	return result == 0;

#else

	return true;

#endif
}


bool os_socket_create(socket_t& socket_handle)
{
	socket_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	return socket_handle != INVALID_SOCKET;
}


bool os_socket_receive_buffer(socket_t socket, char* dst, int n_bytes)
{
	return recv(socket, dst, n_bytes, 0) != SOCKET_ERROR;
}


bool os_socket_send_buffer(socket_t socket, const char* src, int n_bytes)
{
	return send(socket, src, n_bytes, 0) != SOCKET_ERROR;
}


void os_socket_close(socket_t socket)
{
#if defined(_WIN32)

	closesocket(socket);

#else

	close(socket);

#endif
}


void os_socket_cleanup()
{
#if defined(_WIN32)

	WSACleanup();

#endif
}
```

With that, all of the differences between operating systems is taken care of.  What's left is to implement functionality specific to the server and client.

### Server



```cpp
class ServerSocketInfo
{
public:

	socket_t server_socket = 0;
	socket_t client_socket = 0;

	struct sockaddr_in server_addr = { 0 };
	struct sockaddr_in client_addr = { 0 };

	socklen_t client_len;

	bool open = false;
	bool bind = false;
	bool listen = false;

	bool server_running = false;
	bool client_connected = false;

	char ip_address[20];
	int port;
};
```

Open

```cpp
bool os_server_open(ServerSocketInfo& server_info, int port)
{
	server_info.open = os_socket_create(server_info.server_socket);

	if (server_info.open)
	{
		server_info.server_addr = { 0 };
		server_info.server_addr.sin_family = AF_INET;
		server_info.server_addr.sin_addr.s_addr = INADDR_ANY;
		server_info.server_addr.sin_port = htons(port);

		server_info.port = port;
	}	

	return server_info.open;
}
```

Bind

```cpp
bool os_server_bind(ServerSocketInfo& server_info)
{
	auto socket = server_info.server_socket;
	auto addr = (addr_t*)&server_info.server_addr;
	int size = sizeof(server_info.server_addr);

	server_info.bind = bind(socket, addr, size) != SOCKET_ERROR;

	return server_info.bind;
}
```

Listen

```cpp
bool os_socket_listen(ServerSocketInfo& socket_info)
{
	auto socket = server_info.server_socket;
	int backlog = 1;

	server_info.listen = listen(socket, backlog) != SOCKET_ERROR;

	return server_info.listen;
}
```

Accept

```cpp
bool os_socket_accept(ServerSocketInfo& socket_info)
{
	server_info.client_len = sizeof(server_info.client_addr);

	server_info.client_connected = false;

	auto srv_socket = server_info.server_socket;
	auto cli_addr = (addr_t*)&server_info.client_addr;
	auto cli_len = &server_info.client_len;

	server_info.client_socket = accept(srv_socket, cli_addr, cli_len);

	server_info.client_connected = server_info.client_socket != INVALID_SOCKET;

	return server_info.client_connected;
}
```

### Client

```cpp
class ClientSocketInfo
{
public:

	sockaddr_in server_addr = { 0 };
	socket_t client_socket = 0;

	bool open = false;
	bool connected = false;	
};
```

Open

```cpp
bool os_client_open(ClientSocketInfo& client_info, const char* server_ip, int server_port)
{
	client_info.open = os_socket_create(client_info.client_socket);

	if (client_info.open)
	{
		client_info.server_addr = { 0 };
		client_info.server_addr.sin_family = AF_INET;
		client_info.server_addr.sin_addr.s_addr = inet_addr(server_ip);
		client_info.server_addr.sin_port = htons(server_port);
	}

	return client_info.open;
}
```

Connect

```cpp
bool os_client_connect(ClientSocketInfo& client_info)
{
	auto socket = client_info.client_socket;
	auto addr = (addr_t*)&client_info.server_addr;
	int size = sizeof(client_info.server_addr);

	client_info.connected = connect(socket, addr, size) != SOCKET_ERROR;

	return client_info.connected;
}
```

Server program

```cpp
#include <cstdio>


void run_server()
{
	int port = 58002;

	ServerSocketInfo server{};

	printf("\nServer\n\n");

	if (!os_socket_init())
	{
		printf("socket init failed.\n");
		return -1;
	}

	if (!os_server_open(server, port))
	{
		printf("server open failed.\n");
		return -1;
	}	

	if (!os_server_bind(server))
	{
		printf("server bind failed.\n");
		return -1;
	}

	if (!os_server_listen(server))
	{
		printf("server listen failed.\n");
		return -1;
	}

	printf("Waiting for client to connect on port %d\n", server.port);
	if (!os_server_accept(server))
	{
		printf("client connect failed.\n");
		return -1;
	}

	printf("Client connected\n");

	char message_buffer[50] = "hello from server";

	os_socket_send_buffer(server.client_socket, message_buffer, strlen(message_buffer));

	memset(message_buffer, 0, 50);

	os_socket_receive_buffer(server.client_socket, message_buffer, 50);

	printf("recv: %s\n", message_buffer);

	auto c = getchar();

	os_socket_close(server.client_socket);
	os_socket_close(server.server_socket);
	os_socket_cleanup();
}
```

Client program

```cpp
#include <cstdio>

#if defined(_WIN32)

#define sprintf sprintf_s

#endif


void run_client()
{
	int server_port = 58002;
	const char* server_ip_address = "127.0.0.1";

	ClientSocketInfo client{};

	printf("\nClient\n\n");

	if (!os_socket_init())
	{
		printf("socket init failed.\n");
		return -1;
	}

	if (!os_client_open(client, server_ip_address, server_port))
	{
		printf("client open failed.\n");
		return -1;
	}

	if (!os_client_connect(client))
	{
		printf("client connect failed.\n");
		return -1;
	}

	printf("Client connected.\n");

	char message_buffer[50] = "";

	os_socket_receive_buffer(client.client_socket, message_buffer, 50);

	printf("recv: %s\n", message_buffer);

	memset(message_buffer, 0, 50);

	sprintf(message_buffer, "Hello from client");

	os_socket_send_buffer(client.client_socket, message_buffer, strlen(message_buffer));

	auto c = getchar();

	os_socket_close(client.client_socket);
	os_socket_cleanup();
}
```

Firewall

![alt text](https://github.com/adam-lafontaine/CMS/raw/p12-sockets/blog/img/%5B012%5D/firewall.bmp)

Server output

```plaintext
Server

Waiting for client to connect on port 58002
Client connected
recv: Hello from client
```

Client output

```plaintext
Client

Client connected.
recv: hello from server
```