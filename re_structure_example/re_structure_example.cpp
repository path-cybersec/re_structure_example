// re_structure_example.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 0x1000

struct packet_t
{
    UINT64 len;
    UINT64 type;
    char* data;
};

struct my_struct_t
{
    int socket;
    DWORD TcpClientNum;
    UINT32 total_recvd;
    struct packet_t packet;
};



int setup_socket(SOCKET* res_sock) {
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, "1337", &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // No longer need server socket
    closesocket(ListenSocket);

    // Receive until the peer shuts down the connection
    do {

        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            printf("Bytes received: %d\n", iResult);

            // Echo the buffer back to the sender
            iSendResult = send(ClientSocket, recvbuf, iResult, 0);
            if (iSendResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(ClientSocket);
                WSACleanup();
                return 1;
            }
            printf("Bytes sent: %d\n", iSendResult);
        }
        else if (iResult == 0)
            printf("Connection closing...\n");
        else {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }

    } while (iResult > 0);

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}

int recv_loop(struct my_struct_t* ms)
{
    int len_recvd; // eax
    int Error; // eax
    int next_data_len; // [esp+Ch] [ebp-10h]
    int b_header_recvd; // [esp+10h] [ebp-Ch]
    int len_recvd_2; // [esp+14h] [ebp-8h]
    DWORD* packet; // [esp+18h] [ebp-4h]

    //packet = (DWORD*)(a1->buf_E008_recv_buf);
    char* header = (char*)malloc(sizeof(UINT64) * 2);
    next_data_len = 0;
    b_header_recvd = 0;
    ms->total_recvd = 0;
    *packet = 0;
    while (1)
    {
        if (ms->total_recvd == 16 )
        {
            ms->packet.data = (char*)malloc(ms->packet.len);
            len_recvd = recv(
                ms->socket,
                (char*)(ms->packet.data + ms->total_recvd),
                ms->packet.len - ms->total_recvd,
                0);
        }
        else
        {
            len_recvd = recv(ms->socket, (char*)(header + ms->total_recvd), 16 - ms->total_recvd, 0);
        }
        if (len_recvd == -1)
        {
            Error = WSAGetLastError();
            printf("Recv failed: %d for socket %d\n", Error, ms->socket);
            return -1;
        }
        if (!len_recvd)
        {
            printf("Connection closed.\n");
            return -1;
        }
        ms->total_recvd += len_recvd;
        
        printf("Data length:%d\n", ((struct packet_t*)header)->len);
        if (ms->total_recvd == (ms->packet.len + 16))
        {
            printf("Packet recieved\n");
            switch (ms->packet.type)
            {
            case 1:
                printf("Data: %s\n", ms->packet.data);
                break;
            case 2:
                printf("Test packet recieved\n");
                break;
            default:
                printf("Unknown packet type\n");

            }
            break;

        }
        if (ms->total_recvd > 16)
        {
            printf("Fatal error\n");
            return -1;
        }
    }
    printf("Recv loop end\n");
    return 0;
    
}

int main()
{
    SOCKET sock;
    int err;
    err = setup_socket(&sock);
    struct my_struct_t* ms;
    ms = (struct my_struct_t* )malloc(sizeof(struct my_struct_t));
    recv_loop(ms);

}
