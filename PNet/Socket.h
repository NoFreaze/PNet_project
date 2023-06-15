#pragma once
#include "SocketHandle.h"
#include "PResult.h"
#include "IPVersion.h"
#include "SocketOptions.h"
#include "IPEndpoint.h"
#include "Constants.h"
#include "Packet.h"

namespace PNet
{
	class Socket
	{
		public:
			Socket(IPVersion ipversion = IPVersion::IPv4, SocketHandle handle = INVALID_SOCKET);
			PResult Create();
			PResult Close();
			PResult Bind(IPEndpoint endpoint);
			SocketHandle GetHandle();
			IPVersion GetIPVersion();
			PResult Send(const void* data, int numberOfBytes, int& bytesSent);
			PResult Recv(void* destination, int numberOfBytes, int& bytesReceived);
			PResult SendAll(const void* data, int numberOfBytes);
			PResult RecvAll(void* destination, int numberOfBytes);
			PResult Send(Packet& packet);
			PResult Recv(Packet& packet);

			PResult Listen(IPEndpoint endpoint, int backlog = 5);
			PResult Accept(Socket & outSocket);
			PResult Connect(IPEndpoint endpoint);
		private:
			PResult SetSocketOption(SocketOption option, BOOL value);
			IPVersion ipversion = IPVersion::IPv4;
			SocketHandle handle = INVALID_SOCKET;
	};

}