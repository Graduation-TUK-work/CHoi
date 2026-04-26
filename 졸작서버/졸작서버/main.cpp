#include <winsock2.h>
#include <iostream>
#include <vector>
#include <list>
#include <mutex>
#include "Shared.h"

#pragma comment(lib, "ws2_32.lib")
using namespace std;

// IOCP에서 사용할 컨텍스트
enum class IO_TYPE { READ, WRITE };

struct Session {
    SOCKET socket;
};

struct OverlappedContext {
    WSAOVERLAPPED overlapped;
    WSABUF wsaBuf;
    char buffer[1024];
    IO_TYPE ioType;
};

// 전역 변수: 세션 관리 및 동기화
list<Session*> g_sessions;
mutex g_sessionLock;

// 다른 모든 클라이언트에게 데이터 전송 (Broadcast)
void Broadcast(char* buf, int len, Session* exceptSession = nullptr) {
    lock_guard<mutex> lock(g_sessionLock);
    for (Session* s : g_sessions) {
        if (s == exceptSession) continue; // 보낸 사람 본인은 제외
        send(s->socket, buf, len, 0);
    }
}

// 워커 스레드: 패킷 처리의 핵심
DWORD WINAPI WorkerThread(LPVOID lpParam) {
    HANDLE hIOCP = (HANDLE)lpParam;
    DWORD bytesTransferred = 0;
    Session* pSession = nullptr;
    OverlappedContext* pContext = nullptr;

    while (true) {
        BOOL result = GetQueuedCompletionStatus(
            hIOCP, &bytesTransferred, (PULONG_PTR)&pSession, (LPOVERLAPPED*)&pContext, INFINITE
        );

        // 접속 종료 시
        if (result == FALSE || bytesTransferred == 0) {
            cout << "[알림] 클라이언트 접속 종료" << endl;
            lock_guard<mutex> lock(g_sessionLock);
            g_sessions.remove(pSession);
            closesocket(pSession->socket);
            delete pSession;
            delete pContext;
            continue;
        }

        if (pContext->ioType == IO_TYPE::READ) {
            unsigned char packetType = (unsigned char)pContext->buffer[0];

            switch (packetType) {
            case PKT_MOVE: {
                FPacketMove* movePkt = (FPacketMove*)pContext->buffer;
                printf("ID[%d] Type[%d] Move: X=%.1f, Y=%.1f, Z=%.1f, Yaw=%.1f\n",
                    movePkt->Data.PlayerId, movePkt->Data.CharacterType, movePkt->Data.X, movePkt->Data.Y, movePkt->Data.Z, movePkt->Data.RotationYaw);
                Broadcast(pContext->buffer, sizeof(FPacketMove), pSession);
                break;
            }
            case PKT_ACTION: {
                FPacketAction* actionPkt = (FPacketAction*)pContext->buffer;
                printf("ID[%d] Action[%d] Target[%d]\n",
                    actionPkt->InstigatorId, actionPkt->ActionType, actionPkt->TargetId);
                Broadcast(pContext->buffer, sizeof(FPacketAction), pSession);
                break;
            }
            }

            // 다음 패킷 수신 예약
            DWORD flags = 0;
            DWORD recvBytes = 0;
            memset(&pContext->overlapped, 0, sizeof(OVERLAPPED));
            WSARecv(pSession->socket, &pContext->wsaBuf, 1, &recvBytes, &flags, &pContext->overlapped, NULL);
        }
    }
    return 0;
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    HANDLE hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    int threadCount = sysInfo.dwNumberOfProcessors * 2;
    for (int i = 0; i < threadCount; ++i) {
        CreateThread(NULL, 0, WorkerThread, hIOCP, 0, NULL);
    }

    SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    SOCKADDR_IN serverAddr = { 0 };
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(7777);

    bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    listen(listenSocket, SOMAXCONN);

    cout << "======= Unreal IOCP Server Running (Port 7777) =======" << endl;

    while (true) {
        SOCKADDR_IN clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);

        if (clientSocket != INVALID_SOCKET) {
            cout << "[접속] 새로운 언리얼 클라이언트 연결!" << endl;

            Session* newSession = new Session{ clientSocket };
            {
                lock_guard<mutex> lock(g_sessionLock);
                g_sessions.push_back(newSession);
            }

            OverlappedContext* recvContext = new OverlappedContext();
            memset(recvContext, 0, sizeof(OverlappedContext));
            recvContext->ioType = IO_TYPE::READ;
            recvContext->wsaBuf.buf = recvContext->buffer;
            recvContext->wsaBuf.len = sizeof(recvContext->buffer);

            CreateIoCompletionPort((HANDLE)clientSocket, hIOCP, (ULONG_PTR)newSession, 0);

            // 1. 클라이언트에게 접속 ID 부여 (PKT_JOIN)
            FPacketJoin joinPkt;
            joinPkt.Type = PKT_JOIN;
            joinPkt.MyId = (int)clientSocket; // 소켓 핸들을 임시 ID로 사용
            send(clientSocket, (char*)&joinPkt, sizeof(joinPkt), 0);

            // 2. 최초 수신 예약
            DWORD flags = 0;
            DWORD recvBytes = 0;
            WSARecv(clientSocket, &recvContext->wsaBuf, 1, &recvBytes, &flags, &recvContext->overlapped, NULL);
        }
    }

    WSACleanup();
    return 0;
}