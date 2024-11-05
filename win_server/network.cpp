#include "network.h"

#ifdef __WIN32__
std::unordered_set<unsigned> g_journal; // ������ ��� ����������� �������� � ������� ���������������
#endif

/// <summary>
/// ����������� �� ���������
/// </summary>
network::RAII_OSsock::RAII_OSsock(log_t& logger) : journal(g_journal)
{
#ifdef __WIN32__
    if (journal.empty()) // ���� ��������� ��� �� ��������� � ���������� ������������� = 0
        if (WSAStartup(MAKEWORD(2, 2), &wsdata))// ������� ���������
            logger.doLog("RAII_OSsock - WSAStartup ", GetError());// ��������� ������

    objectID = ++countWSAusers; // ������� ���������� ID
    journal.insert(objectID); // �������������� 
#endif
}
network::RAII_OSsock::~RAII_OSsock()
{
#ifdef __WIN32__
    journal.erase(objectID); // ������� ���� ������, ���� ��� �� ������� ����� ������ (Move ���������)

    if (journal.empty()) // ��� ������ ������ �������� ��������� ����
        WSACleanup(); // ������� ���������
#endif
}

/// <summary>
/// ����� ������ ������ ��������� ������
/// </summary>
/// <returns> ����� ��������� ������ </returns>
int network::RAII_OSsock::GetError()
{
#ifdef __WIN32__
    return WSAGetLastError();
#else
    return errno;
#endif
}

/// <summary>
/// ����� ��������� ����� ��� ������
/// </summary>
/// <param name="socket"> - ���������� ������ </param>
/// <param name="option"> - �����</param>
/// <param name="logger"> - ������ ��� ����������� </param>
/// <returns> 1 - ����� </returns>
bool network::RAII_OSsock::setSocketOpt(SOCKET sock, int option, log_t& logger)
{
    bool result = false;

    switch (option)
    {
    case option_t::NON_BLOCK: // ����� �� ���������� �������������� ������
    {
#ifdef __WIN32__
        u_long mode = 0;
        if (ioctlsocket(sock, FIONBIO, &mode))
            logger.doLog("RAII_OSsock - ioctlsocket ", GetError());// ��������� ������
        else
            result = true;
#else
        if (fcntl(sock, F_SETFL, O_NONBLOCK))
            logger.doLog("RAII_OSsock - ioctl ", GetError());// ��������� ������
        else
            result = true;
#endif
    }
    default:
        break;
    }

    return result;
}

#ifdef __WIN32__
WSADATA network::RAII_OSsock::wsdata;
int network::RAII_OSsock::countWSAusers = 0;
#endif


/// <summary>
/// ����� ���������� ��������� ����������� �����. ����� ���������� ���������� �������� ��������� � ������ ������ UpdateSockInfo()
/// </summary>
/// <returns> ��������� �� ��������� ����������� ����� </returns>
sockaddr* network::sockInfo_t::setSockAddr()
{
    return &Addr;
}

/// <summary>
/// ����� ���������� ����������� � ������, ���������� ����� ���������� Addr ������ setSockAddr()
/// </summary>
void network::sockInfo_t::UpdateSockInfo()
{
    // ������ ������ ���������� � �������� ������
    IP_port.first.clear();
    IP_port.second = 0;

    sockaddr_in AddrIN; // ��������� ��������� ����� ��� ������ � ����������� IP
    memset(&AddrIN, 0, SizeAddr());
    memmove(&AddrIN, &Addr, SizeAddr()); //Addr ==>> AddrIN

    char bufIP[32];// ����� ��� ������ IP
    memset(bufIP, '\0', sizeof(bufIP));

    // ������� InetNtop ����������� ��������-����� IPv4 ��� IPv6 � ������ � ����������� ������� ���������
    if (inet_ntop(AF_INET, &AddrIN, bufIP, sizeof(bufIP)))
    {
        IP_port.first.assign(bufIP);
        IP_port.second = ntohs(AddrIN.sin_port);
    }
    else // ��������� ������
        logger.doLog("inet_ntop fail", GetError());
}

/// <summary>
/// ����� ���������� ����������� � ������, ���������� ����� ���������� Addr
/// </summary>
/// <param name="ip"> IP ������ � ������� "����.����.����.����" </param>
/// <param name="port"> ����� ����� </param>
void network::sockInfo_t::UpdateSockInfo(std::string ip, unsigned short port)
{
    IP_port.first.swap(ip);
    IP_port.second = port;
}

/// <summary>
/// ����������� � ����� ����������
/// </summary>
/// <param name="logger"> - ������ ��� ������������ ������ </param>
network::sockInfo_t::sockInfo_t(log_t& logger) : RAII_OSsock(logger), logger(logger)
{
    memset(&Addr, 0, sizeof(Addr));
    IP_port.first.clear();
    IP_port.second = 0;
    sizeAddr = sizeof(Addr);
}

/// <summary>
/// ����������� � 2-� �����������
/// </summary>
/// <param name="ip"> - IP ������ � ������� "����.����.����.����" </param>
/// <param name="port"> - ����� ����� </param>
/// <param name="logger"> - ������ ��� ������������ ������ </param>
network::sockInfo_t::sockInfo_t(std::string ip, unsigned short port, log_t& logger) : sockInfo_t(logger)
{
    setSockInfo(ip, port);
}

/// <summary>
/// ����������
/// </summary>
network::sockInfo_t::~sockInfo_t()
{
    memset(&Addr, 0, sizeof(Addr));
    IP_port.first.clear();
    IP_port.second = 0;
}

/// <summary>
/// ����� ��������� ���������� � ������
/// </summary>
/// <param name="ip"> - IP ������ � ������� "����.����.����.����" </param>
/// <param name="port"> - ����� �����</param>
/// <returns> true - �����; false - ������� </returns>
bool network::sockInfo_t::setSockInfo(std::string ip, unsigned short port)
{
    bool result = false; // ���������

    sockaddr_in AddrIN; // ��������� ��������� ����� ��� ������ � ����������� IP
    AddrIN.sin_family = AF_INET; // ��������� �������
    AddrIN.sin_port = htons(port); // ����� ����� ������������� ���������
    // ������� htons ���������� �������� � ������� ������ ���� TCP/IP

    //AddrIN.sin_addr - ��������� IN_ADDR , ���������� ������������ ����� IPv4.
    // ���������:
    // NADDR_ANY ��� ������ ���������� �����(0.0.0.0);
    // INADDR_LOOPBAC ����� loopback ����������(127.0.0.1);
    // INADDR_BROADCAST ����������������� �����(255.255.255.255)
    int inet_pton_state = inet_pton(AF_INET, ip.c_str(), &AddrIN.sin_addr);
    // ������� InetPton ����������� ������� ����� IPv4 ��� IPv6 � ����������� �����
    //������������� ������ � �������� �������� �����
    // ������� 1 - �����, 0 - �������� ������, -1 - ������

    // ��������� ��������� ������ �������
    if (inet_pton_state == 1) // ��� �������
    {
        memset(&Addr, 0, SizeAddr());
        memmove(&Addr, &AddrIN, SizeAddr()); //AddrIN ==>> Addr
        result = true;
        UpdateSockInfo(ip, port);
    }
    else if (inet_pton_state == 0) // ������� ����� IP
        logger.doLog(std::string("setSockAddr Fail, invalid IP: ") + ip);
    else //inet_pton fail
        logger.doLog("inet_pton Fail,IP: " + ip, GetError());

    if (result)// ����� ���������� ���������� ��� ���������� ������ �������
    {
        DEBUG_TRACE(logger, "setSockAddr: -> OK")
    }

    return result;
}

/// <summary>
/// ����� ��������� ���������� � ������
/// </summary>
/// <param name="sockInfo"> - ��������� ����������� ����� ��� ������ � ����������� IP </param>
void network::sockInfo_t::setSockInfo(const sockInfo_t& sockInfo)
{
    memset(&Addr, 0, SizeAddr());
    memmove(&Addr, &sockInfo.Addr, SizeAddr());
    IP_port = sockInfo.IP_port;
}

/// <summary>
/// ����� ���������� ������������ ��������� �� ��������� ����������� �����
/// </summary>
/// <returns> ����������� ��������� �� ��������� ����������� ����� </returns>
const sockaddr* network::sockInfo_t::getSockAddr() const
{
    return &Addr;
}

/// <summary>
/// ����� �������� ������� ��������� ����������� �����
/// </summary>
/// <returns> ������ ��������� ����������� ����� </returns>
size_t network::sockInfo_t::SizeAddr() const
{
    return sizeAddr;
}

/// <summary>
/// ����� �������� IP
/// </summary>
/// <returns> IP ������ � ������� "����.����.����.����" </returns>
std::string network::sockInfo_t::GetIP() const
{
    return IP_port.first;
}

/// <summary>
/// ����� �������� ������ �����
/// </summary>
/// <returns> ����� ����� </returns>
unsigned short network::sockInfo_t::GetPort() const
{
    return IP_port.second;
}

/// <summary>
/// �������� ���������
/// </summary>
/// <param name="rValue"> - �������������� �������� </param>
/// <returns> 1 - ������� ����� </returns>
bool network::sockInfo_t::operator == (const sockInfo_t& rValue) const
{// ���������� ������ �������� sockaddr (����� ����������)
    return getSockAddr() == rValue.getSockAddr();
}

/// <summary>
/// �������� ���������
/// </summary>
/// <param name="rValue"> - �������������� �������� </param>
/// <returns> 1 - ������� �� ����� </returns>
bool network::sockInfo_t::operator != (const sockInfo_t& rValue) const
{
    return !(*this == rValue);
}

/// <summary>
/// ����� ������ ����������� ������ (��� ����������� ����������� ��������� ������� TCP)
/// </summary>
/// <param name="socket"> - ���������� ������ </param>
/// <param name="nonBlock"> - ���� �������������� ������ </param>
/// <returns> true - ��� ������� ������, false - ��� ��������� </returns>
bool network::socket_t::SetSocket(SOCKET socket, bool nonBlock)
{
    bool result = false;
    // ���� �������� ��������
    if (socket != INVALID_SOCKET)
        if (Close()) // ��������� ������ �����
        {
            Socket = socket;
            this->nonBlock = nonBlock; // accept ������ ��������� ����������� �����
            // ��������� ���������� � ������
            int sizeAddr = SizeAddr();
            if (!getsockname(Socket, setSockAddr(), &sizeAddr))
                UpdateSockInfo();// ����������� ����� setSockAddr()
            else
                logger.doLog("getsockname fail", GetError());

            result = true;
        }

    return result;
}

/// <summary>
/// ����� �������� ������
/// </summary>
/// <returns> true - ����� ������ </returns>
bool network::socket_t::Close()
{
    if (CheckValidSocket(false)) // ���� ����� ��������
        if (CLOSE_SOCKET(Socket)) // ���������
            logger.doLog("closesocket fail", GetError());
        else
        {
            Socket = INVALID_SOCKET; // �������� ����������
            UpdateSockInfo("", 0); // � ����������
        }

    return Socket == INVALID_SOCKET;
}

/// <summary>
/// ��������� ������� �����, �� �������� ��� ����� �������� ������������ ��������� ��
/// ������� �������������� ���������, � ����� ��������� ����, �� �������� ���
/// �������������� �������� ����� ���������������� �������-����������.
/// �������������� ��� ���������� � �������� ������ ��� ��������.
/// </summary>
/// <returns> true - �����, false - ������� </returns>
bool network::socket_t::Bind()
{
    bool result = false;//���������

    if (CheckValidSocket() && !IP_port.first.empty() && IP_port.second != 0) // ���� ������ ���������� � �������� ������
    {
        result = (bind(Socket, getSockAddr(), SizeAddr()) == 0); // ����������� ��� � IP � �����
        if (result) // ��������� ���������
            DEBUG_TRACE(logger, "bind -> ok " + IP_port.first + '.' + std::to_string(IP_port.second));
        else
            logger.doLog("bind -> fail " + IP_port.first + '.' + std::to_string(IP_port.second), GetError());
    }
    else // ���� ������� Bind() �� ��������� ���������� � �������� ������
        logger.doLog("bind invalid IP_port");

    return result;
}

/// <summary>
/// ��������� ������� �����, �� �������� ��� ����� �������� ������������ ��������� ��
/// ������� �������������� ���������, � ����� ��������� ����, �� �������� ���
/// �������������� �������� ����� ���������������� �������-����������.
/// </summary>
/// <param name="ip"> - IP ������ � ������� "����.����.����.����" </param>
/// <param name="port"> - ����� ����� </param>
/// <returns> 1 - ��������� </returns>
bool network::socket_t::Bind(std::string ip, unsigned short port)
{
    bool result = (setSockInfo(ip, port));
    result = result ? Bind() : false;
    return result;
}

/// <summary>
/// ����������� � ����� ����������, ��� �������� ������� ������� ��� ��������� ������
/// </summary>
/// <param name="logger"> - ������ ��� ������������ </param>
network::socket_t::socket_t(log_t& logger) : sockInfo_t(logger), Socket(INVALID_SOCKET), nonBlock(false)
{}

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
network::socket_t::socket_t(int af, int type, int protocol, log_t& logger) : socket_t(logger)
{
    Socket = socket(af, type, protocol); // ����������� ��������� ����� � �������� ����������� ������������ �������������� ����
    CheckValidSocket(); // ��������� ���������� ������
}

/// <summary>
///  ����������� � 6 �����������
/// </summary>
/// <param name="af"> - ��������� �������: ������ ����� �������� � ������� ���������� �������. �������� ������ ��������� � IPv4.
///  ����������� ��� AF_INET </param>
/// <param name="type"> - �� ������: ������ �������� ��� ������������� ��������� TCP (SOCK_STREAM) ��� UDP (SOCK_DGRAM).
///  �� ������ � ��� ���������� "�����" ������, ���������� ������� ��� ����������� ���������� � �������� �������������.
///  ��� ������������ SOCK_RAW </param>
/// <param name="protocol"> - ��� ���������: �������������� ��������, ���� ��� ������ ������ ��� TCP ��� UDP � ����� �������� �������� 0.
/// </param>
/// <param name="ip"> - IP ������ � ������� "����.����.����.����" </param>
/// <param name="port"> - ����� ����� </param>
/// <param name="logger"> - ������ ��� ������������ ������ </param>
network::socket_t::socket_t(int af, int type, int protocol, std::string ip, unsigned short port, log_t& logger) : socket_t(af, type, protocol, logger)
{
    Bind(ip, port);
}

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
network::socket_t::socket_t(int af, int type, int protocol, sockInfo_t sockInfo, log_t& logger) : socket_t(af, type, protocol, logger)
{
    setSockInfo(sockInfo);
    Bind();
}

/// <summary>
/// ����� �������� ���������� ������
/// </summary>
/// <param name="logOn"> - ���� �� ������� ������������ ������������ ������ </param>
/// <returns> true - ����� �������, false - ����� �� ������� </returns>
bool network::socket_t::CheckValidSocket(bool logOn)
{
    bool result = Socket != INVALID_SOCKET;

    if (!result && logOn) logger.doLog("Socket != INVALID_SOCKET", GetError());

    return result;
}

/// <summary>
/// ����������
/// </summary>
network::socket_t::~socket_t()
{
    Close();
}

/// <summary>
/// ����� �������� ����������� ������
/// </summary>
/// <returns> ���������� ������ </returns>
SOCKET network::socket_t::getSocket() const
{
    return Socket;
}

/// <summary>
/// ���������� ����� �������������
/// </summary>
/// <param name="NonBlock"></param>
/// <returns> 1 - ����� �� ����������� </returns>
bool network::socket_t::setNonBlock()
{
    if (!nonBlock && CheckValidSocket(false))
        nonBlock = RAII_OSsock::setSocketOpt(Socket, RAII_OSsock::option_t::NON_BLOCK, logger);
    return nonBlock;
}

/// <summary>
/// �������� �����, ������ ����� �������� ������� ������������ �������, �� ��������� ��� ���������� ������ ��� ac�ept()
/// </summary>
/// <param name="socket"> - ����� ���������� ������ </param>
/// <param name="sockInfo"> - ����������� ���������� </param>
/// <returns> true - �������� ������ </returns>
bool network::TCP_socketClient_t::SetSocket(SOCKET socket, sockInfo_t sockInfo)
{
    bool result = (b_connected = socket_t::SetSocket(socket, false));
    if (result) serverInfo.setSockInfo(sockInfo);
    return result;
}

/// <summary>
/// ����� �������� ����������� ������� (����������) ����� ��������
/// </summary>
/// <param name="source"> - ������ �� ����� �������� - ����� ������ ������ �� �������� �������� ����� </param>
void network::TCP_socketClient_t::Move(TCP_socketClient_t& source)
{
    if (Close() && socket_t::SetSocket(source.getSocket(), source.nonBlock))
    {
        serverInfo.setSockInfo(source.serverInfo);
        b_connected = source.b_connected;
        source.b_connected = false;
        source.Socket = INVALID_SOCKET;
        source.nonBlock = false;
        source.serverInfo.UpdateSockInfo("", 0);
        source.UpdateSockInfo("", 0);
    }
}

/// <summary>
/// ����������� � 1 ����������
/// </summary>
/// <param name="logger"> - ������ ��� ������������ </param>
network::TCP_socketClient_t::TCP_socketClient_t(log_t& logger) : socket_t(logger), b_connected(false), serverInfo(logger)
{}

/// <summary>
/// ����������� � 3 �����������
/// </summary>
/// <param name="ip_server"> - IP ����� ������� � ������� "����.����.����.����" </param>
/// <param name="port_server"> - ����� ����� ������� </param>
/// <param name="logger"> - ������ ������������ </param>
network::TCP_socketClient_t::TCP_socketClient_t(std::string ip_server, unsigned short port_server, log_t& logger) : socket_t(AF_INET, SOCK_STREAM, 0, logger), b_connected(false), serverInfo(logger)
{
    if (serverInfo.setSockInfo(ip_server, port_server)) // ���� ������� ������ ���������� � �������
        Connected(); // ������������� ��������� � ���
}

/// <summary>
/// ���������� � 2 �����������
/// </summary>
/// <param name="serverSockInfo"> - ���������� � ������� </param>
/// <param name="logger"> - ������ ������������ </param>
network::TCP_socketClient_t::TCP_socketClient_t(sockInfo_t serverSockInfo, log_t& logger) : socket_t(AF_INET, SOCK_STREAM, 0, logger), b_connected(false), serverInfo(logger)
{
    serverInfo.setSockInfo(serverSockInfo); // ������ ���������� � �������
    Connected(); // ������������� ����������
}

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
int network::TCP_socketClient_t::Recive(std::string& str_bufer, const std::string str_EndOfMessege, const size_t sizeMsg)
{
    int result = -1;
    // ���� ���� ����������
    if (b_connected && CheckValidSocket(false))
    {
        str_bufer.clear(); // ������� �������� ��������
        std::string tempStr(2048, '\0'); // ��������� ������ �������������� ������� ��� ������ ������
        int reciveSize = 0; // ������ �������� ������
        bool EOM = str_EndOfMessege.empty() && (sizeMsg == 0); // EndOfMessege ������� ����� ���������
        // ���� ������ ������
        do {
            reciveSize = recv(Socket, &tempStr[0], tempStr.size(), 0); // ������� ������ ��� ������ ������ �� ������.

            if (reciveSize > 0)
            {// ���� ������ ����
                DEBUG_TRACE(logger, "Recive msg: " + tempStr)
                    tempStr.assign(tempStr.c_str()); // ����������� �� ������ '\0';
                str_bufer += tempStr; // ��������� � �����
                tempStr.clear(); // ������ ������

                if (!str_EndOfMessege.empty())
                { // ���� ����� EOM
                    int pos = str_bufer.size() - str_EndOfMessege.size(); // ��������� ������� ��� � ������
                    if (pos >= 0) // ���� ��� �������
                        EOM = (str_bufer.rfind(str_EndOfMessege) == pos); // ������� ��� � ����� ������
                }
                if (sizeMsg != 0) // ���� ����� ������ ���������
                    EOM |= (str_bufer.size() >= sizeMsg); // ���������, �� ��� �� �� ��� ��������

                result = EOM ? 0 : reciveSize; // ���� ������� ��� ���������, �� 0, ���� �����, �� ���-�� ����
            }
            else if (reciveSize < 0)
            {   // ���� ����� �� �����������, ���������, ����� ������ ��� ������
                if (nonBlock && GetError() == error_t::NON_BLOCK_SOCKET_NOT_READY)
                    result = -3; // ����� �� �����������, ��� ������
                else
                {   // ���� ���� ������, ���������
                    logger.doLog("TCP_socketClient_t::Recive() fail, errno: ", GetError());
                    result = -1; // ��������� ������
                    b_connected = false; // � ��������� ����������
                }
                break;
            }
            else
            {
                result = -2; // ���������� �������
                b_connected = false;
                break;
            }

        } while (!EOM && !nonBlock); // ��������� ���� �� ����� ������, ���� �� ������������� �����, � ��������� ���� ���� ���
    }
    else
        result = -2; // ���������� �������

    return result;
}

/// <summary>
/// ����� �������� ��������� � ������������ ������ � ���������� �������� ������� ������������� ���������
/// </summary>
/// <param name="str_bufer"> - �����, ���������� ������ ��� �������� </param>
 /// <returns> 0 - ��������� ���������� ��������;
///           N>0 - ���������� N ����;
///           -1 - ��������� ������;
///           -2 - ���������� ������� ��� ���������� �����;
///           -3 - ����� �� ����� � �������� (������������� �����)</returns>
int network::TCP_socketClient_t::Send(const std::string& str_bufer)
{
    int result = -1;
    // ���� �� ����������
    if (b_connected && CheckValidSocket(false))
    {
        int totalSendSize = str_bufer.size(); // ��������� ���������� ������������ ����
        int sendSize = 0; // ������� ���������� ������������ ����
        // ���� ��������
        do {
            int tempSize = send(Socket, &str_bufer[sendSize], totalSendSize - sendSize, 0); // ������������ ��� ��������� ��������� � ������ �����
            if (tempSize > 0)
            { // ���� ��� �� ���������
                DEBUG_TRACE(logger, std::string(&str_bufer[sendSize], tempSize));
                sendSize += tempSize;
                result = (totalSendSize == sendSize) ? 0 : sendSize; // ��� �� ���������?
            }
            else if (tempSize < 0)
            {// ���� ����� �� �����������, ���������, ����� ������ ��� ������
                if (nonBlock && GetError() == error_t::NON_BLOCK_SOCKET_NOT_READY)
                    result = -3; // ����� �� �����������, ��� ������
                else // ���� ������, ��������� ������ � ��������� ����������, ������� �� �����
                {
                    logger.doLog("TCP_socketClient_t::Send() fail, errno: ", GetError());
                    result = -1; // ��������� ������
                    b_connected = false; // � ��������� ����������
                }
                break;
            }
            else
            {
                result = str_bufer.empty() ? 0 : -1; // �������� ��������� ������ ���������? � ��� ����������
                break;
            }
        } while ((totalSendSize > sendSize) && !nonBlock);// ��������� ���� �� ��� ������, ���� �� ������������� �����, � ��������� ���� ���� ���
    }
    else
        result = -2; // ���������� �������

    return result;
}

/// <summary>
/// ����� ����������� ������ � ���������� ������
/// </summary>
/// <returns> 1 - ���� ����������� </returns>
bool network::TCP_socketClient_t::Connected()
{
    // ���� ����� �������
    if (CheckValidSocket(false) && !b_connected)
    {   // ������� ��������� ������� �������������� ������������ - ��� ��������� TCP - ����� ������� ��������� ������ ����� SYN
        if (0 != connect(Socket, serverInfo.getSockAddr(), serverInfo.SizeAddr()))
            logger.doLog("TCP_socketClient_t non connected with server:", GetError());
        else
        {   // ���� �� ������� ������������, ��������� ���������� � ���� (��� ������ ���������)
            b_connected = true;
            UpdateSockInfo();
        }
    }

    return b_connected;
}
/// <summary>
/// ����� �������� ��������� ����������
/// </summary>
/// <returns> 1 - ���������� ���� </returns>
bool network::TCP_socketClient_t::GetConnected() const
{
    return b_connected;
}

/// <summary>
/// ����������� � 3-� �����������
/// </summary>
/// <param name="ip"> - IP ������ � ������� "����.����.����.����" </param>
/// <param name="port"> - ����� ����� </param>
/// <param name="logger"> - ������ ������������ </param>
network::TCP_socketServer_t::TCP_socketServer_t(std::string ip, unsigned short port, log_t& logger) : socket_t(AF_INET, SOCK_STREAM, 0, ip, port, logger)
{ //������� listen �������� ����� � ���������, � ������� �� ������������ �������� ����������
    if (CheckValidSocket(false))
        if (0 != listen(Socket, SOMAXCONN))
            this->logger.doLog("TCP_socketServer_t listen fali ", GetError());
}

/// <summary>
/// ����������� � 2-� �����������
/// </summary>
/// <param name="sockInfo"> - ���������� � ������ </param>
/// <param name="logger"> - ������ ������������ </param>
network::TCP_socketServer_t::TCP_socketServer_t(sockInfo_t sockInfo, log_t& logger) : socket_t(AF_INET, SOCK_STREAM, 0, sockInfo, logger)
{
    if (CheckValidSocket(false))
        if (0 != listen(Socket, SOMAXCONN))
            this->logger.doLog("TCP_socketServer_t listen fali ", GetError());
}

/// <summary>
/// ����� ���������� ������������ ��������
/// </summary>
/// <param name="client"> - ������ �� ������� ��� ������ �� ������������� ������� </param>
/// <returns> 0 - �������� ������������ ������,
///          -1 - ��������� ������,
///          -2 - ��� �������� � ������� �� ����������� (������������� �����)</returns>
///			 -3 - ���������� �������� (������������ �����) ��� ���������� ��������� �����
int network::TCP_socketServer_t::AddClient(TCP_socketClient_t& client)
{
    int result = -1;

    if (!client.CheckValidSocket(false) && CheckValidSocket(false))
    {
        sockInfo_t tempInfo(logger); // ���������� � ������������ ������
        int sizeAddr = tempInfo.SizeAddr(); // �� ������
        //������� ������������ �������� ��� �������� ����� �� �����. ����� ������ ���� ��� ��������� � ������ ������ �������.
        //���� ������ ������������� ����� � ��������, �� ������� accept ���������� ����� �����-����������, ����� �������
        //� ���������� ������� ������� � ��������.
        SOCKET tempSocket = accept(Socket, tempInfo.setSockAddr(), &sizeAddr);
        if (tempSocket != INVALID_SOCKET)
        { // ���� ���� ����� �� �����, �� ������������� ���
            tempInfo.UpdateSockInfo(); // ��������������������� ���������� � ������
            if (client.SetSocket(tempSocket, tempInfo))
            { // ��� ����������? ����� ������� � ����������
                DEBUG_TRACE(logger, "addClient success" + tempInfo.GetIP() + std::to_string(tempInfo.GetPort()))
                    result = 0;
            }
            else // ����� �������� � ��������� ������
                logger.doLog("fail SetSocket in addClient", GetError());
        } // ��� �� �� �������� ������� ����� � ���� �� ������������� ����� � ������ ������� � ����������� �������� � ������� �� �����������
        else if (GetError() == error_t::NON_BLOCK_SOCKET_NOT_READY && nonBlock)
            result = -2;
        else
            logger.doLog("accept fail", GetError()); // ����� ��� ��������� ������
    }
    else
        result = -3;

    return result;
}

/// <summary>
/// ����� �������� ����������� ������� (����������) ����� ��������
/// </summary>
/// <param name="source"> - ������ �� ����� �������� - ����� ������ ������ �� �������� �������� ����� </param>
void network::UDP_socket_t::Move(UDP_socket_t& source)
{
    if (Close() && socket_t::SetSocket(source.Socket, source.nonBlock))
    {
        lastCommunicationSocket.setSockInfo(source.lastCommunicationSocket);
        source.Socket = INVALID_SOCKET;
        source.nonBlock = false;
        source.lastCommunicationSocket.UpdateSockInfo("", 0);
        source.UpdateSockInfo("", 0);
    }
}

/// <summary>
/// ����� ��������� MTU
/// </summary>
void network::UDP_socket_t::setMTU()
{
    int optlen = sizeof(u32_MTU); // ������ �����
    //������� getsockopt ��������� ������� �������� ��� ��������� ������, ���������� � ������� ������ ����, � ����� ���������
    if (getsockopt(Socket, SOL_SOCKET, SO_MAX_MSG_SIZE, (char*)(&u32_MTU), &optlen))
        logger.doLog("getsockopt fail ", GetError());
}

/// <summary>
/// ����������� � 1 ����������
/// </summary>
/// <param name="logger"> - ������ ��� ������������ </param>
network::UDP_socket_t::UDP_socket_t(log_t& logger) : socket_t(AF_INET, SOCK_DGRAM, 0, logger), lastCommunicationSocket(logger)
{
    setMTU();
}

/// <summary>
/// ����������� � 3-� �����������
/// </summary>
/// <param name="ip"> - IP ������ � ������� "����.����.����.����" </param>
/// <param name="port"> - ����� ����� </param>
/// <param name="logger"> - ������ ������������ </param>
network::UDP_socket_t::UDP_socket_t(std::string ip, unsigned short port, log_t& logger) : socket_t(AF_INET, SOCK_DGRAM, 0, ip, port, logger), lastCommunicationSocket(logger)
{
    setMTU();
}

/// <summary>
/// ����������� � 2-� �����������
/// </summary>
/// <param name="sockInfo"> - ���������� � ������ </param>
/// <param name="logger"> - ������ ������������ </param>
network::UDP_socket_t::UDP_socket_t(sockInfo_t& sockInfo, log_t& logger) : socket_t(AF_INET, SOCK_DGRAM, 0, sockInfo, logger), lastCommunicationSocket(logger)
{
    setMTU();
}

/// <summary>
/// ����� �������� ������ � ���� ��� ���������������� ����������
/// </summary>
/// <param name="buffer"> - ������� � ������� ��� �������� </param>
/// <param name="target"> - ���������� � ������ ��������� </param>
/// <returns> 0 - ���������� ��� ���������;
///         N>0 - ���������� N ����;
///          -1 - ��������� ������;
///          -2 - ������ ��������� ������ MTU ��� ����� �� ��������
///          -3 - ����� �� ����� � �������� (������������� �����) </returns>
int network::UDP_socket_t::SendTo(const std::string& buffer, sockInfo_t target)
{
    int result = -1;
    // ��������� ������ ���������
    if (CheckValidSocket(false) && buffer.size() < MTU())
    { // ������� sendto ���������� ������ � ������������ ����� ����������
        int sendSize = sendto(Socket, buffer.c_str(), buffer.size(), 0, target.getSockAddr(), target.SizeAddr());
        // ��������� ���������
        if (sendSize > 0)
        { // ���� ���� ������������� ���������
            result = (sendSize == buffer.size()) ? 0 : sendSize; // ���� ��������� ����������� ���������, �� 0 - ��� ���, ���� ���, �� ���������� ��������� ����
            DEBUG_TRACE(logger, "sendto: " + buffer)
        }
        else if (sendSize < 0)
        { // ���� ���� ������, ���������, ������� �� ��� � ����������� ��� ������������� ������
            if (GetError() == error_t::NON_BLOCK_SOCKET_NOT_READY && nonBlock)
                result = -3; // ����� �� ����� (�������������)
            else
                logger.doLog("sendto fail ", GetError()); // ����� ��������� ������
        }
        else if (buffer.empty()) // ���� �������� ��������� ����? �� � ��� ��� ����������
            result = 0;
    }
    else
        result = -2; // ������ ��������� ������ MTU ��� ����� �� ��������

    // ���� ��������� ������ ������ ������, ��������� ���������� �� ���� (��� ��������� ��������� �����)
    if (target != lastCommunicationSocket)
        lastCommunicationSocket.setSockInfo(target);

    return result;
}

/// <summary>
/// ����� �������� ������ � ���� ��� ���������������� ����������, �������� ����������� ������ � ������� ���� ��������������
/// </summary>
/// <param name="buffer"> - ������� � ������� ��� �������� </param>
/// <returns> 0 - ���������� ��� ���������;
///         N>0 - ���������� N ����;
///          -1 - ��������� ������;
///          -2 - ������ ��������� ������ MTU ��� ����� �� ��������
///          -3 - ����� �� ����� � �������� (������������� �����) </returns>
int network::UDP_socket_t::SendTo(const std::string& buffer)
{
    return SendTo(buffer, lastCommunicationSocket); // �������� ����������� ������ � ������� ���� ��������������
}

/// <summary>
/// ����� �������� ������ � ���� ��� ���������������� ����������
/// </summary>
/// <param name="buffer"> - ������� � ������� ��� �������� </param>
/// <param name="ip"> - IP ������ � ������� "����.����.����.����" </param>
/// <param name="port"> - ����� ����� </param>
/// <returns> 0 - ���������� ��� ���������;
///         N>0 - ���������� N ����;
///          -1 - ��������� ������;
///          -2 - ������ ��������� ������ MTU ��� ����� �� ��������
///          -3 - ����� �� ����� � �������� (������������� �����) </returns>
int network::UDP_socket_t::SendTo(const std::string& buffer, std::string ip, unsigned short port)
{
    if (lastCommunicationSocket.setSockInfo(ip, port)) // ������ ���������� � ������, ���� ��� �������
        return SendTo(buffer); // ���������
    else return -1;
}

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
int network::UDP_socket_t::RecvFrom(std::string& buffer, const std::string str_EndOfMessege, const size_t sizeMsg)
{
    int result = -1;

    if (CheckValidSocket(false))
    {
        buffer.clear(); // ������� �����
        std::string tempStr(2048, '\0'); // ��������� ������ �������������� ������� ��� ������ ������
        int SizeAddr = lastCommunicationSocket.SizeAddr(); // ������ ��������� Addr
        // ������� recvfrom �������� ���������� � ��������� �������� �����
        int recvSize = recvfrom(Socket, &tempStr[0], tempStr.size(), 0, lastCommunicationSocket.setSockAddr(), &SizeAddr);

        if (recvSize > 0)
        { // ���� ��������� �����������
            buffer.assign(tempStr.c_str()); // ������� ������ '\0' � ����� ������
            DEBUG_TRACE(logger, "recvfrom: " + tempStr)

                bool EOM = str_EndOfMessege.empty() && (sizeMsg == 0);// EndOfMessege ������� ����� ���������
            if (!str_EndOfMessege.empty())
            { // ���� ����� EOM
                int pos = buffer.size() - str_EndOfMessege.size(); // ��������� ������� ��� � ������
                if (pos >= 0) // ���� ��� �������
                    EOM = (buffer.rfind(str_EndOfMessege) == pos); // ������� ��� � ����� ������
            }
            if (sizeMsg != 0) // ���� ����� ������ ���������
                EOM |= (buffer.size() >= sizeMsg); // ���������, �� ��� �� �� ��� ��������

            result = EOM ? 0 : recvSize; // ���� �������� ���, �� 0, ����� ���-�� ���������� ����

            lastCommunicationSocket.UpdateSockInfo(); // �������� ����� ���������� setSockAddr()
        }
        else if (recvSize < 0)
        { // ���� ��������� ������, ��������� �� ������� �� ��� � ����������� �������������� ������
            if (GetError() == error_t::NON_BLOCK_SOCKET_NOT_READY && nonBlock)
                result = -3;
            else
                logger.doLog("recfrom fail ", GetError());
        }
        else
            result = -2; // ���������� �������
    }
    else
        result = -2;

    return result;
}

/// <summary>
/// ����� �������� ���������� � ������ � ������� ����������� ��������� �������������� (��������/����� ������)
/// </summary>
/// <returns> ����� � ������� ����������� ��������� �������������� (��������/����� ������) </returns>
network::sockInfo_t network::UDP_socket_t::GetLastCommunication() const
{
    return lastCommunicationSocket;
}

/// <summary>
/// ����� �������� MTU
/// </summary>
/// <returns> ������������ ������ ��������� </returns>
unsigned int network::UDP_socket_t::MTU() const
{
    return u32_MTU;
}

/// <summary>
/// ����� ���������� ������ � ���� �� �������
/// </summary>
/// <param name="m_sock"> - ������������� ������ ��� ���������� </param>
/// <param name="socket"> - ����� �� ���������� </param>
/// <returns> 1 - ����� �������� </returns>
bool network::NonBlockSocket_manager_t::addSocket(std::map<int, std::shared_ptr<socket_t>>& m_sock, std::shared_ptr<socket_t> socket)
{
    bool result = false;

    if (socket->CheckValidSocket()) // ����� ��������?
        if (socket->setNonBlock()) // ���� ����� �������������
            if (result = m_sock.find(socket->getSocket()) == m_sock.end()) // � ��� ��� ��� � �������
            {
                m_sock[socket->getSocket()] = socket; // ��������� ���
                b_change = true; // ��������� ���������, ����� ������ pollfd
            }

    return result;
}
/// <summary>
/// ����� �������� ������ �� �������
/// </summary>
/// <param name="m_sock"> - ������������� ������ ��� ���������� </param>
/// <param name="socket"> - ����� �� �������� </param>
/// <returns> 1 - ����� ������ </returns>
bool network::NonBlockSocket_manager_t::deleteSocket(std::map<int, std::shared_ptr<socket_t>>& m_sock, std::shared_ptr<socket_t> socket)
{
    bool result = false;
    // ����� �� ������ ����� ����������� ����� ��������? �� ���� ���, ������� �� ���� �����, ����� ��� �������� � ������ ��������
    if (result = m_sock.find(socket->getSocket()) != m_sock.end()) // ���� ����� ����� ���� � ������
    {
        m_sock.erase(socket->getSocket()); // ������� ���
        b_change = true; // ��������� ���������, ����� ������ pollfd
    }

    return result;
}


/// <summary>
/// ����� ���������� ��������� pollfd
/// </summary>
void network::NonBlockSocket_manager_t::UpdatePollfd()
{
    if (b_change)
    { // ���� ���� ��������� � ������� �������
        b_change = false;
        size_t size = m_senderSocket.size() + m_readerSocket.size() + m_serverSocket.size() + m_clientSocket.size(); // ������ ������� �������� pollfd
        v_fds.clear(); // ������� ������ �������� pollfd
        v_fds.resize(size);

        size_t indx = 0; // ������ ��� ������ ������� �������� pollfd
        std::map<int, std::shared_ptr<socket_t>>::const_iterator iter_sender = m_senderSocket.begin(); // �������� ��� ������ ������������
        std::map<int, std::shared_ptr<socket_t>>::const_iterator iter_reader = m_readerSocket.begin(); // �������� ��� ������ ���������
        std::map<int, std::shared_ptr<socket_t>>::const_iterator iter_server = m_serverSocket.begin();
        std::map<int, std::shared_ptr<socket_t>>::const_iterator iter_client = m_clientSocket.begin();
        // ���� ��� ������ ������� �������� pollfd
        for (; indx < size; ++indx)
        {
            if (iter_sender != m_senderSocket.end()) // ������� ������������
            {
                v_fds[indx].fd = iter_sender->second->getSocket(); // ���������� ������
                v_fds[indx].events = POLLOUT; // ��������� �������
                v_fds[indx].revents = 0; // ������������ �������
                ++iter_sender;
            }
            else if (iter_reader != m_readerSocket.end()) // ������� ���������
            {
                v_fds[indx].fd = iter_reader->second->getSocket();
                v_fds[indx].events = POLLIN;
                v_fds[indx].revents = 0;
                ++iter_reader;
            }
            else if (iter_server != m_serverSocket.end())
            {
                v_fds[indx].fd = iter_sender->second->getSocket();
                v_fds[indx].events = POLLIN;
                v_fds[indx].revents = 0;
                ++iter_server;
            }
            else if (iter_server != m_serverSocket.end())
            {
                v_fds[indx].fd = iter_client->second->getSocket();
                v_fds[indx].events = POLLOUT;
                v_fds[indx].revents = 0;
                ++iter_client;
            }
            else break; // ���� ������ ���������, ������� (�� ������ ���������)
        }

    } // ���� ��������� �� ����, ������ �������� ������������ �������
    else
        for (size_t indx = 0; indx < v_fds.size(); ++indx)
            v_fds[indx].revents = 0;

}
/// <summary>
/// ������� ������������������ ������������� �������
/// </summary>
/// <param name="timeOut"> - ����� timeout ��� ������� ������������������� </param>
/// <returns> -1 - ��������� ������; 0 - ����� ������� � ������� �� ���������; N>0 - ���-�� ������� </returns>
int network::NonBlockSocket_manager_t::Poll(int timeOut)
{
#ifdef __WIN32__
    return WSAPoll(&v_fds[0], v_fds.size(), timeOut);//������� WSAPoll ���������� ��������� ������ � �������� revents ��������� WSAPOLLFD
#else
    return poll(&v_fds[0], v_fds.size(), timeOut);
#endif
}

/// <summary>
/// ����������� � ����� ����������
/// </summary>
/// <param name="logger"> - ������ ��� ������������ </param>
network::NonBlockSocket_manager_t::NonBlockSocket_manager_t(log_t& logger) : RAII_OSsock(logger), b_change(false), logger(logger)
{}
/// <summary>
/// ����������� � ����� �����������
/// </summary>
/// <param name="size"> - ��������������� ���������� ����������� ������� </param>
/// <param name="logger"> - ������ ��� ����������� </param>
network::NonBlockSocket_manager_t::NonBlockSocket_manager_t(int size, log_t& logger) : RAII_OSsock(logger), b_change(false), logger(logger)
{
    v_fds.reserve(size);
}
/// <summary>
/// ����� ���������� �����������
/// </summary>
/// <param name="socket"> - ������� ����� </param>
/// <returns> 1 - ����� �������� </returns>
bool network::NonBlockSocket_manager_t::AddSender(std::shared_ptr<socket_t> socket) // ��������� �����
{
    return addSocket(m_senderSocket, socket);
}
/// <summary>
/// ����� �������� �����������
/// </summary>
/// <param name="socket"> - ������� ����� </param>
/// <returns> 1 - ����� ������ </returns>
bool network::NonBlockSocket_manager_t::deleteSender(std::shared_ptr<socket_t> socket)
{
    return deleteSocket(m_senderSocket, socket);
}
/// <summary>
/// ����� ���������� ��������
/// </summary>
/// <param name="socket"> - ������� ����� </param>
/// <returns> 1 - ����� �������� </returns>
bool network::NonBlockSocket_manager_t::AddReader(std::shared_ptr<socket_t> socket)
{
    return addSocket(m_readerSocket, socket);
}
/// <summary>
/// ����� �������� ��������
/// </summary>
/// <param name="socket"> - ������� ����� </param>
/// <returns> 1 - ����� ������ </returns>
bool network::NonBlockSocket_manager_t::deleteReader(std::shared_ptr<socket_t> socket)
{
    return deleteSocket(m_readerSocket, socket);
}

/// <summary>
/// ����� ���������� �������
/// </summary>
/// <param name="socket"> - ������� ����� </param>
/// <returns> 1 - ����� �������� </returns>
bool network::NonBlockSocket_manager_t::AddServer(std::shared_ptr<socket_t> socket)
{
    return addSocket(m_serverSocket, socket);
}

/// <summary>
/// ����� �������� �������
/// </summary>
/// <param name="socket"> - ������� ����� </param>
/// <returns> 1 - ����� ������ </returns>
bool network::NonBlockSocket_manager_t::deleteServer(std::shared_ptr<socket_t> socket)
{
    return deleteSocket(m_serverSocket, socket);
}

/// <summary>
/// ����� ���������� �������
/// </summary>
/// <param name="socket"> - ������� ����� </param>
/// <returns> 1 - ����� �������� </returns>
bool network::NonBlockSocket_manager_t::AddClient(std::shared_ptr<socket_t> socket)
{
    return addSocket(m_clientSocket, socket);
}

/// <summary>
/// ����� �������� �������
/// </summary>
/// <param name="socket"> - ������� ����� </param>
/// <returns> 1 - ����� ������ </returns>
bool network::NonBlockSocket_manager_t::deleteClient(std::shared_ptr<socket_t> socket)
{
    return deleteSocket(m_clientSocket, socket);
}

/// <summary>
/// ����� �������� ���������� ����������� � ��������
/// </summary>
/// <param name="socket"> - ������� ����� </param>
/// <returns> 1 - ����� ����� </returns>
bool network::NonBlockSocket_manager_t::GetReadySender(std::shared_ptr<socket_t> socket)
{
    return m_readySender.find(socket->getSocket()) != m_readySender.end();
}

/// <summary>
/// ����� �������� ���������� �������� � ������
/// </summary>
/// <param name="socket"> - ������� ����� </param>
/// <returns> 1 - ����� ����� </returns>
bool network::NonBlockSocket_manager_t::GetReadyReader(std::shared_ptr<socket_t> socket)
{
    return m_readyReader.find(socket->getSocket()) != m_readyReader.end();
}

/// <summary>
/// ����� �������� ������� ������� �� ����������� � �������
/// </summary>
/// <param name="socket"> - ������� ����� </param>
/// <returns> 1 - ���� ����������� </returns>
bool network::NonBlockSocket_manager_t::GetReadyServer(std::shared_ptr<socket_t> socket)
{
    return m_readyServer.find(socket->getSocket()) != m_readyServer.end();
}

/// <summary>
/// ����� �������� ������������� ����������� � ������� � �������
/// </summary>
/// <param name="socket"> - ������� ����� </param>
/// <returns> 1 - ���� ����������� </returns>
bool network::NonBlockSocket_manager_t::GetReadyClient(std::shared_ptr<socket_t> socket)
{
    return m_readyClient.find(socket->getSocket()) != m_readyClient.end();
}


/// <summary>
/// �������� ����� ������ �������������
/// </summary>
/// <param name="timeOut"> - ����� �������� ������������� </param>
/// <returns> 1 - ������� ������ ���� ������� </returns>
bool network::NonBlockSocket_manager_t::Work(const int timeOut)
{// ������� ����� ������� �������
    m_readySender.clear();
    m_readyReader.clear();
    m_readyServer.clear();
    m_readyClient.clear();
    // ���������� ��������� pollfd
    UpdatePollfd();
    size_t size = v_fds.size();
    // �������� ������ �������������������
    int resPoll = Poll(timeOut);
    if (resPoll > 0) // ���� ��������� �����������
    {  // �� ���� ���������� pollfd
        for (size_t indx = 0; indx < size; ++indx)
            if (v_fds[indx].revents & POLLIN && m_readerSocket.find(v_fds[indx].fd) != m_readerSocket.end()) // ����� ������� �� ��������
                m_readyReader[v_fds[indx].fd] = m_readerSocket[v_fds[indx].fd];
            else if (v_fds[indx].revents & POLLOUT && m_senderSocket.find(v_fds[indx].fd) != m_senderSocket.end()) // ����� ������� �� �����������
                m_readySender[v_fds[indx].fd] = m_senderSocket[v_fds[indx].fd];
            else if (v_fds[indx].revents & POLLIN && m_serverSocket.find(v_fds[indx].fd) != m_serverSocket.end()) // ����� ������� �� ������
                m_readyServer[v_fds[indx].fd] = m_serverSocket[v_fds[indx].fd];
            else if (v_fds[indx].revents & POLLOUT && m_clientSocket.find(v_fds[indx].fd) != m_clientSocket.end()) // ����� ������� �� �������
                m_readyClient[v_fds[indx].fd] = m_clientSocket[v_fds[indx].fd];
    }
    else if (resPoll < 0) // ��������� ������
        logger.doLog("poll error", GetError());

    return !m_readySender.empty() || !m_readyReader.empty() || !m_readyServer.empty() || !m_readyClient.empty(); // ���� ��� �� �������� � ������?
}

