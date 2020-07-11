#ifndef EbBDCNetwork_h
#define EbBDCNetwork_h

#include "EbBDCNetCommons.h"
#define MAXBUFSIZE 8
#define PROTOCOL_ADDR_LENGTH 16
#define NET_TCP_NODELAY 8
#include <vector>
#if defined(_WIN32)
#include <winsock2.h>
#endif
#define NET_SOCKET SOCKET
#define NET_fd_set fd_set
#define NET_FD_ZERO FD_ZERO
#define NET_FD_SET FD_SET
#define NET_FD_ISSET FD_ISSET

typedef std::vector<NET_SOCKET> SocketList;
struct NetBuffer
{
	void* data[MAXBUFSIZE];
	int len[MAXBUFSIZE];
	int totalNum;
};

enum NetSocketType
{
	NET_ADDR_IPV4,
	NET_ADDR_IPV6,
};

struct NetAddressIpv4
{
	sockaddr_in addrV4;
	bool operator==(const NetAddressIpv4& addr);
};

struct NetAddr
{
	NetSocketType type;
	unsigned char protocalAddr[PROTOCOL_ADDR_LENGTH];
	int port;
};

typedef struct NetTimeVal
{
	int sec;
	int nanosec;
}NetTimeVal;

enum NetOption
{
	NET_NOPT_DEFAULT,
	NET_NOPT_WRSOCKET, //Ð´
	NET_NOPT_RDSOCKET, //¶Á
	NET_NOPT_WRSOCKETONLY, // Ö»Ð´
};

enum NetProtocol
{
	NET_PROTO_UDP,
	NET_PROTO_TCP,
};

struct TCPKeepalive
{
	DWORD tcpMaxrt;
	DWORD keepIdle;
	DWORD keepIntvl;
};

BDCNetECode BDCNetStartup();

BDCNetECode BDCNetClose();

BDCNetECode BDCCreateSocket(NET_SOCKET& sock, NetProtocol proto);

int BDCCloseSocket(NET_SOCKET sock);

BDCNetECode BDCNetBind(NET_SOCKET sock, NetAddressIpv4& addr, unsigned int option);

BDCNetECode BDCNetListen(NET_SOCKET sock, int maxQueueLen, unsigned int option);

BDCNetECode BDCNetConnect(NET_SOCKET sock, NetAddressIpv4& addr, unsigned int option);

BDCNetECode BDCNetAccept(NET_SOCKET sock, NetAddressIpv4& newAddr, unsigned int option);

BDCNetECode BDCNetSelect(SocketList sockList, size_t sockCount, NetTimeVal* timeout, SocketList& activeSockList, size_t& activeCount, NetOption option);

BDCNetECode BDCNetSend(NET_SOCKET sock, NetBuffer *buffers, NetAddressIpv4* addr, NetProtocol proto, unsigned int option);

BDCNetECode BDCNetRecv(NET_SOCKET sock, NetBuffer* buffers, size_t* recvSize, NetAddressIpv4* srcAddr, NetProtocol proto, unsigned int option);

BDCNetECode BDCNetECodeMap(int errCode);

BDCNetECode BDCTCPNetSetKeepAlive(NET_SOCKET sock, TCPKeepalive* optVal);

BDCNetECode BDCUDPNetSetBroadCast(NET_SOCKET sock);

BDCNetECode BDCNetSetSendBufSize(NET_SOCKET sock, int sendBufSize);

BDCNetECode BDCNetSetRecvBufSize(NET_SOCKET sock, int recvBufSize);
#endif