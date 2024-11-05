#include "log.h"
#include <chrono>
#include <sstream>
#include <iomanip>

/// <summary>
/// ����������� �� ���������
/// ���� �� ������ ��� ����� ������������, ������� ������ � �������
/// </summary>
log_t::log_t() : consoleActive(true), lastErr(0)
{
    time_zone = 3; // TO_DO
}
/// <summary>
/// ����������� � 3-� �����������
/// </summary>
/// <param name="nameLogFile"> - ��� ����� ������������ </param>
/// <param name="consoleActive"> - ���� �� ����� � ������� </param>
log_t::log_t(std::string nameLogFile, bool consoleActive) : consoleActive(consoleActive), lastErr(0)
{
    time_zone = 3; // TO_DO
    logFile.open(nameLogFile.c_str(), std::ios::app); // ��������� ���� ������������ ��� ��������
    if (!logFile)
    {
        if (consoleActive) std::cout << "logFile.open fail";
        else std::cerr << "logFile.open fail";//TODO check
    }
}

log_t::~log_t()
{
    if (logFile.is_open()) // ���� ���� ������ - ���������
        logFile.close();
}
/// <summary>
/// ����� ��� ������ � ���
/// </summary>
/// <param name="log"> - ������ ���� </param>
/// <param name="errCode"> - ��� ������ (�����������) </param>
void log_t::doLog(std::string log, int errCode)
{
    // ������� �������� ��������� ����
    std::string msg /*= getTime()*/;
    /*msg.append(" :: "); */
    msg.append(log);
    // ���� ���� ��� ������, ��������� ���
    if (errCode != 0x80000000)
    {
        lastErr = errCode; // ���������� �������� ������
        msg.append(" errno: ");
        msg.append(std::to_string(errCode));
    }
    // ����� � �������
    if (consoleActive) printf("%s\n", msg.c_str());
    // ����� � ����
    if (logFile.is_open()) logFile << msg << '\n';
}
#ifdef DEBUG
/// <summary>
/// ����� ��� ������ � ��� ������ � ������ �������
/// </summary>
/// <param name="log"> - ������ ���� </param>
void log_t::doDebugTrace(std::string trace)
{
    // ������� �������� ��������� ������
    std::string msg = getTime();
    msg.append(" :: ");
    msg.append(trace);
    // ����� � �������
    if (consoleActive) std::cout << trace << '\n';
    // ����� � ����
    if (logFile.is_open()) logFile << trace << '\n';
}
#endif
/// <summary>
/// ����� �������� ���� ��������� ������, ��������� ����� ����� doLog()
/// </summary>
/// <returns> ��� ��������� ������, ��������� ����� ����� doLog() </returns>
int log_t::GetLastErr() const
{
    return lastErr;
}
/// <summary>
/// ����� ������ �������
/// </summary>
/// <returns> ������ ������� "����.��.��->��:��:��"</returns>
std::string log_t::getTime()
{
    // �++17 �������
    std::stringstream result; // ���������

    bool leap = false; // ���� ����������� ����

    auto now = std::chrono::system_clock::now().time_since_epoch();// 1970 �������
    auto msec = std::chrono::duration_cast<std::chrono::milliseconds> (now); // ������������
    auto sec = std::chrono::duration_cast<std::chrono::seconds> (now); // �������
    auto min = std::chrono::duration_cast<std::chrono::minutes> (now); // ������
    auto tempHour = std::chrono::duration_cast<std::chrono::hours> (now); // ����
    // ������ �++17 ��� �� ����, ������ ������ ����(�� ��������� � �++20) 
    int hour = tempHour.count() + time_zone; // +3 ���������� ������� ���� 

    int totalDay = hour / 24; // ����� ���������� ����
    std::string weekDays; // ���� ������

    switch ((totalDay + 3) % 7) // �������� � �������� 1970 ����
    {
    case 0:
        weekDays = "monday";
        break;
    case 1:
        weekDays = "tuesday";
        break;
    case 2:
        weekDays = "wednesday";
        break;
    case 3:
        weekDays = "thursday";
        break;
    case 4:
        weekDays = "friday";
        break;
    case 5:
        weekDays = "saturday";
        break;
    case 6:
        weekDays = "sunday";
        break;
    default:
        break;
    }

    int year = 1969; // ����� ���������� ���
    int day = 0; // ���

    while (totalDay > 0) // ���� ����� ���������� ���� ��� ����
    {
        ++year; // ���� ���
        leap = year % 4 == 0; // ��� ��� ����������?
        day = totalDay; // �������������� ������������� ����
        totalDay -= leap ? 366 : 365; // �������� ��� ���� �� ������ ���������� 
    }

    int mounth = 0; // ����� ������������� �  ����������� �� ���������� ���� � ���� ����
    if (day < 31) mounth = 1;
    else if (leap ? day < 59 : day < 60) { mounth = 2; day -= 31; }
    else if (leap ? day < 90 : day < 91) { mounth = 3; day -= leap ? 59 : 60; }
    else if (leap ? day < 120 : day < 121) { mounth = 4; day -= leap ? 90 : 91; }
    else if (leap ? day < 151 : day < 152) { mounth = 5; day -= leap ? 120 : 121; }
    else if (leap ? day < 181 : day < 182) { mounth = 6; day -= leap ? 151 : 152; }
    else if (leap ? day < 212 : day < 213) { mounth = 7; day -= leap ? 181 : 182; }
    else if (leap ? day < 243 : day < 244) { mounth = 8; day -= leap ? 212 : 213; }
    else if (leap ? day < 273 : day < 274) { mounth = 9; day -= leap ? 243 : 244; }
    else if (leap ? day < 304 : day < 305) { mounth = 10; day -= leap ? 273 : 274; }
    else if (leap ? day < 334 : day < 335) { mounth = 11; day -= leap ? 304 : 305; }
    else if (leap ? day < 365 : day < 366) { mounth = 12; day -= leap ? 334 : 335; }
    // ������� ��������� ������� "����.��.��-���� ������-��:��:��:����"
    /*result << std::setfill('0') << year << '.' << std::setw(2) << mounth << '.' << std::setw(2) << day << '-' << weekDays << '-' //����
        << std::setw(2) << hour % 24 << ':' << std::setw(2) << min.count() % 60 << ':' << std::setw(2) << sec.count() % 60 //�����
        << '.' << std::setw(3) << msec.count() % 1000;//�����������

     return std::string(result.str());*/

    
    // ���������� ��� ��������� ������� ��� �������� "PBF group" ������� ������ ������
    result << '[' << std::setfill('0') << year << '-' << std::setw(2) << mounth << '-' << std::setw(2) << day << /*'-' << weekDays << '-'*/' ' //����
        << std::setw(2) << hour % 24 << ':' << std::setw(2) << min.count() % 60 << ':' << std::setw(2) << sec.count() % 60 //�����
        << '.' << std::setw(3) << msec.count() % 1000 << ']';//�����������

    return std::string(result.str());

    // �++03 - ������� (����������, �� ������������)
    /*char result[64]; // ���������
    std::memset(&result, 0, sizeof(result));
    time_t t1 = time(NULL);//������������ ������� ����� � ������� time_t � ���������� ������, ��������� � 00:00 1 ������ 1970.
    tm t = *localtime(&t1);//������� localtime() ��������� ��������� time_t � ��������� tm, ������� ������� �� �����, �������������� �������� ����, ������, �����, ��� � �. �.
    std::sprintf(result ,"%.4d.%.2d.%.2d.->%.2d:%.2d:%.2d", t.tm_year + 1900, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    return std::string(result);*/
}

