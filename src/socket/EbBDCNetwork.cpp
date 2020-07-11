#include "EbBDCNetwork.h"
#include "winerror.h"
#include <WinSock2.h>
#if defined(_WIN32)
#include <WS2tcpip.h>
#pragma warning(disable:4996)
#endif

int g_NetInit = 0;

BDCNetECode BDCNetStartup()
{
	if (g_NetInit == 1)
	{
		return NET_SUCCEED;
	}
#if defined(_WIN32)
	WSAData wsaData;
	int rst = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (rst == 0)
	{
		g_NetInit = 1;
	}
	else
	{
		g_NetInit = 0;
	}
	return BDCNetECodeMap(rst);
#endif
}

BDCNetECode BDCNetClose()
{
	if (g_NetInit == 0)
	{
		return NET_SUCCEED;
	}
	g_NetInit = 0;
#if defined(_WIN32)
	int closeRst = WSACleanup();
	return BDCNetECodeMap(closeRst);
#endif
}

BDCNetECode BDCCreateSocket(NET_SOCKET& sock, NetProtocol proto)
{
	// not implement ipv6
	switch (proto)
	{
	case NET_PROTO_TCP:
	{
		sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock == INVALID_SOCKET)
		{
			return BDCNetECodeMap(WSAGetLastError());
		}
		return NET_SUCCEED;
	}
	case NET_PROTO_UDP:
	{
		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock == INVALID_SOCKET)
		{
			return BDCNetECodeMap(WSAGetLastError());
		}
		return NET_SUCCEED;
	}
	default:
	{
		return NET_EUNDEFINE;
	}
	}
}

int BDCCloseSocket(NET_SOCKET sock)
{
#if defined(_WIN32)
	shutdown(sock, SD_BOTH);
	return closesocket(sock);
#endif
}

BDCNetECode BDCNetBind(NET_SOCKET sock, NetAddressIpv4& addr, unsigned int option)
{
	if (sock == INVALID_SOCKET)
	{
		return NET_ESOCKINIT;
	}
	if (bind(sock, (struct sockaddr*)&addr.addrV4, sizeof(addr.addrV4)) == SOCKET_ERROR)
	{
		return BDCNetECodeMap(WSAGetLastError());
	}
	return NET_SUCCEED;
}

BDCNetECode BDCNetListen(NET_SOCKET sock, int maxQueueLen, unsigned int option)
{
	if (sock == INVALID_SOCKET)
	{
		return NET_ESOCKINIT;
	}
	if (listen(sock, maxQueueLen) < 0)
	{
		return BDCNetECodeMap(WSAGetLastError());
	}
	return NET_SUCCEED;
}

BDCNetECode BDCNetConnect(NET_SOCKET sock, NetAddressIpv4& addr, unsigned int option)
{
	if (sock == INVALID_SOCKET)
	{
		return NET_ESOCKINIT;
	}
	if (connect(sock, (struct sockaddr*) & addr.addrV4, sizeof(addr.addrV4)) == SOCKET_ERROR)
	{
		return BDCNetECodeMap(WSAGetLastError());
	}
	return NET_SUCCEED;
}

BDCNetECode BDCNetAccept(NET_SOCKET sock, NetAddressIpv4& newAddr, unsigned int option)
{
#if defined(_WIN32)
	int sysAddrSize = sizeof(newAddr.addrV4);
	NET_SOCKET newSock = accept(sock, (struct sockaddr*) & (newAddr.addrV4), &sysAddrSize);
#else
	unsigned int sysAddrSize = sizeof(addr.addrV4);
	NET_SOCKET newSock = accept(sock, (struct sockaddr*) & (newAddr.addrV4), (socklen_t*)&sysAddrSize);
#endif
	if (sock == INVALID_SOCKET)
	{
		return BDCNetECodeMap(WSAGetLastError());
	}
	if ((option & NET_TCP_NODELAY) != 0)
	{
		// no delay
		int opt = 1;
		setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(opt));
	}
	return NET_SUCCEED;
}

BDCNetECode BDCNetSelect(SocketList sockList, size_t sockCount, NetTimeVal *timeout, SocketList& activeSockList, size_t& activeCount, NetOption option)
{
	NET_fd_set rdSet;
	NET_fd_set excepSet;
	timeval timeoutVal;
	timeval* timeoutPtr;
	NET_FD_ZERO(&rdSet);
	NET_FD_ZERO(&excepSet);
	NET_SOCKET maxSock = 0;
	size_t index = 0;
	for (index = 0; index < sockCount; ++index)
	{
		if (sockList[index] == INVALID_SOCKET)
		{
			continue;
		}
		NET_FD_SET(sockList[index], &rdSet);
		NET_FD_SET(sockList[index], &excepSet);
		if (sockList[index] > maxSock)
		{
			maxSock = sockList[index];
		}
	}
	if (timeout != NULL)
	{
		timeoutVal.tv_sec = timeout->sec;
		timeoutVal.tv_usec = timeout->nanosec / 1000;
		timeoutPtr = &timeoutVal;
	}
	else
	{
		timeoutPtr = NULL;
	}
	if (option == NET_NOPT_WRSOCKET)
	{
		if (select(maxSock + 1, NULL, &rdSet, &excepSet, timeoutPtr) == SOCKET_ERROR)
		{
			return BDCNetECodeMap(WSAGetLastError());
		}
	}
	else if (option == NET_NOPT_RDSOCKET)
	{
		if (select(maxSock + 1, &rdSet, NULL, &excepSet, timeoutPtr) == SOCKET_ERROR)
		{
			return BDCNetECodeMap(WSAGetLastError());
		}
	}
	else if (option == NET_NOPT_WRSOCKETONLY)
	{
		int ret = select(maxSock + 1, NULL, &rdSet, NULL, timeoutPtr);
		if (ret == SOCKET_ERROR)
		{
			return BDCNetECodeMap(WSAGetLastError());
		}
	}
	else
	{
		return NET_EBADPARAM;
	}
	
	activeCount = 0;
	for (size_t actIndex = 0; actIndex < sockCount; ++actIndex)
	{
		if (NET_FD_ISSET(sockList[actIndex], &rdSet))
		{
			activeSockList[activeCount] = sockList[actIndex];
			++activeCount;
		}
	}
	return NET_SUCCEED;
}

BDCNetECode BDCNetSend(NET_SOCKET sock, NetBuffer* buffers, NetAddressIpv4* addr, NetProtocol proto, unsigned int option)
{
	if (buffers == NULL)
	{
		return NET_EBADPARAM;
	}
	switch (proto)
	{
	case NET_PROTO_TCP:
	{
		if (sock == INVALID_SOCKET)
		{
			return NET_ESOCKINIT;
		}
		WSABUF sysBufs[MAXBUFSIZE + 1];
		DWORD totalLength = 0;
		for (size_t index = 0; index < buffers->totalNum; ++index)
		{
			sysBufs[index].buf = (CHAR*)buffers->data[index];
			sysBufs[index].len = buffers->len[index];
			totalLength += buffers->len[index];
		}
		unsigned int nTotalLength = totalLength;
		sysBufs[0].buf = (CHAR FAR*) & nTotalLength;
		sysBufs[0].len = 4;
		if (WSASend(sock, sysBufs, buffers->totalNum + 1, &totalLength, 0, NULL, NULL) == SOCKET_ERROR)
		{
			return BDCNetECodeMap(WSAGetLastError());
		}
		return NET_SUCCEED;
	}
	case NET_PROTO_UDP:
	{
		if (sock == INVALID_SOCKET)
		{
			return NET_ESOCKINIT;
		}
		WSABUF sysBufs[MAXBUFSIZE];
		DWORD totalLength = 0;
		for (size_t index = 0; index < buffers->totalNum; ++index)
		{
			sysBufs[index].buf = (CHAR*)buffers->data[index];
			sysBufs[index].len = buffers->len[index];
			totalLength += buffers->len[index];
		}
		if (WSASendTo(sock, sysBufs, buffers->totalNum, &totalLength, 0, (struct sockaddr*) & (addr->addrV4), sizeof(addr->addrV4), NULL, NULL) == SOCKET_ERROR)
		{
			return BDCNetECodeMap(WSAGetLastError());
		}
		return NET_SUCCEED;
	}
	default:
	{
		return NET_EPROTOTYPE;
	}
	}
}

BDCNetECode BDCNetRecv(NET_SOCKET sock, NetBuffer* buffers, size_t* recvSize, NetAddressIpv4* srcAddr, NetProtocol proto, unsigned int option)
{
	if (buffers == NULL)
	{
		return NET_EBADPARAM;
	}
	switch (proto)
	{
	case NET_PROTO_TCP:
	{
		WSABUF sysBufs[MAXBUFSIZE];
		DWORD totallength = 0;
		for (size_t index = 0; index < buffers->totalNum; ++index)
		{
			sysBufs[index].buf = (char*)buffers->data[index];
			sysBufs[index].len = buffers->len[index];
			totallength += buffers->len[index];
		}
		DWORD flags = 0;
		if (WSARecv(sock, sysBufs, buffers->totalNum, &totallength, &flags, NULL, NULL) == SOCKET_ERROR)
		{
			return BDCNetECodeMap(WSAGetLastError());
		}
		if (recvSize != NULL)
		{
			*recvSize = totallength;
		}
		return NET_SUCCEED;
	}
	case NET_PROTO_UDP:
	{
		WSABUF sysBufs[MAXBUFSIZE];
		DWORD totalLength = 0, flags = 0;
		for (size_t index = 0; index < buffers->totalNum; ++index)
		{
			sysBufs[index].buf = (char*)buffers->data[index];
			sysBufs[index].len = buffers->len[index];
			totalLength += buffers->len[index];
		}
		sockaddr* fromAddr = NULL;
		int addrSize = 0;
		if (srcAddr != NULL)
		{
			BDCZeroMemory(srcAddr);
			fromAddr = (struct sockaddr*) & (srcAddr->addrV4);
			addrSize = sizeof(srcAddr->addrV4);
		}
		flags = 0;
		if (WSARecvFrom(
			sock, sysBufs, buffers->totalNum,
			&totalLength, &flags, fromAddr,
			fromAddr == NULL ? NULL : &addrSize,
			NULL, NULL) == SOCKET_ERROR)
		{
			return BDCNetECodeMap(WSAGetLastError());
		}
		if (recvSize != NULL)
		{
			*recvSize = totalLength;
		}
		return NET_SUCCEED;
	}
	default:
	{
		return NET_EUNDEFINE;
	}
	}
}

BDCNetECode BDCNetECodeMap(int errCode)
{
	switch (errCode)
	{
	case 0: return NET_SUCCEED;
	case WSA_INVALID_HANDLE: return NET_INVALID_HANDLE;
	case WSA_NOT_ENOUGH_MEMORY: return NET_NOT_ENOUGH_MEMORY;
	case WSA_INVALID_PARAMETER: return NET_INVALID_PARAMETER;
	case WSA_OPERATION_ABORTED: return NET_OPERATION_ABORTED;
	case WSA_IO_INCOMPLETE: return NET_IO_INCOMPLETE;
	case WSA_IO_PENDING: return NET_IO_PENDING;
	case WSAEINTR: return NET_EINTR;
	case WSAEBADF: return NET_EBADF;
	case WSAEACCES: return NET_EACCES;
	case WSAEFAULT: return NET_EFAULT;
	case WSAEINVAL: return NET_EINVAL;
	case WSAEMFILE: return NET_EMFILE;
	case WSAEWOULDBLOCK: return NET_EWOULDBLOCK;
	case WSAEINPROGRESS: return NET_EINPROGRESS;
	case WSAEALREADY: return NET_EALREADY;
	case WSAENOTSOCK: return NET_ENOTSOCK;
	case WSAEDESTADDRREQ: return NET_EDESTADDRREQ;
	case WSAEMSGSIZE: return NET_EMSGSIZE;
	case WSAEPROTOTYPE: return NET_EPROTOTYPE;
	case WSAENOPROTOOPT: return NET_ENOPROTOOPT;
	case WSAEPROTONOSUPPORT: return NET_EPROTONOSUPPORT;
	case WSAESOCKTNOSUPPORT: return NET_ESOCKTNOSUPPORT;
	case WSAEOPNOTSUPP: return NET_EOPNOTSUPP;
	case WSAEPFNOSUPPORT: return NET_EPFNOSUPPORT;
	case WSAEAFNOSUPPORT: return NET_EAFNOSUPPORT;
	case WSAEADDRINUSE: return NET_EADDRINUSE;
	case WSAEADDRNOTAVAIL: return NET_EADDRNOTAVAIL;
	case WSAENETDOWN: return NET_ENETDOWN;
	case WSAENETUNREACH: return NET_ENETUNREACH;
	case WSAENETRESET: return NET_ENETRESET;
	case WSAECONNABORTED: return NET_ECONNABORTED;
	case WSAECONNRESET: return NET_ECONNRESET;
	case WSAENOBUFS: return NET_ENOBUFS;
	case WSAEISCONN: return NET_EISCONN;
	case WSAENOTCONN: return NET_ENOTCONN;
	case WSAESHUTDOWN: return NET_ESHUTDOWN;
	case WSAETOOMANYREFS: return NET_ETOOMANYREFS;
	case WSAETIMEDOUT: return NET_ETIMEDOUT;
	case WSAECONNREFUSED: return NET_ECONNREFUSED;
	case WSAELOOP: return NET_ELOOP;
	case WSAENAMETOOLONG: return NET_ENAMETOOLONG;
	case WSAEHOSTDOWN: return NET_EHOSTDOWN;
	case WSAEHOSTUNREACH: return NET_EHOSTUNREACH;
	case WSAENOTEMPTY: return NET_ENOTEMPTY;
	case WSAEPROCLIM: return NET_EPROCLIM;
	case WSAEUSERS: return NET_EUSERS;
	case WSAEDQUOT: return NET_EDQUOT;
	case WSAESTALE: return NET_ESTALE;
	case WSAEREMOTE: return NET_EREMOTE;
	case WSASYSNOTREADY: return NET_SYSNOTREADY;
	case WSAVERNOTSUPPORTED: return NET_VERNOTSUPPORTED;
	case WSANOTINITIALISED: return NET_NOTINITIALISED;
	case WSAEDISCON: return NET_EDISCON;
	case WSAENOMORE: return NET_ENOMORE;
	case WSAECANCELLED: return NET_ECANCELLED;
	case WSAEINVALIDPROCTABLE: return NET_EINVALIDPROCTABLE;
	case WSAEINVALIDPROVIDER: return NET_EINVALIDPROVIDER;
	case WSAEPROVIDERFAILEDINIT: return NET_EPROVIDERFAILEDINIT;
	case WSASYSCALLFAILURE: return NET_SYSCALLFAILURE;
	case WSASERVICE_NOT_FOUND: return NET_SERVICE_NOT_FOUND;
	case WSATYPE_NOT_FOUND: return NET_TYPE_NOT_FOUND;
	case WSA_E_NO_MORE: return NET_E_NO_MORE;
	case WSA_E_CANCELLED: return NET_E_CANCELLED;
	case WSAEREFUSED: return NET_EREFUSED;
	case WSAHOST_NOT_FOUND: return NET_HOST_NOT_FOUND;
	case WSATRY_AGAIN: return NET_TRY_AGAIN;
	case WSANO_RECOVERY: return NET_NO_RECOVERY;
	case WSANO_DATA: return NET_NO_DATA;
	case WSA_QOS_RECEIVERS: return NET_QOS_RECEIVERS;
	case WSA_QOS_SENDERS: return NET_QOS_SENDERS;
	case WSA_QOS_NO_SENDERS: return NET_QOS_NO_SENDERS;
	case WSA_QOS_NO_RECEIVERS: return NET_QOS_NO_RECEIVERS;
	case WSA_QOS_REQUEST_CONFIRMED: return NET_QOS_REQUEST_CONFIRMED;
	case WSA_QOS_ADMISSION_FAILURE: return NET_QOS_ADMISSION_FAILURE;
	case WSA_QOS_POLICY_FAILURE: return NET_QOS_POLICY_FAILURE;
	case WSA_QOS_BAD_STYLE: return NET_QOS_BAD_STYLE;
	case WSA_QOS_BAD_OBJECT: return NET_QOS_BAD_OBJECT;
	case WSA_QOS_TRAFFIC_CTRL_ERROR: return NET_QOS_TRAFFIC_CTRL_ERROR;
	case WSA_QOS_GENERIC_ERROR: return NET_QOS_GENERIC_ERROR;
	case WSA_QOS_ESERVICETYPE: return NET_QOS_ESERVICETYPE;
	case WSA_QOS_EFLOWSPEC: return NET_QOS_EFLOWSPEC;
	case WSA_QOS_EPROVSPECBUF: return NET_QOS_EPROVSPECBUF;
	case WSA_QOS_EFILTERSTYLE: return NET_QOS_EFILTERSTYLE;
	case WSA_QOS_EFILTERTYPE: return NET_QOS_EFILTERTYPE;
	case WSA_QOS_EFILTERCOUNT: return NET_QOS_EFILTERCOUNT;
	case WSA_QOS_EOBJLENGTH: return NET_QOS_EOBJLENGTH;
	case WSA_QOS_EFLOWCOUNT: return NET_QOS_EFLOWCOUNT;
	case WSA_QOS_EUNKOWNPSOBJ: return NET_QOS_EUNKOWNPSOBJ;
	case WSA_QOS_EPOLICYOBJ: return NET_QOS_EPOLICYOBJ;
	case WSA_QOS_EFLOWDESC: return NET_QOS_EFLOWDESC;
	case WSA_QOS_EPSFLOWSPEC: return NET_QOS_EPSFLOWSPEC;
	case WSA_QOS_EPSFILTERSPEC: return NET_QOS_EPSFILTERSPEC;
	case WSA_QOS_ESDMODEOBJ: return NET_QOS_ESDMODEOBJ;
	case WSA_QOS_ESHAPERATEOBJ: return NET_QOS_ESHAPERATEOBJ;
	case WSA_QOS_RESERVED_PETYPE: return NET_QOS_RESERVED_PETYPE;
	default: return NET_EUNKNOW;
	}
}

BDCNetECode BDCTCPNetSetKeepAlive(NET_SOCKET sock, TCPKeepalive* optVal)
{
	bool bKeepAlive = true;
	int ret = setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (const char*)&bKeepAlive, sizeof(bool));
	if (ret != 0)
	{
		return BDCNetECodeMap(ret);
	}
	if (optVal != NULL)
	{
		if (optVal->tcpMaxrt < 0)
		{
			DWORD maxrt = -1;
			ret = setsockopt(sock, IPPROTO_TCP, TCP_MAXRT, (const char*)&maxrt, sizeof(DWORD));
		}
		else
		{
			ret = setsockopt(sock, IPPROTO_TCP, TCP_MAXRT, (const char*)&optVal->tcpMaxrt, sizeof(DWORD));
		}
		if (ret != 0)
		{
			return BDCNetECodeMap(ret);
		}
		ret = setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, (const char*)&optVal->keepIdle, sizeof(DWORD));
		if (ret != 0)
		{
			return BDCNetECodeMap(ret);
		}
		ret = setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, (const char*)&optVal->keepIntvl, sizeof(DWORD));
		if (ret != 0)
		{
			return BDCNetECodeMap(ret);
		}
	}
	return NET_SUCCEED;
}

BDCNetECode BDCUDPNetSetBroadCast(NET_SOCKET sock)
{
	bool bBroadCast = true;
	int ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char*)&bBroadCast, sizeof(bool));
	if (ret != 0)
	{
		return BDCNetECodeMap(ret);
	}
	return NET_SUCCEED;
}

BDCNetECode BDCNetSetSendBufSize(NET_SOCKET sock, int sendBufSize)
{
	int ret = setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (const char*)&sendBufSize, sizeof(int));
	if (ret != 0)
	{
		return BDCNetECodeMap(ret);
	}
	return NET_SUCCEED;
}

BDCNetECode BDCNetSetRecvBufSize(NET_SOCKET sock, int recvBufSize)
{
	int ret = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (const char*)&recvBufSize, sizeof(int));
	if (ret != 0)
	{
		return BDCNetECodeMap(ret);
	}
	return NET_SUCCEED;
}

bool NetAddressIpv4::operator==(const NetAddressIpv4& addr)
{
	if (addrV4.sin_family != addr.addrV4.sin_family)
	{
		return false;
	}
	if (addrV4.sin_addr.s_addr != addr.addrV4.sin_addr.s_addr)
	{
		return false;
	}
	if (addrV4.sin_port != addr.addrV4.sin_port)
	{
		return false;
	}
	return true;
}
