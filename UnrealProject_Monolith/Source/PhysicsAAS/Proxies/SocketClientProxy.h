// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 524288
#define DEFAULT_PORT "27015"

DECLARE_LOG_CATEGORY_EXTERN(LogSocketClientProxy, Log, All);

/**
 * 
 */
class PHYSICSAAS_API FSocketClientProxy
{
public:
	/** */
	static bool OpenSocketConnection(const FString& SocketServerIpAddr);
	/** */
	static bool CloseSocketConnection();

	/** */
	static FString SendMessageAndGetResponse(const char* Message);

	/** */
	inline static bool HasValidConnection() 
		{ return ConnectionSocket != INVALID_SOCKET; }

private:
	/** */
	static bool StartupWinsock();

	/** */
	static addrinfo* GetClientAddrInfo(const FString& SocketServerIpAddr);

	/** */
	static void ConnectSocketToLocalSocketServer(addrinfo* addrinfoToConnect);

private:
	/** */
	static SOCKET ConnectionSocket;
};
