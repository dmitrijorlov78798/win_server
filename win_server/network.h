#pragma once
#ifndef NETWORK_H_
#define NETWORK_H_

#include <vector>
#include <list>
#include <map>
#include <string>

#include "log.h"

#ifdef __WIN32__

#include <unordered_set>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h> // ������������ ����, ���������� ���������� ���������� ������� ��� ������ � ��������
#include <WS2tcpip.h> // ������������ ����, ������� �������� ��������� ����������� ����������, ��������� � ������� ��������� TCP/IP (�������� ��������� ������ � ������, ���������� ���������� � �.�.)
#include <iphlpapi.h>
#pragma comment(lib, "Ws2_32.lib") // ������������ � ���������� ������������ ���������� ���� ��: ws2_32.dll. ������ ��� ����� ��������� �����������
#define CLOSE_SOCKET(socket) closesocket(socket)

#else

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define CLOSE_SOCKET(socket) close(socket)

#endif

/// <summary>
/// ����������� ���� ������� ��� ������ � �����
/// </summary>
namespace network
{
    /// <summary>
    /// ����� ���������� �� ������������ �������
    /// ���������� ������� RAII
    /// </summary>
    class RAII_OSsock
    {
    private :
#ifdef __WIN32__
        static WSADATA wsdata; // ��������� WSADATA �������� �������� � ���������� ������� Windows.
        static int countWSAusers; // ���������� ������������� ����������
        unsigned objectID; // ID ����� �������
        std::unordered_set<unsigned>& journal; // ������ ����������� �������� �������� �������������� 
#endif
    protected :
        struct option_t // ����� ��� ������
        {
            static const int NON_BLOCK = 1; // ������������� �����
        };
        struct error_t // ������ ������
        {
#ifdef __WIN32__
            static constexpr int NON_BLOCK_SOCKET_NOT_READY = WSAEWOULDBLOCK; // ����� �� �����������, �� ����� � �������� ���� ������
#else
            static const int NON_BLOCK_SOCKET_NOT_READY = EWOULDBLOCK; // ����� �� �����������, �� ����� � �������� ���� ������
#endif
        };
        /// <summary>
        /// ����������� �� ���������
        /// </summary>
        RAII_OSsock(log_t& logger);

        virtual ~RAII_OSsock();
        /// <summary>
        /// ����� ������ ������ ��������� ������
        /// </summary>
        /// <returns> ����� ��������� ������ </returns>
        int GetError();

        /// <summary>
        /// ����� ��������� ����� ��� ������
        /// </summary>
        /// <param name="socket"> - ���������� ������ </param>
        /// <param name="option"> - �����</param>
        /// <param name="logger"> - ������ ��� ����������� </param>
        /// <returns> 1 - ����� </returns>
        bool setSocketOpt(SOCKET socket, int option, log_t& logger);
    };

    /// <summary>
    /// ����� ��������� �������� ���������� � ������
    /// </summary>
    class sockInfo_t : public RAII_OSsock
    {
        friend class UDP_socket_t; // ��� ������ RecvFrom
        friend class TCP_socketServer_t; // ��� ������ AddClient
        friend class TCP_socketClient_t; // ��� ������ Move
    protected:
        /// <summary>
        /// ����� ���������� ��������� ����������� �����. ����� ���������� ���������� �������� ��������� � ������ ������ UpdateSockInfo()
        /// </summary>
        /// <returns> ��������� �� ��������� ����������� ����� </returns>
        sockaddr* setSockAddr();

        /// <summary>
        /// ����� ���������� ����������� � ������, ���������� ����� ���������� Addr ������ setSockAddr()
        /// </summary>
        void UpdateSockInfo();

        /// <summary>
        /// ����� ���������� ����������� � ������, ���������� ����� ���������� Addr
        /// </summary>
        /// <param name="ip"> IP ������ � ������� "����.����.����.����" </param>
        /// <param name="port"> ����� ����� </param>
        void UpdateSockInfo(std::string ip, unsigned short port);

    public:
        /// <summary>
        /// ����������� � ����� ����������
        /// </summary>
        /// <param name="logger"> - ������ ��� ������������ ������ </param>
        sockInfo_t(log_t& logger);

        /// <summary>
        /// ����������� � 2-� �����������
        /// </summary>
        /// <param name="ip"> - IP ������ � ������� "����.����.����.����" </param>
        /// <param name="port"> - ����� ����� </param>
        /// <param name="logger"> - ������ ��� ������������ ������ </param>
        sockInfo_t(std::string ip, unsigned short port, log_t& logger);

        virtual ~sockInfo_t();

        /// <summary>
        /// ����� ��������� ���������� � ������
        /// </summary>
        /// <param name="ip"> - IP ������ � ������� "����.����.����.����" </param>
        /// <param name="port"> - ����� �����</param>
        /// <returns> true - �����; false - ������� </returns>
        bool setSockInfo(std::string ip, unsigned short port);

        /// <summary>
        /// ����� ��������� ���������� � ������
        /// </summary>
        /// <param name="sockInfo"> - ��������� ����������� ����� ��� ������ � ����������� IP </param>
        /// <returns></returns>
        void setSockInfo(const sockInfo_t& sockInfo);

        /// <summary>
        /// ����� ���������� ������������ ��������� �� ��������� ����������� �����
        /// </summary>
        /// <returns> ����������� ��������� �� ��������� ����������� ����� </returns>
        const sockaddr* getSockAddr() const;

        /// <summary>
        /// ����� �������� ������� ��������� ����������� �����
        /// </summary>
        /// <returns> ������ ��������� ����������� ����� </returns>
        size_t SizeAddr() const;

        /// <summary>
        /// ����� �������� IP
        /// </summary>
        /// <returns> IP ������ � ������� "����.����.����.����" </returns>
        std::string GetIP() const;

        /// <summary>
        /// ����� �������� ������ �����
        /// </summary>
        /// <returns> ����� ����� </returns>
        unsigned short GetPort() const;

        /// <summary>
        /// �������� ���������
        /// </summary>
        /// <param name="rValue"> - �������������� �������� </param>
        /// <returns> 1 - ������� ����� </returns>
        bool operator == (const sockInfo_t& rValue) const;

        /// <summary>
        /// �������� ���������
        /// </summary>
        /// <param name="rValue"> - �������������� �������� </param>
        /// <returns> 1 - ������� �� ����� </returns>
        bool operator != (const sockInfo_t& rValue) const;
    protected:
        std::pair<std::string, unsigned short> IP_port; // IP ����� � ����� �����
        sockaddr Addr; // ����������� ��������� ��� �������� ���������� � ������
        size_t sizeAddr; // ������ ��������� sockaddr
        log_t& logger; // ������ ��� ������������ ������
    };

    /// <summary>
    /// ����� ��������� �������� � ������ ������
    /// </summary>
    class socket_t : public sockInfo_t
    {// TODO ������ � DNS    getaddrinfo(char const* node, char const* service, struct addrinfo const* hints, struct addrinfo** res)
        friend class NonBlockSocket_manager_t; // �������� ������������� �������, ���������� setNonBlock
    protected:
        /// <summary>
        /// ����� ������ ����������� ������ (��� ����������� ����������� ��������� ������� TCP)
        /// </summary>
        /// <param name="socket"> - ���������� ������ </param>
        /// <param name="nonBlock"> - ���� �������������� ������ </param>
        /// <returns> true - ��� ������� ������, false - ��� ��������� </returns>
        bool SetSocket(SOCKET socket, bool nonBlock);

        /// <summary>
        /// ����� �������� ������
        /// </summary>
        /// <returns> true - ����� ������ </returns>
        bool Close();

        /// <summary>
        /// ��������� ������� �����, �� �������� ��� ����� �������� ������������ ��������� ��
        /// ������� �������������� ���������, � ����� ��������� ����, �� �������� ���
        /// �������������� �������� ����� ���������������� �������-����������.
        /// �������������� ��� ���������� � �������� ������ ��� ��������.
        /// </summary>
        /// <returns> true - �����, false - ������� </returns>
        bool Bind();

        /// <summary>
        /// ��������� ������� �����, �� �������� ��� ����� �������� ������������ ��������� ��
        /// ������� �������������� ���������, � ����� ��������� ����, �� �������� ���
        /// �������������� �������� ����� ���������������� �������-����������.
        /// </summary>
        /// <param name="ip"> - IP ������ � ������� "����.����.����.����" </param>
        /// <param name="port"> - ����� ����� </param>
        /// <returns> 1 - ��������� </returns>
        bool Bind(std::string ip, unsigned short port);

        /// <summary>
        /// ����������� � ����� ����������, ��� �������� ������� ������� ��� ��������� ������
        /// </summary>
        /// <param name="logger"> - ������ ��� ������������ </param>
        socket_t(log_t& logger);

        /// <summary>
        /// ����������� � 4-� �����������
        /// </summary>
        /// <param name="af"> - ��������� �������: ������ ����� �������� � ������� ���������� �������. �������� ������ ��������� � IPv4.
        ///  ����������� ��� AF_INET </param>
        /// <param name="type"> - �� ������: ������ �������� ��� ������������� ��������� TCP (SOCK_STREAM) ��� UDP (SOCK_DGRAM).
        ///  �� ������ � ��� ���������� "�����" ������, ���������� ������� ��� ����������� ���������� � �������� �������������.
        ///  ��� ������������ SOCK_RAW </param>
        /// <param name="protocol"> - ��� ���������: �������������� ��������, ���� ��� ������ ������ ��� TCP ��� UDP � ����� �������� �������� 0.
        /// </param>
        /// <param name="logger"> - ������ ��� ������������ ������ </param>
        socket_t(int af, int type, int protocol, log_t& logger);

        /// <summary>
        ///  ����������� � 6 �����������
        /// </summary>
        /// <param name="af"> - ��������� �������: ������ ����� �������� � ������� ���������� �������. �������� ������ ��������� � IPv4.
        ///  ����������� ��� AF_INET </param>
        /// <param name="type"> - ��� ������: ������ �������� ��� ������������� ��������� TCP (SOCK_STREAM) ��� UDP (SOCK_DGRAM).
        ///  �� ������ � ��� ���������� "�����" ������, ���������� ������� ��� ����������� ���������� � �������� �������������.
        ///  ��� ������������ SOCK_RAW </param>
        /// <param name="protocol"> - ��� ���������: �������������� ��������, ���� ��� ������ ������ ��� TCP ��� UDP � ����� �������� �������� 0.
        /// </param>
        /// <param name="ip"> - IP ������ � ������� "����.����.����.����" </param>
        /// <param name="port"> - ����� ����� </param>
        /// <param name="logger"> - ������ ��� ������������ ������ </param>
        socket_t(int af, int type, int protocol, std::string ip, unsigned short port, log_t& logger);

        /// <summary>
        ///  ����������� � 5 �����������
        /// </summary>
        /// <param name="af"> - ��������� �������: ������ ����� �������� � ������� ���������� �������. �������� ������ ��������� � IPv4.
        ///  ����������� ��� AF_INET </param>
        /// <param name="type"> - �� ������: ������ �������� ��� ������������� ��������� TCP (SOCK_STREAM) ��� UDP (SOCK_DGRAM).
        ///  �� ������ � ��� ���������� "�����" ������, ���������� ������� ��� ����������� ���������� � �������� �������������.
        ///  ��� ������������ SOCK_RAW </param>
        /// <param name="protocol"> - ��� ���������: �������������� ��������, ���� ��� ������ ������ ��� TCP ��� UDP � ����� �������� �������� 0.
        /// </param>
        /// <param name="sockInfo"> - ������ ���������� ���������� � ������
        /// <param name="logger"> - ������ ��� ������������ ������ </param>
        socket_t(int af, int type, int protocol, sockInfo_t sockInfo, log_t& logger);

        // ������ - ���������� ������, ������� ����������� ��������
        socket_t(const socket_t& sock) = delete;
        socket_t& operator = (const socket_t& sock) = delete;

        /// <summary>
        /// ����� �������� ���������� ������
        /// </summary>
        /// <param name="logOn"> - ���� �� ������� ������������ ������������ ������ </param>
        /// <returns> true - ����� �������, false - ����� �� ������� </returns>
        bool CheckValidSocket(bool logOn = true);

        virtual ~socket_t();

        /// <summary>
        /// ����� �������� ����������� ������
        /// </summary>
        /// <returns> ���������� ������ </returns>
        SOCKET getSocket() const;

        /// <summary>
        /// ���������� ����� �������������
        /// </summary>
        /// <returns> 1 - ����� �� ����������� </returns>
        bool setNonBlock();
    protected:
        SOCKET Socket; // ���������� ������
        bool nonBlock; // ������� �������������� ������
    };

    /// <summary>
    /// TCP ���������� �����
    /// </summary>
    class TCP_socketClient_t : private socket_t
    {
        friend class TCP_socketServer_t; // ���� ������ ������� ���������� ������ (���������� ��� ac�ept())
    private:
        /// <summary>
        /// �������� �����, ������ ����� �������� ������� ������������ �������, �� ��������� ��� ���������� ������ ��� ac�ept()
        /// </summary>
        /// <param name="socket"> - ����� ���������� ������ </param>
        /// <param name="sockInfo"> - ����������� ���������� </param>
        /// <returns> true - �������� ������ </returns>
        bool SetSocket(SOCKET socket, sockInfo_t sockInfo);

    public:
        /// <summary>
        /// ����� �������� ����������� ������� (����������) ����� ��������
        /// </summary>
        /// <param name="source"> - ������ �� ����� �������� - ����� ������ ������ �� �������� �������� ����� </param>
        void Move(TCP_socketClient_t& source);

        /// <summary>
        /// ����������� � 1 ����������
        /// </summary>
        /// <param name="logger"> - ������ ��� ������������ </param>
        TCP_socketClient_t(log_t& logger);

        /// <summary>
        /// ����������� � 3 �����������
        /// </summary>
        /// <param name="ip_server"> - IP ����� ������� � ������� "����.����.����.����" </param>
        /// <param name="port_server"> - ����� ����� ������� </param>
        /// <param name="logger"> - ������ ������������ </param>
        TCP_socketClient_t(std::string ip_server, unsigned short port_server, log_t& logger);

        /// <summary>
        /// ���������� � 2 �����������
        /// </summary>
        /// <param name="serverSockInfo"> - ���������� � ������� </param>
        /// <param name="logger"> - ������ ������������ </param>
        TCP_socketClient_t(sockInfo_t serverSockInfo, log_t& logger);

        /// <summary>
        /// ����� ������ ��������� � ������������ ������ � ���������� �������� ������� ��������� ���������
        /// </summary>
        /// <param name="str_bufer"> - ����� ��� ������ ������ </param>
        /// <param name="str_EndOfMessege"> - ������ �������� ������������ ����� ��������� (�����������) </param>
        /// <param name="sizeMsg"> - ������ ���������� ��������� (�����������) </param>
        /// <returns> 0 - ��������� ������� ��������;
        ///           N>0 - ������� N ����;
        ///           -1 - ��������� ������;
        ///           -2 - ���������� ������� ��� ���������� �����;
        ///           -3 - ������ �� ����� ���(������������� �����)</returns>
        int Recive(std::string& str_bufer, const std::string str_EndOfMessege = "", const size_t sizeMsg = 0);

        /// <summary>
        /// ����� �������� ��������� � ������������ ������ � ���������� �������� ������� ������������� ���������
        /// </summary>
        /// <param name="str_bufer"> - �����, ���������� ������ ��� �������� </param>
         /// <returns> 0 - ��������� ���������� ��������;
        ///           N>0 - ���������� N ����;
        ///           -1 - ��������� ������;
        ///           -2 - ���������� ������� ��� ���������� �����;
        ///           -3 - ����� �� ����� � �������� (������������� �����)</returns>
        int Send(const std::string& str_bufer);

        /// <summary>
        /// ����� ����������� ������ � ���������� ������
        /// </summary>
        /// <returns> 1 - ����� ��������� </returns>
        bool Connected();

        /// <summary>
        /// ����� �������� ��������� ����������
        /// </summary>
        /// <returns> 1 - ���������� ���� </returns>
        bool GetConnected() const;
    private:
        bool b_connected; // ������� ����������� ������ � �������
        sockInfo_t serverInfo; // ���������� � �������
    };

    /// <summary>
    /// TCP ��������� �����
    /// </summary>
    class TCP_socketServer_t : private socket_t
    {
    public:
        /// <summary>
        /// ����������� � 3-� �����������
        /// </summary>
        /// <param name="ip"> - IP ������ � ������� "����.����.����.����" </param>
        /// <param name="port"> - ����� ����� </param>
        /// <param name="logger"> - ������ ������������ </param>
        TCP_socketServer_t(std::string ip, unsigned short port, log_t& logger);

        /// <summary>
        /// ����������� � 2-� �����������
        /// </summary>
        /// <param name="sockInfo"> - ���������� � ������ </param>
        /// <param name="logger"> - ������ ������������ </param>
        TCP_socketServer_t(sockInfo_t sockInfo, log_t& logger);

        /// <summary>
        /// ����� ���������� ������������ ��������
        /// </summary>
        /// <param name="client"> - ������ �� ������� ��� ������ �� ������������� ������� </param>
        /// <returns> 0 - �������� ������������ ������,
        ///          -1 - ��������� ������,
        ///          -2 - ��� �������� � ������� �� ����������� (������������� �����)</returns>
        int AddClient(TCP_socketClient_t& client);
    };

    /// <summary>
    /// UDP �����
    /// </summary>
    class UDP_socket_t : public socket_t
    {
    private:
        /// <summary>
        /// ����� ��������� MTU
        /// </summary>
        void setMTU();
    public:

        /// <summary>
        /// ����� �������� ����������� ������� (����������) ����� ��������
        /// </summary>
        /// <param name="source"> - ������ �� ����� �������� - ����� ������ ������ �� �������� �������� ����� </param>
        void Move(UDP_socket_t& source);

        /// <summary>
        /// ����������� � 1 ����������
        /// </summary>
        /// <param name="logger"> - ������ ��� ������������ </param>
        UDP_socket_t(log_t& logger);

        /// <summary>
        /// ����������� � 3-� �����������
        /// </summary>
        /// <param name="ip"> - IP ������ � ������� "����.����.����.����" </param>
        /// <param name="port"> - ����� ����� </param>
        /// <param name="logger"> - ������ ������������ </param>
        UDP_socket_t(std::string ip, unsigned short port, log_t& logger);

        /// <summary>
        /// ����������� � 2-� �����������
        /// </summary>
        /// <param name="sockInfo"> - ���������� � ������ </param>
        /// <param name="logger"> - ������ ������������ </param>
        UDP_socket_t(sockInfo_t& sockInfo, log_t& logger);

        /// <summary>
        /// ����� �������� ������ � ���� ��� ���������������� ����������
        /// </summary>
        /// <param name="buffer"> - ������� � ������� ��� �������� </param>
        /// <param name="target"> - ���������� � ������ ��������� </param>
        /// <returns> 0 - ���������� ��� ���������;
        ///         N>0 - ���������� N ����;
        ///          -1 - ��������� ������;
        ///          -2 - ������ ��������� ������ MTU ��� ����� �� ��������;
        ///          -3 - ����� �� ����� � �������� (������������� �����) </returns>
        int SendTo(const std::string& buffer, sockInfo_t target);

        /// <summary>
        /// ����� �������� ������ � ���� ��� ���������������� ����������, �������� ����������� ������ � ������� ���� ��������������
        /// </summary>
        /// <param name="buffer"> - ������� � ������� ��� �������� </param>
        /// <returns> 0 - ���������� ��� ���������;
        ///         N>0 - ���������� N ����;
        ///          -1 - ��������� ������;
        ///          -2 - ������ ��������� ������ MTU ��� ����� �� ��������;
        ///          -3 - ����� �� ����� � �������� (������������� �����) </returns>
        int SendTo(const std::string& buffer);

        /// <summary>
        /// ����� �������� ������ � ���� ��� ���������������� ����������
        /// </summary>
        /// <param name="buffer"> - ������� � ������� ��� �������� </param>
        /// <param name="ip"> - IP ������ � ������� "����.����.����.����" </param>
        /// <param name="port"> - ����� ����� </param>
        /// <returns> 0 - ���������� ��� ���������;
        ///         N>0 - ���������� N ����;
        ///          -1 - ��������� ������;
        ///          -2 - ������ ��������� ������ MTU ��� ����� �� ��������;
        ///          -3 - ����� �� ����� � �������� (������������� �����) </returns>
        int SendTo(const std::string& buffer, std::string ip, unsigned short port);

        /// <summary>
        /// ����� ������ ������ �� ���� ��� ���������������� ����������
        /// </summary>
        /// <param name="buffer"> - ����� � ��������� ������� </param>
        /// <param name="str_EndOfMessege"> - ������� ����� ��������� (�����������) </param>
        /// <param name="sizeMsg"> - �������� ������ ��������� (�����������) </param>
        /// <returns>   0 - ��� ��������� ������� �������;
        ///             N>0 - ������� N-����;
        ///             -1 - ��������� ������;
        ///             -2 - ����������� ������� ��� ����� �� ��������;
        ///             -3 - ����� �� ����� (�������������);</returns>
        int RecvFrom(std::string& buffer, const std::string str_EndOfMessege = "", const size_t sizeMsg = 0);

        /// <summary>
        /// ����� �������� ���������� � ������ � ������� ����������� ��������� �������������� (��������/����� ������)
        /// </summary>
        /// <returns> ����� � ������� ����������� ��������� �������������� (��������/����� ������) </returns>
        sockInfo_t GetLastCommunication() const;

        /// <summary>
        /// ����� �������� MTU
        /// </summary>
        /// <returns> ������������ ������ ��������� </returns>
        unsigned int MTU() const;
    private:
        sockInfo_t lastCommunicationSocket; // ��������� �����, � ��� ����������� ��������������
        unsigned int u32_MTU; // ������������ ������ ������������ ������
    };

    /// <summary>
    /// ����� ������������������� ������������� �������
    /// </summary>
    class NonBlockSocket_manager_t : private RAII_OSsock
    {
    protected:
        /// <summary>
        /// ����� ���������� ������ � ���� �� �������
        /// </summary>
        /// <param name="m_sock"> - ������������� ������ ��� ���������� </param>
        /// <param name="socket"> - ����� �� ���������� </param>
        /// <returns> 1 - ����� �������� </returns>
        bool addSocket(std::map<int, std::shared_ptr<socket_t>>& m_sock, std::shared_ptr<socket_t> socket);

        /// <summary>
        /// ����� �������� ������ �� �������
        /// </summary>
        /// <param name="m_sock"> - ������������� ������ ��� ���������� </param>
        /// <param name="socket"> - ����� �� �������� </param>
        /// <returns> 1 - ����� ������ </returns>
        bool deleteSocket(std::map<int, std::shared_ptr<socket_t>>& m_sock, std::shared_ptr<socket_t> socket);

        /// <summary>
        /// ����� ���������� ��������� pollfd
        /// </summary>
        void UpdatePollfd();

        /// <summary>
        /// ������� ������������������ ������������� �������
        /// </summary>
        /// <param name="timeOut"> - ����� timeout ��� ������� ������������������� </param>
        /// <returns> -1 - ��������� ������; 0 - ����� ������� � ������� �� ���������; N>0 - ���-�� ������� </returns>
        int Poll(int timeOut);
    public:
        /// <summary>
        /// ����������� � ����� ����������
        /// </summary>
        /// <param name="logger"> - ������ ��� ������������ </param>
        NonBlockSocket_manager_t(log_t& logger);

        /// <summary>
        /// ����������� � ����� �����������
        /// </summary>
        /// <param name="size"> - ��������������� ���������� ����������� ������� </param>
        /// <param name="logger"> - ������ ��� ����������� </param>
        NonBlockSocket_manager_t(int size, log_t& logger);

        /// <summary>
        /// ����� ���������� �����������
        /// </summary>
        /// <param name="socket"> - ������� ����� </param>
        /// <returns> 1 - ����� �������� </returns>
        bool AddSender(std::shared_ptr<socket_t> socket);

        /// <summary>
        /// ����� �������� �����������
        /// </summary>
        /// <param name="socket"> - ������� ����� </param>
        /// <returns> 1 - ����� ������ </returns>
        bool deleteSender(std::shared_ptr<socket_t> socket);

        /// <summary>
        /// ����� ���������� ��������
        /// </summary>
        /// <param name="socket"> - ������� ����� </param>
        /// <returns> 1 - ����� �������� </returns>
        bool AddReader(std::shared_ptr<socket_t> socket);

        /// <summary>
        /// ����� �������� ��������
        /// </summary>
        /// <param name="socket"> - ������� ����� </param>
        /// <returns> 1 - ����� ������ </returns>
        bool deleteReader(std::shared_ptr<socket_t> socket);

        /// <summary>
        /// ����� ���������� �������
        /// </summary>
        /// <param name="socket"> - ������� ����� </param>
        /// <returns> 1 - ����� �������� </returns>
        bool AddServer(std::shared_ptr<socket_t> socket);

        /// <summary>
        /// ����� �������� �������
        /// </summary>
        /// <param name="socket"> - ������� ����� </param>
        /// <returns> 1 - ����� ������ </returns>
        bool deleteServer(std::shared_ptr<socket_t> socket);

        /// <summary>
        /// ����� ���������� �������
        /// </summary>
        /// <param name="socket"> - ������� ����� </param>
        /// <returns> 1 - ����� �������� </returns>
        bool AddClient(std::shared_ptr<socket_t> socket);

        /// <summary>
        /// ����� �������� �������
        /// </summary>
        /// <param name="socket"> - ������� ����� </param>
        /// <returns> 1 - ����� ������ </returns>
        bool deleteClient(std::shared_ptr<socket_t> socket);

        /// <summary>
        /// ����� �������� ���������� ����������� � ��������
        /// </summary>
        /// <param name="socket"> - ������� ����� </param>
        /// <returns> 1 - ����� ����� </returns>
        bool GetReadySender(std::shared_ptr<socket_t> socket);

        /// <summary>
        /// ����� �������� ���������� �������� � ������
        /// </summary>
        /// <param name="socket"> - ������� ����� </param>
        /// <returns> 1 - ����� ����� </returns>
        bool GetReadyReader(std::shared_ptr<socket_t> socket);

        /// <summary>
        /// ����� �������� ������� ������� �� ����������� � �������
        /// </summary>
        /// <param name="socket"> - ������� ����� </param>
        /// <returns> 1 - ���� ����������� </returns>
        bool GetReadyServer(std::shared_ptr<socket_t> socket);

        /// <summary>
        /// ����� �������� ������������� ����������� � ������� � �������
        /// </summary>
        /// <param name="socket"> - ������� ����� </param>
        /// <returns> 1 - ���� ����������� </returns>
        bool GetReadyClient(std::shared_ptr<socket_t> socket);

        /// <summary>
        /// �������� ����� ������ �������������
        /// </summary>
        /// <param name="timeOut"> - ����� �������� ������������� </param>
        /// <returns> 1 - ������� ���� �� ���� ������� </returns>
        bool Work(const int timeOut);
    protected:
        std::vector <struct pollfd> v_fds; // ������������ ������ �������� pollfd
        std::map<int, std::shared_ptr<socket_t>> m_senderSocket; // ��� ������� ��������� ��������
        std::map<int, std::shared_ptr<socket_t>> m_readerSocket; // ��� ������� ��������� �����
        std::map<int, std::shared_ptr<socket_t>> m_serverSocket; // ��� ������� ��������� �������� �����������
        std::map<int, std::shared_ptr<socket_t>> m_clientSocket; // ��� ������� ��������� ��������� �����������
        std::map<int, std::shared_ptr<socket_t>> m_readySender; // ��� ������� ������������
        std::map<int, std::shared_ptr<socket_t>> m_readyReader; // ��� ������� ���������
        std::map<int, std::shared_ptr<socket_t>> m_readyServer; // ��� ������� ��������
        std::map<int, std::shared_ptr<socket_t>> m_readyClient; // ��� ������� ��������
        bool b_change; // ���� ��������� �������� pollfd
        log_t& logger; // ������ ������������
    };
};






#endif /* NETWORK_H_ */
