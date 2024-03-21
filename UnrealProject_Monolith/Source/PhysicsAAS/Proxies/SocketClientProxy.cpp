// Fill out your copyright notice in the Description page of Project Settings.


#include "SocketClientProxy.h"

DEFINE_LOG_CATEGORY(LogSocketClientProxy);

SOCKET FSocketClientProxy::ConnectionSocket = INVALID_SOCKET;

bool FSocketClientProxy::OpenSocketConnection
    (const FString& SocketServerIpAddr)
{
    UE_LOG(LogSocketClientProxy, Warning, TEXT("Connecting on:%s"), 
        *SocketServerIpAddr);

    // Initialize Winsock. This is needed to ensure the startup of the
    // environment to be able to use windows sockets
    if (!StartupWinsock())
    {
        // If startup fails, return false to avoid further processing
        return false;
    }

    // Get this client (local) addrinfo
    // This will get the client addr as localhost
    addrinfo* addrInfoResult = GetClientAddrInfo(SocketServerIpAddr);
    if (!addrInfoResult)
    {
        return false;
    }

    // Attempt to connect to the socket server (that is localhost)
    ConnectSocketToLocalSocketServer(addrInfoResult);

    // Free memory as we don't need it anymore
    freeaddrinfo(addrInfoResult);

    // Check if we have found a valid connection with server
    if (ConnectionSocket == INVALID_SOCKET)
    {
        // If not, clear winsock and return false to avoid further processing
        UE_LOG(LogSocketClientProxy, Error, 
            TEXT("Unable to connect to server! Must likely no server was found to connect to."));

        WSACleanup();
        return false;
    }

    // All good, ConnectionSocket is up and valid 
    // (connected to localhost server)
    return true;
}

bool FSocketClientProxy::StartupWinsock()
{
    // Initialize Winsock
    WSADATA wsaData;
    const int StartupResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (StartupResult != 0)
    {
        UE_LOG(LogSocketClientProxy, Error, 
            TEXT("WSAStartup failed with error: %d"), StartupResult);
        return false;
    }

    return true;
}

addrinfo* FSocketClientProxy::GetClientAddrInfo
    (const FString& SocketServerIpAddr)
{
    // Create hints to pass as the addr information
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));

    // Set hits to get a socket of TCP protocol type
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the client address and port
    addrinfo* addrResultInfo = NULL;
    const char* serverIpAddr = TCHAR_TO_ANSI(*SocketServerIpAddr);
    const int addrinfoReturnValue = getaddrinfo(serverIpAddr, DEFAULT_PORT, 
        &hints, &addrResultInfo);

    // Check for errors
    if (addrinfoReturnValue != 0)
    {
        UE_LOG(LogSocketClientProxy, Error, 
            TEXT("GetClientAddrInfo() failed with error: %d"), 
            addrinfoReturnValue);

        // Clean up winsock
        WSACleanup();
        return NULL;
    }

    return addrResultInfo;
}

void FSocketClientProxy::ConnectSocketToLocalSocketServer
    (addrinfo* addrinfoToConnect)
{
    // Attempt to connect to an address until one succeeds
    struct addrinfo* ptr = NULL;
    for (ptr = addrinfoToConnect; ptr != NULL; ptr = ptr->ai_next)
    {
        // Create a SOCKET for connecting to server
        ConnectionSocket = socket(ptr->ai_family, ptr->ai_socktype, 
            ptr->ai_protocol);
        if (ConnectionSocket == INVALID_SOCKET)
        {
            UE_LOG(LogSocketClientProxy, Error, 
                TEXT("socket failed with error: %d\n"), WSAGetLastError());
            WSACleanup();
            return;
        }

        // Connect to server.
        const int ConnectResultValue = connect(ConnectionSocket, ptr->ai_addr, 
            (int)ptr->ai_addrlen);

        // Check for errors. If error, just continue to try and get the
        // next connection
        if (ConnectResultValue == SOCKET_ERROR)
        {
            closesocket(ConnectionSocket);
            ConnectionSocket = INVALID_SOCKET;
            continue;
        }
        
        // If have found a valid connection, break and terminate 
        // processing as we've found a valid connection
        break;
    }
}

bool FSocketClientProxy::CloseSocketConnection()
{
    // Check if connection socket is not already invalid
    if (ConnectionSocket == INVALID_SOCKET)
    {
        return true;
    }

    // Shutdown the connection since no more data will be sent
    const int ShutdownResult = shutdown(ConnectionSocket, SD_SEND);

    // Check for shutdown errors
    if (ShutdownResult == SOCKET_ERROR)
    {
        UE_LOG(LogSocketClientProxy, Error, 
            TEXT("Shutdown failed with error: %d"), WSAGetLastError());

        // Close socket and cleanup winsock
        closesocket(ConnectionSocket);
        WSACleanup();

        return false;
    }

    // Close socket and Cleanup winsock
    closesocket(ConnectionSocket);
    ConnectionSocket = INVALID_SOCKET;
    WSACleanup();

    return true;
}

FString FSocketClientProxy::SendMessageAndGetResponse(const char* Message)
{
    if (ConnectionSocket == INVALID_SOCKET)
    {
        UE_LOG(LogSocketClientProxy, Error, 
            TEXT("Could not send message as socket connection is not valid."));
        return FString();
    }

    // Sending message
    const int SendReturn = send(ConnectionSocket, Message,
        (int)strlen(Message), 0);

    // Check for error
    if (SendReturn == SOCKET_ERROR)
    {
        UE_LOG(LogSocketClientProxy, Error, TEXT("Send failed with error: %d"), 
            WSAGetLastError());

        // Cleanup winsock
        closesocket(ConnectionSocket);
        WSACleanup();

        return FString();
    }

    // Setup buffer length to receive message. 
    // Any more characters will be discarded
    // TODO do a better solution for this
    char recvbuf[DEFAULT_BUFLEN];
    const int recvbuflen = DEFAULT_BUFLEN;

    // Await response from socket 
    // (this will stall the game thread until we receive a response)
    const int ReceiveReturn = recv(ConnectionSocket, recvbuf, recvbuflen, 0);

    // Check for errors
    if (ReceiveReturn < 0)
    {
        UE_LOG(LogSocketClientProxy, Error, 
            TEXT("Recv failed with error: %d"), WSAGetLastError());
        return FString();
    }

    // Debug amount of bytes received
    //UE_LOG(LogSocketClientProxy, Log, TEXT("Bytes received: %d"), ReceiveReturn);

    // Decode message received from socket as FString
    // The buffer will contain the message returned as char
    // The return value is the amount of bytes, and thus, the message length
    FString ReceivedMsgAsString(ReceiveReturn, recvbuf);

    return ReceivedMsgAsString;
}
