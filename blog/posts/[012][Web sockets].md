# Web sockets
## A basic cross platform socket API

Both Windows and Linux provide a C API for communicating with other devices over network.  They are similar to each other but some work needs to be done if you want to provide the same API to the rest of your application.  This post will demonstrate some basic socket functionality and how to hide the differences between system calls.

### Windows

On Windows we need to first initialize the library.

```cpp
#include <WinSock2.h>
#pragma comment (lib,"ws2_32.lib")


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

Send bytes over a socket connection.  Can be called at any time as long as the socket is connected.  The recipient must be ready to receive or the message will be missed.

```cpp
inline bool os_socket_send_buffer(SOCKET socket, const char* src, int n_bytes)
{
	return send(socket, src, n_bytes, 0) != SOCKET_ERROR;
}
```

Make a socket available to receive a message by providing a character buffer for it to write to.  By default the call will wait for a message to arrive and block the thread it is running on.  If the message is larger than the number of bytes provided, the message will be truncated.

```cpp
inline bool os_socket_receive_buffer(SOCKET socket, char* dst, int n_bytes)
{
	return recv(socket, dst, n_bytes, 0) != SOCKET_ERROR;
}
```

When finished with a socket, close it and free all associated resources.

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

### Linux

Linux has similar functions and uses a different convention for the data types.  There is also no need to initialize or cleanup the socket library.

```cpp
#include <unistd.h>
#include <netdb.h>
#include <ifaddrs.h>


bool os_socket_create(int& socket_handle)
{
	socket_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	return socket_handle != -1;
}


inline bool os_socket_send_buffer(int socket, const char* src, int n_bytes)
{
	return send(socket, src, n_bytes, 0) != -1;
}


inline bool os_socket_receive_buffer(int socket, char* dst, int n_bytes)
{
	return recv(socket, dst, n_bytes, 0) != -1;
}


void os_socket_close(int socket)
{
	close(socket);
}
```

We can make all of these functions look identical for both operating systems with some strategic type aliasing and preprocessor definitions.

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

With that, all of the differences between operating systems is taken care of.  What's left is to implement functionality specific to the server and client.  With the type aliasing above, the implentations will be the same for Windows and Linux.

### Server

We'll create a structure to store the current state of a socket server.


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

The first step is to create the socket and set the server address properties if successful.

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

The AF_INET constant indicates that we are using IPv4 addressing.  INADDR_ANY means that the server can use any available IP address on the machine, i.e. any connected network adaptor.

The server socket must be bound to the given IP address(es).

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

In order to accept connections, the socket must be in a listening state.  A backlog amount is used to set the maximum number of client connections.

```cpp
bool os_server_listen(ServerSocketInfo& socket_info)
{
	auto socket = server_info.server_socket;
	int backlog = 1;

	server_info.listen = listen(socket, backlog) != SOCKET_ERROR;

	return server_info.listen;
}
```

The server can accept a pending connection in its backlog, or wait for a client to request a connection if there are no connections available.  By default the call to accept() is blocking.  Execution will stop until a client connection is established.

```cpp
bool os_server_accept(ServerSocketInfo& socket_info)
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

Disconnect from a client by closing the client socket.

```cpp
void os_server_disconnect(ServerSocketInfo& server_info)
{
	os_socket_close(server_info.client_socket);
	server_info.client_connected = false;
}
```

The server socket can be closed when we are done with it.

```cpp
void os_server_close(ServerSocketInfo& server_info)
{	
	os_socket_close(server_info.server_socket);
	server_info.listen = false;
	server_info.bind = false;
	server_info.open = false;
}
```

### Client

Create a structure to store the state of the client socket.

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

Set the server information if the socket is created successfully.  The client needs to know which IP address and port number the server is listening on.

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

Connect to a listening server socket.  A connection will be established if accept() has been called on the server socket.

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

Close the socket to disconnect from the server.

```cpp
void os_client_close(ClientSocketInfo& client_info)
{
	os_socket_close(client_info.client_socket);
	client_info.connected = false;
	client_info.open = false;
}
```

### Sample programs

To demonstrate how these functions work, we'll make a client and a server connect to each other and exchange messages.

The first program creates a server socket and waits for a client connection.  When a connection is made, messages are exchanged and then the server disconnects and shuts down.

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
		return 1;
	}

	if (!os_server_open(server, port))
	{
		printf("server open failed.\n");
		return 1;
	}	

	if (!os_server_bind(server))
	{
		printf("server bind failed.\n");
		return 1;
	}

	if (!os_server_listen(server))
	{
		printf("server listen failed.\n");
		return 1;
	}

	printf("Waiting for client to connect on port %d\n", server.port);
	if (!os_server_accept(server))
	{
		printf("client connect failed.\n");
		return 1;
	}

	printf("Client connected\n");

	char message_buffer[50] = "Hello from server";

	os_socket_send_buffer(server.client_socket, message_buffer, strlen(message_buffer));

	memset(message_buffer, 0, 50);

	os_socket_receive_buffer(server.client_socket, message_buffer, 50);

	printf("recv: %s\n", message_buffer);

	auto c = getchar();

	os_server_disconnect(server);
	os_server_close(server);
	os_socket_cleanup();
}
```

The next program creates a client socket and connects to a waiting server socket.  After the exhange of messages, the client disconnects and shuts down.

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
		return 1;
	}

	if (!os_client_open(client, server_ip_address, server_port))
	{
		printf("client open failed.\n");
		return 1;
	}

	if (!os_client_connect(client))
	{
		printf("client connect failed.\n");
		return 1;
	}

	printf("Client connected.\n");

	char message_buffer[50] = "";

	os_socket_receive_buffer(client.client_socket, message_buffer, 50);

	printf("recv: %s\n", message_buffer);

	memset(message_buffer, 0, 50);

	sprintf(message_buffer, "Hello from client");

	os_socket_send_buffer(client.client_socket, message_buffer, strlen(message_buffer));

	auto c = getchar();

	os_client_close(client);
	os_socket_cleanup();
}
```

When running these, you may get a message like the following from Windows Firewall.

![alt text](https://github.com/adam-lafontaine/CMS/raw/p12-sockets/blog/img/%5B012%5D/firewall.bmp)

Give your application(s) access in order to continue.

Compile both programs separately and start the server program first.  The server will wait until the client program connects and the two programs will exchange messages.

The output from the server should look like so.

```plaintext
Server

Waiting for client to connect on port 58002
Client connected
recv: Hello from client
```

And the output from the client.

```plaintext
Client

Client connected.
recv: Hello from server
```

Try running each program on different machines on the same network.  Be sure that the client has the correct IP address for the server.

### Now for the hard part

Your operating system provides a fairly simple means for allowing devices to communicate over a network.  Differences between operating systems add some complications but this post showed that they are pretty easy to overcome.  The difficult part is getting devices to agree on how to communicate.  Messages can be sent at any time, devices can disconnect at any time, errors can occur at any time.  Each device probably has different software running as well.  There is no way to trust that the device on the other end will behave properly either.

Every application needs to have rules that each device can agree with.  Rules about how to connect and disconnect, what to do when a device disconnects unexpectedly, how messages are structured etc.  Often the communication protocol that you need to implement requires more effort than the core functionality of the application, simply because of all of the edge cases that need to be considered.  Getting this right can take a while but once it works you'll have it forever.

At least the socket part is easy.