#pragma once
#ifndef LOG_T_H_
#define LOG_T_H_

#include <iostream>
#include <fstream>
#include <string>

#ifdef DEBUG
#define DEBUG_TRACE(logger, string) logger.doDebugTrace(string)
#else
#define DEBUG_TRACE(logger, string)
#endif

/// <summary>
/// ����� ��� ������������ ������� ����� ���� �/��� �������
/// </summary>
class log_t
{
public:
    log_t();
    log_t(std::string nameLogFile, bool consoleActive);
    std::string getTime();
    void doLog(std::string log, int errCode = 0x80000000);
#ifdef DEBUG
    void doDebugTrace(std::string trace);
#endif
    int GetLastErr() const;
    virtual ~log_t();
protected:
    std::ofstream logFile; // ���� ��� ������������
    bool consoleActive; // ���� ������ � �������
    int time_zone; // ������� ����
    int lastErr; // ��� ��������� ������
};

#endif // !LOG_T
