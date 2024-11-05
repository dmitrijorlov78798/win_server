#pragma once
#ifndef POOLTHREAD_H_
#define POOLTHREAD_H_

#include <thread>
#include <atomic>
#include <map>
#include <mutex>
#include <vector>
#include <memory>

typedef unsigned long long taskID; // ����� ������

/// <summary>
/// ����������� ����� ���������������� ������
/// </summary>
class ABStask
{
public:
    /// <summary>
    /// �������� ����� ������
    /// </summary>
    /// <param name="stop"> - ��� ��������� ������� ��������, ���������� ��������� ������ ������ </param>
    virtual void Work(const volatile std::atomic_bool& stop) = 0;
    virtual ~ABStask() {}
};

/// <summary>
/// ����� ������� ���������������� ������
/// </summary>
class task_t
{
    friend struct thread_t;
    friend class poolThread_t;
protected :
    /// <summary>
    /// �����������
    /// </summary>
    /// <param name="cv_wakeUp"> - �������� ���������� ���������� �������� ��������� � ���������� ���������������� ������ </param>
    /// <param name="mtx_wakeUp"> - ������� � �������� ���������� </param>
    task_t(std::condition_variable& cv_wakeUp, std::mutex& mtx_wakeUp);

    /// <summary>
    /// ����� �������� ������. ����������� � ������ ������
    /// </summary>
    void Work();

    /// <summary>
    /// ����� ���������� ������
    /// </summary>
    /// <param name="userTask"> - ��������� �� ������ ���������������� ����� </param>
    /// <returns> 1 - ����� ��������� </returns>
    bool UpdateTask(std::shared_ptr<ABStask> userTask);

    /// <summary>
    /// ����� �������� ����������
    /// </summary>
    /// <returns> 1 - ����� ������� </returns>
    bool GetActive() const;

    /// <summary>
    /// ����� �������������� ������ �� �����
    /// </summary>
    void Stop();

    virtual ~task_t();
protected:
    std::condition_variable cv_condition; // �������� ���������� ��� ������ ������ ����������� ���������������� ������
    mutable std::mutex mutex;// ������� ��� �������� ���������� 
    std::condition_variable& cv_wakeUp; //  �������� ���������� ���������� �������� ��������� � ���������� ���������������� ������
    std::mutex& mtx_wakeUp; // ������� ��� �������� ���������� �������� ���������
    volatile std::atomic_bool b_stop; // ���� �������� ������ �����
    volatile std::atomic_bool b_active; // ���� ���������� ������ �����

    std::shared_ptr<ABStask> p_userTask; // ��������� �� ���������������� ������
};

/// <summary>
/// ��������� ����������� �������������� ������ � ������
/// </summary>
struct thread_t
{
    friend class poolThread_t;
 
    /// <summary>
    /// ����������� �� ���������
    /// </summary>
    thread_t(std::condition_variable& cv_wakeUp, std::mutex& mtx_wakeUp);
    ///����������
    virtual ~thread_t();

protected :
    std::thread thread; // ����������� �����
    task_t task; // �����
};


/// <summary>
/// ����� ���� �������
/// </summary>
class poolThread_t
{
    friend class poolThread_manager_t;
protected :
    /// <summary>
    /// �����������
    /// </summary>
    /// <param name="cv_wakeUp"> - �������� ���������� �������� ��������� - ������ � ���������� ������ </param>
    /// <param name="mtx_wakeUp"> - ���������� ������ ���������� </param>
    poolThread_t(std::condition_variable& cv_wakeUp, std::mutex& mtx_wakeUp, const size_t SIZE); 

    virtual ~poolThread_t()
    {}

    /// <summary>
    /// ���� ���������� �����
    /// </summary>
    /// <param name="userTask"> - ��������� �� ������ ���������������� ����� </param>
    /// <returns> INVALID_TASK - ��� ��������� �������; ID>INVALID_TASK - ID ����� </returns>
    bool UpdateTask(std::shared_ptr<ABStask> userTask, taskID ID);
    
    /// <summary>
    /// ����� �������� ���������� �����
    /// </summary>
    /// <param name="ID"> - ID ������� ����� </param>
    /// <returns> 1 - ����� ������� </returns>
    bool GetActiveTask(taskID ID);

    /// <summary>
    /// ����� ������ ������� ���������� ������
    /// </summary>
    /// <returns> 1 - ���� ���� �� ���� ��������� ����� </returns>
    bool GetFree() const;

protected:
    std::vector<std::shared_ptr<thread_t>> v_thread; // ������ �������
    std::vector<taskID> v_taskID; // ������ ID ������
    const size_t SIZE; // ������ ����
    mutable bool free; // ���� ������� ���������� ������
};

// ������� ��������� � ��� ������
#define NON_DEFINE 0 // �� ����������(���������� taskID)
#define ACTIVE 1 // ������ � �������� ���������� 
#define COMPLECTED 2 // ������ ���������
#define EXCEPTION 3 // ������ � ������� �� ���������� 

/// <summary>
/// ��������������� ��������� ������� �� ���������������� �������
/// </summary>
struct status_p_task_t
{
    friend class poolThread_manager_t;
    status_p_task_t() : status(0), p_task(nullptr)
    {}
protected :
    char status; // ������ ������
    std::shared_ptr<ABStask> p_task; // ��������� �� ������
};

/// <summary>
/// �������� ���� �������, ��������� ���������������� ������ � �������� �� � ���
/// </summary>
class poolThread_manager_t
{
protected:
    
    /// <summary>
    /// ����� ������ ������������ ������
    /// </summary>
    void Update();
   
public:
    /// <summary>
    /// ����������� �� ���������
    /// </summary>
    poolThread_manager_t(const size_t SIZE);

    /// <summary>
    /// ����������
    /// </summary>
    virtual ~poolThread_manager_t();

    /// <summary>
    /// ����� ���������� ����� ������
    /// </summary>
    /// <param name="p_task"> - smart_ptr �� ���������������� ������ </param>
    /// <returns> ����������� ���������������� ������ ���������� ����� (����������) </returns>
    taskID AddTask(std::shared_ptr<ABStask> p_task);

    /// <summary>
    /// ����� ��������� ������� ������
    /// </summary>
    /// <param name="ID"></param>
    /// <returns> NON_DEFINE (0) - �� ����������(���������� taskID)
    ///           ACTIVE (1) - ������ � �������� ���������� 
    ///           COMPLECTED (2) - ������ ���������
    ///           EXCEPTION (3) - ������ � ������� �� ����������  </returns>
    int GetStatusTask(taskID ID);

protected :
    poolThread_t slave_pool; // ����������� ��� �������
    taskID counter; // �������������
    std::map<taskID, status_p_task_t> m_task; // ��� ���������������� ����������� ����� � ����� � ������� �� ����������
    std::thread master_thread; // ����������� �����, �� ���� �������������, ������������ ������ � ���
    std::condition_variable cv_condition; // �������� ����������, ��������� �� ���������� ����� �� �����
    mutable std::mutex mutex;// ������� - ������ �������������� ������
    volatile std::atomic_bool stop; // ���� ��������� ���� ���������
};



#endif /* POOLTHREAD_H_ */

