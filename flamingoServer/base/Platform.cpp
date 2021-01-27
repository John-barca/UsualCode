#include "platform.h" 
// 当前平台是windows平台时
#ifdef WIN32

NetworkInitializer::NetworkInitializer()
{
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    ::WSAStartup(wVersionRequested, &wsaData);   
}

NetworkInitializer::~NetworkInitializer()
{
    ::WSACleanup();
}

#endif