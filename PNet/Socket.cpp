#include "Socket.h"
#include <assert.h>
#include <iostream>

namespace PNet
{
    Socket::Socket(IPVersion ipversion, SocketHandle handle):ipversion(ipversion), handle(handle)
    {
        assert(ipversion == IPVersion::IPv4);
    }

    PResult Socket::Create()
    {
        assert(ipversion == IPVersion::IPv4);
        if (handle != INVALID_SOCKET)
        {
            return PResult::P_GenericError;
        }

        handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //attenpt to create socket
        if (handle == INVALID_SOCKET)
        {
            int error = WSAGetLastError();
            return PResult::P_GenericError;

        }

        if (SetSocketOption(SocketOption::TCP_NoDelay, TRUE) != PResult::P_Success)
        {
            return PResult::P_GenericError;
        }


        return PResult::P_Success;
    }

    PResult Socket::Close()
    {
        if (handle == INVALID_SOCKET)
        {
            return PResult::P_GenericError;
        }
        
        int result = closesocket(handle);
        if (result != 0)
        {
            int error = WSAGetLastError();
            return PResult::P_GenericError;
        }
        handle = INVALID_SOCKET;
        return PResult::P_Success;
    }

    PResult Socket::Bind(IPEndpoint endpoint)
    {
        sockaddr_in addr = endpoint.GetSocaddrIPv4();
        int result = bind(handle, (sockaddr*)(&addr), sizeof(sockaddr_in));
        if (result != 0)
        {
            int error = WSAGetLastError();
            return PResult::P_GenericError;
        }
        return PResult::P_Success;
    }

    SocketHandle Socket::GetHandle()
    {
        return handle;
    }

    IPVersion Socket::GetIPVersion()
    {
        return ipversion;
    }

    PResult Socket::Send(const void* data, int numberOfBytes, int& bytesSent)
    {
        bytesSent = send(handle, (const char*)data, numberOfBytes, NULL);
        if (bytesSent == SOCKET_ERROR)
        {
            int error = WSAGetLastError();
            return PResult::P_GenericError;
        }

        return PResult::P_Success;
    }

    PResult Socket::Recv(void* destination, int numberOfBytes, int& bytesReceived)
    {
        bytesReceived = recv(handle, (char*)destination, numberOfBytes, NULL);

        if (bytesReceived == 0)
        {
            return PResult::P_GenericError;
        }

        if (bytesReceived == SOCKET_ERROR)
        {
            int error = WSAGetLastError();
            return PResult::P_GenericError;
        }

        return PResult::P_Success;
    }

    PResult Socket::SendAll(const void* data, int numberOfBytes)
    {
        int totalBytesSent = 0;
        int bytesRemaining = 0;
        int bytesSent = 0;
        char* bufferOffset;
        while (totalBytesSent < numberOfBytes)
        {
            bytesRemaining = numberOfBytes - totalBytesSent;
            bufferOffset = (char*)data + totalBytesSent;
            PResult result = Send(bufferOffset, bytesRemaining, bytesSent);
            if (result != PResult::P_Success)
            {
                return PResult::P_GenericError;
            }
            totalBytesSent += bytesSent;
        }

        return PResult::P_Success;
    }

    PResult Socket::RecvAll(void* destination, int numberOfBytes)
    {

        int totalBytesReceived = 0;
        int bytesReceiving = 0;
        int bytesReceived = 0;
        char* bufferOffset;
        while (totalBytesReceived < numberOfBytes)
        {
            bytesReceiving = numberOfBytes - totalBytesReceived;
            bufferOffset = (char*)destination + totalBytesReceived;
            PResult result = Recv(bufferOffset, bytesReceiving, bytesReceived);
            if (result != PResult::P_Success)
            {
                return PResult::P_GenericError;
            }
            totalBytesReceived += bytesReceived;
        }

        return PResult::P_Success;
    }

    PResult Socket::Send(Packet& packet)
    {
        uint16_t encodedPacketSize = htons(packet.buffer.size());
        PResult result = SendAll(&encodedPacketSize, sizeof(uint16_t));
        if (result != PResult::P_Success)
            return PResult::P_GenericError;
        
        result = SendAll(packet.buffer.data(), packet.buffer.size());
        if (result != PResult::P_Success)
            return PResult::P_GenericError;

        return PResult::P_Success;
    }

    PResult Socket::Recv(Packet& packet)
    {
        packet.Clear();

        uint16_t encodedSize = 0;
        PResult result = RecvAll(&encodedSize, sizeof(uint16_t));
        if (result != PResult::P_Success)
            return PResult::P_GenericError;

        uint16_t bufferSize = ntohs(encodedSize);
        if (bufferSize > PNet::g_MaxPacketSize)
            return PResult::P_GenericError;

        packet.buffer.resize(bufferSize);

        result = RecvAll(&packet.buffer[0], bufferSize);
        if (result != PResult::P_Success)
            return PResult::P_GenericError;

        return PResult::P_Success;
    }

    PResult Socket::Listen(IPEndpoint endpoint, int backlog)
    {
        if (Bind(endpoint) != PResult::P_Success)
        {
            return PResult::P_GenericError;
        }
        int result = listen(handle, backlog);
        if (result != 0)
        {
            int error = WSAGetLastError();
            return PResult::P_GenericError;
        }
        return PResult::P_Success;
    }

    PResult Socket::Accept(Socket& outSocket)
    {
        sockaddr_in addr = {};
        int len = sizeof(sockaddr_in);
        SocketHandle acceptedConnectionHandle = accept(handle, (sockaddr*)(&addr), &len);
        if(acceptedConnectionHandle == INVALID_SOCKET)
        {
            int error = WSAGetLastError();
            return PResult::P_GenericError;
        }
        IPEndpoint newConnectionEndpoint((sockaddr*)&addr);
        std::cout << "New connection accepted!" << std::endl;
        newConnectionEndpoint.Print();
        outSocket = Socket(IPVersion::IPv4, acceptedConnectionHandle);
        return PResult::P_Success;
    }

    PResult Socket::Connect(IPEndpoint endpoint)
    {
        sockaddr_in addr = endpoint.GetSocaddrIPv4();
        int result = connect(handle, (sockaddr*)(&addr), sizeof(sockaddr_in));
        if (result != 0)
        {
            int error = WSAGetLastError();
            return PResult::P_GenericError;
        }
        return PResult::P_Success;
    }

    PResult Socket::SetSocketOption(SocketOption option, BOOL value)
    {
        int result = 0;
        switch (option)
        {
        case SocketOption::TCP_NoDelay:
            result = setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, (const char*)&value, sizeof(value));
            break;
        default:
            return PResult::P_GenericError;
        }

        if (result != 0)
        {
            int error = WSAGetLastError();
            return PResult::P_GenericError;
        }

        return PResult::P_Success;
    }
}
