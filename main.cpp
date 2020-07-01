#include <cstdio>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <pthread.h>

#include <errno.h>
#include <iostream>
#include <wiringPi.h>
#include <wiringSerial.h>

# define RX_BUFFER_SIZE 1000 // UDP 패킷 버퍼

int serialPort;
bool bUsingUart = false;

void serialPutString(unsigned char* ptmp, int len);
void *udpServer1(void* arg);
void *udpServer2(void* arg);

int main(int argc, char* argv[])
{
    if ((serialPort = serialOpen("/dev/ttyAMA0", 115200)) < 0)
    {
        fprintf(stderr, "Unable to open serial device: %s\n", strerror(errno));
        return 1;
    }
    if (wiringPiSetup() == -1)
    {
        fprintf(stdout, "Unable to start wiringPi: %s\n", strerror(errno));
        return 1;
    }

    pthread_t pth1, pth2;

    int thread1 = pthread_create(&pth1, NULL, udpServer1, (void*)&"10001");
    int thread2 = pthread_create(&pth2, NULL, udpServer2, (void*)&"10002");

    pthread_join(pth1, NULL);
    pthread_join(pth2, NULL);

    return 0;
}


int main2(int argc, char* argv[]) 
{
    int ser, x;
    char* pstr = "Hello World!";
    char* ptmp;
    if ((ser = serialOpen("/dev/ttyAMA0", 921600)) < 0)
    {
        fprintf(stderr, "Unable to open serial device: %s\n", strerror(errno));
        return 1;
    }

    if (wiringPiSetup() == -1)
    {
        fprintf(stdout, "Unable to start wiringPi: %s\n", strerror(errno));
        return 1;
    }
    for (x = 0; x < 10; x++) {
        ptmp = pstr;
        while (*ptmp) {
            serialPutchar(ser, *ptmp);
            ptmp++;
        }
        delay(50);

        printf("Receive:");
        while (serialDataAvail(ser))
        {
            printf("%c", (char)serialGetchar(ser));
            fflush(stdout);
        }
        printf("\n");
    }
    return 0;
}

void serialPutString(unsigned char* pString, int len)
{
    while (bUsingUart == true);
    bUsingUart = true;


    unsigned char* ptmp = pString;
    int lentemp = len;
    while (lentemp--) {
        serialPutchar(serialPort, *ptmp);
        ptmp++;
    }

    bUsingUart = false;
}

//void serialPutString(char* pString, int len)
//{
//    char* ptmp = pString;
//    serialPuts(serialPort, ptmp);
//}

//unsigned char 출력
void *udpServer1(void* arg)
{
    unsigned char readBuff[RX_BUFFER_SIZE];
    unsigned char sendBuff[RX_BUFFER_SIZE];
    struct sockaddr_in serverAddress, clientAddress;
    int server_fd, client_fd;
    socklen_t client_addr_size;
    ssize_t receivedBytes;
    ssize_t sentBytes;

    memset(&serverAddress, 0, sizeof(serverAddress));
    memset(&clientAddress, 0, sizeof(clientAddress));

    serverAddress.sin_family = AF_INET;
    //serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_addr.s_addr = inet_addr("192.168.10.111");
    serverAddress.sin_port = htons(atoi((char*)arg));


    // 서버 소켓 생성(UDP)
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) // SOCK_DGRAM : UDP
    {
        printf("Sever : can not Open Socket\n");
        exit(0);
    }

    // bind
    if (bind(server_fd, (struct sockaddr*) & serverAddress, sizeof(serverAddress)) < 0)
    {
        printf("Server : can not bind local address");
        exit(0);
    }

    printf("UDP Server Start. IP:%s Port:%d\n", inet_ntoa(serverAddress.sin_addr), ntohs(serverAddress.sin_port));

    while (1)
    {
        client_addr_size = sizeof(clientAddress);

        // 패킷 수신
        receivedBytes = recvfrom(server_fd, readBuff, RX_BUFFER_SIZE, 0, (struct sockaddr*) & clientAddress, &client_addr_size);
        printf("\n%lu bytes read\n", receivedBytes);
        readBuff[receivedBytes] = '\0';

        // 현재 스레드 서버 IP출력
        printf("Server :  IP:%s Port:%d\n", inet_ntoa(serverAddress.sin_addr), ntohs(serverAddress.sin_port));

        // 클라이언트 IP 확인
        struct sockaddr_in connectSocket;
        socklen_t connectSocketLength = sizeof(connectSocket);
        getpeername(client_fd, (struct sockaddr*) & clientAddress, &connectSocketLength);
        char clientIP[sizeof(clientAddress.sin_addr) + 20] = { 0 };
        sprintf(clientIP, "IP:%s port:%d ", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
        printf("Client : %s\n", clientIP);
        
        // UART 출력
        serialPutString(readBuff, receivedBytes);
        //serialPuts(serialPort, readBuff);

        // 콘솔 출력
        for (int i = 0; i < receivedBytes; i++)
        {
            printf("%2x ", readBuff[i]);
        }
        fflush(stdout);

        // Server Echo
        memcpy(sendBuff, readBuff, receivedBytes);
        sentBytes = sendto(server_fd, sendBuff, receivedBytes, 0, (struct sockaddr*) & clientAddress, sizeof(clientAddress));

    }

    // 서버 소켓 close
    close(server_fd);

    return 0;
}

//char 출력
void* udpServer2(void* arg)
{
    char readBuff[RX_BUFFER_SIZE];
    char sendBuff[RX_BUFFER_SIZE];
    char* turnonString = "TURN ON";
    char* turnoffString = "TURN OFF";
    struct sockaddr_in serverAddress, clientAddress;
    int server_fd, client_fd;
    socklen_t client_addr_size;
    ssize_t receivedBytes;
    ssize_t sentBytes;

    memset(&serverAddress, 0, sizeof(serverAddress));
    memset(&clientAddress, 0, sizeof(clientAddress));

    serverAddress.sin_family = AF_INET;
    //serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_addr.s_addr = inet_addr("192.168.10.113");
    serverAddress.sin_port = htons(atoi((char*)arg));


    // 서버 소켓 생성(UDP)
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) // SOCK_DGRAM : UDP
    {
        printf("Sever : can not Open Socket\n");
        exit(0);
    }

    // bind
    if (bind(server_fd, (struct sockaddr*) & serverAddress, sizeof(serverAddress)) < 0)
    {
        printf("Server : can not bind local address");
        exit(0);
    }

    printf("UDP Server Start. IP:%s Port:%d\n", inet_ntoa(serverAddress.sin_addr), ntohs(serverAddress.sin_port));

    while (1)
    {
        client_addr_size = sizeof(clientAddress);

        // 패킷 수신
        receivedBytes = recvfrom(server_fd, readBuff, RX_BUFFER_SIZE, 0, (struct sockaddr*) & clientAddress, &client_addr_size);
        printf("\n%lu bytes read\n", receivedBytes);
        readBuff[receivedBytes] = '\0';

        // 현재 스레드 서버 IP출력
        printf("Server :  IP:%s Port:%d\n", inet_ntoa(serverAddress.sin_addr), ntohs(serverAddress.sin_port));

        // 클라이언트 IP 확인
        struct sockaddr_in connectSocket;
        socklen_t connectSocketLength = sizeof(connectSocket);
        getpeername(client_fd, (struct sockaddr*) & clientAddress, &connectSocketLength);
        char clientIP[sizeof(clientAddress.sin_addr) + 20] = { 0 };
        sprintf(clientIP, "IP:%s port:%d ", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
        printf("Client : %s\n", clientIP);

        // 콘솔 출력
        fputs(readBuff, stdout);
        fflush(stdout);

        // Server Echo
        sprintf(sendBuff, "%s", readBuff);
        sentBytes = sendto(server_fd, sendBuff, strlen(sendBuff), 0, (struct sockaddr*) & clientAddress, sizeof(clientAddress));

        // UART 출력
        //serialPutString(readBuff, receivedBytes);
        //serialPuts(serialPort, readBuff);

        if (strncmp(readBuff, turnonString, strlen(turnonString)) == 0)
        {
            unsigned char tmpTxUartBuff[] = { 0x30, 0x31, 0xA0, 0x08, 24, 1, 0xB1, 0x32 };
            serialPutString(tmpTxUartBuff, sizeof(tmpTxUartBuff));
        }
        else if (strncmp(readBuff, turnoffString, strlen(turnoffString)) == 0)
        {
            unsigned char tmpTxUartBuff[] = { 0x30, 0x31, 0xA0, 0x08, 24, 0, 0xB0, 0x32 };
            serialPutString(tmpTxUartBuff, sizeof(tmpTxUartBuff));
        }

    }

    // 서버 소켓 close
    close(server_fd);

    return 0;
}

