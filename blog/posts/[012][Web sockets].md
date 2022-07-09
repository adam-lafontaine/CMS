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

Initialize

```cpp
inline bool os_socket_init()
{	
	WSAData wsa;
	int result = WSAStartup(MAKEWORD(2, 2), &wsa);

	return result == 0;
}
```

Open

```cpp
bool os_socket_open(socket_t& socket_handle)
{
	socket_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	return socket_handle != INVALID_SOCKET;
}
```

Receive bytes

```cpp
inline int os_socket_receive_buffer(socket_t socket, char* dst, int n_bytes)
{
	return recv(socket, dst, n_bytes, 0);
}
```

Send bytes

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


inline bool os_socket_init()
{	
    // no socket initializtion on Linux
	return true;
}


bool os_socket_open(socket_t& socket_handle)
{
	socket_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	return socket_handle >= 0;
}


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


inline bool os_socket_init()
{	
#if defined(_WIN32)

	WSAData wsa;
	int result = WSAStartup(MAKEWORD(2, 2), &wsa);

	return result == 0;

#else

	return true;

#endif
}


bool os_socket_open(socket_t& socket_handle)
{
	socket_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

#if defined(_WIN32)

	return socket_handle != INVALID_SOCKET;

#else

	return socket_handle >= 0;

#endif
}


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

	bool open = false;
	bool bind = false;
	bool listen = false;

	bool server_running = false;
	bool client_connected = false;

	const char* ip_address = "";
};
```

Open

```cpp
inline bool os_server_open(ServerSocketInfo& server_info, int port)
{
	server_info.open = os_socket_open(server_info.server_socket);

	if (server_info.open)
	{
		server_info.server_addr = { 0 };
		server_info.server_addr.sin_family = AF_INET;
		server_info.server_addr.sin_addr.s_addr = INADDR_ANY;
		server_info.server_addr.sin_port = htons(port);
	}	

	return server_info.open;
}
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

### Finding the server's public IP address

Windows

```cpp
inline bool os_find_public_ip(ServerSocketInfo& server_info)
{
	char* ip = nullptr;
	bool found = false;

	char host_name[255];
	PHOSTENT host_info;

	if (gethostname(host_name, sizeof(host_name)) != 0 || (host_info = gethostbyname(host_name)) == NULL)
	{
		return false;
	}

	// TODO: gets last ip in the list?
	int count = 0;	
	while (host_info->h_addr_list[count])
	{
		ip = inet_ntoa(*(struct in_addr*)host_info->h_addr_list[count]);
		found = true;
		++count;
    }

	if (found && ip)
	{
		memcpy(server_info.ip_address, ip, strlen(ip) + 1);
	}

	return found && ip;
}
```

Linux

```cpp
inline bool os_find_public_ip(ServerSocketInfo& server_info)
{
    char* ip = nullptr;
	bool found = false;

    //https://www.binarytides.com/get-local-ip-c-linux/

	FILE* f;
	char line[100];
	char* p = NULL;
	char* c = NULL;

	f = fopen("/proc/net/route", "r");

	while (fgets(line, 100, f))
	{
		p = strtok(line, " \t");
		c = strtok(NULL, " \t");

		if (p != NULL && c != NULL)
		{
			if (strcmp(c, "00000000") == 0)
			{
				m_net_interface = std::string(p);
				break;
			}
		}
	}

	//which family do we require , AF_INET or AF_INET6
	int fm = AF_INET; //AF_INET6
	struct ifaddrs* ifaddr, * ifa;
	int family, s;
	char host[NI_MAXHOST];

	if (getifaddrs(&ifaddr) == -1)
	{
		return false;
	}

	//Walk through linked list, maintaining head pointer so we can free list later
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == NULL)
			continue;

		family = ifa->ifa_addr->sa_family;
		if (strcmp(ifa->ifa_name, p) != 0)
			continue;

		if (family != fm)
			continue;

		auto family_size = (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);

		s = getnameinfo(ifa->ifa_addr, family_size, host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

		if (s != 0)
		{			
			return false;
		}

        found = true;
		ip = host;
	}

	freeifaddrs(ifaddr);

    if (found && ip)
	{
		memcpy(server_info.ip_address, ip, strlen(ip) + 1);
	}

	return found && ip;
}
```

Platform independent

```cpp
inline bool os_find_public_ip(ServerSocketInfo& server_info)
{
	char* ip = nullptr;
	bool found = false;

#if defined(_WIN32)

	char host_name[255];
	PHOSTENT host_info;

	if (gethostname(host_name, sizeof(host_name)) != 0 || (host_info = gethostbyname(host_name)) == NULL)
	{
		return false;
	}

	// TODO: gets last ip in the list?
	int count = 0;	
	while (host_info->h_addr_list[count])
	{
		ip = inet_ntoa(*(struct in_addr*)host_info->h_addr_list[count]);
		found = true;
		++count;
	}		

#else

	//https://www.binarytides.com/get-local-ip-c-linux/

	FILE* f;
	char line[100];
	char* p = NULL;
	char* c = NULL;

	f = fopen("/proc/net/route", "r");

	while (fgets(line, 100, f))
	{
		p = strtok(line, " \t");
		c = strtok(NULL, " \t");

		if (p != NULL && c != NULL)
		{
			if (strcmp(c, "00000000") == 0)
			{
				m_net_interface = std::string(p);
				break;
			}
		}
	}

	//which family do we require , AF_INET or AF_INET6
	int fm = AF_INET; //AF_INET6
	struct ifaddrs* ifaddr, * ifa;
	int family, s;
	char host[NI_MAXHOST];

	if (getifaddrs(&ifaddr) == -1)
	{
		return false;
	}

	//Walk through linked list, maintaining head pointer so we can free list later
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == NULL)
			continue;

		family = ifa->ifa_addr->sa_family;
		if (strcmp(ifa->ifa_name, p) != 0)
			continue;

		if (family != fm)
			continue;

		auto family_size = (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);

		s = getnameinfo(ifa->ifa_addr, family_size, host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

		if (s != 0)
		{
			return false;
		}

		found = true;
		ip = host;
	}

	freeifaddrs(ifaddr);

#endif

	if (found && ip)
	{
		memcpy(server_info.ip_address, ip, strlen(ip) + 1);
	}

	return found && ip;
}
```