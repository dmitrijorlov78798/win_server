#include "poolThread.h"

/// <summary>
/// ����������� �� ���������
/// </summary>
task_t::task_t(std::condition_variable& cv_wakeUp, std::mutex& mtx_wakeUp) : cv_wakeUp(cv_wakeUp), mtx_wakeUp(mtx_wakeUp), b_stop(false), b_active(false), p_userTask(nullptr)
{}

/// <summary>
/// ����� �������� ������. ����������� � ������ ������
/// </summary>
void task_t::Work()
{
	bool wakeUp = false; // ������� �� ���������� �������� ��������� � ���������� �����

	while (!b_stop)
	{
		std::unique_lock<std::mutex> lock(mutex);
		if (!b_stop)
			cv_condition.wait(lock, [this]() { return b_active; });// ������� ���������� ������

		if (!b_stop && p_userTask != nullptr/* && b_active */) // ���� ��� ��������� �������
		{
			p_userTask->Work(b_stop); // ��������� ���������������� �����
			p_userTask = nullptr; // �������� ���������� �����
			b_active = false; // ����� ���������������� �����, ��������
			wakeUp = true; // ����� 
		}
		lock.unlock(); // ��������� �������� �������
		// ���������� ��������� � ���������� ������
		if (wakeUp)
		{
			wakeUp = false;
			std::lock_guard<std::mutex> lock(mtx_wakeUp);
			cv_wakeUp.notify_one();
		}
	}
}

/// <summary>
/// ����� ���������� �����
/// </summary>
/// <param name="userTask"> - ��������� �� ������ ���������������� ����� </param>
/// <returns> 1 - ����� ��������� </returns>
bool task_t::UpdateTask(std::shared_ptr<ABStask> userTask)
{
	bool result = false;

	if (mutex.try_lock()) // ������� ��������� �������
	{ // ����� ������ � ������ ���������� ������
		if (!b_active && p_userTask == nullptr)
		{ // ������������� �������� ���������� 
			p_userTask = userTask; // ��������� ������
			b_active = true;
			cv_condition.notify_one();
			result = true;
		}
		mutex.unlock();
	}

	return result;
}

/// <summary>
/// ����� �������� ����������
/// </summary>
/// <returns> 1 - ����� ������� </returns>
bool task_t::GetActive() const
{
	bool result = true;

	if (mutex.try_lock())
	{ // ������� ��������� �������
		result = b_active;
		mutex.unlock();
	}
	// ���� ������� �������� �� ���, �� ������ �������
	return result;
}

/// <summary>
/// ����� �������������� ������ �� �����
/// </summary>
void task_t::Stop()
{
	b_stop = true; // �������� ���� �� ���������

	if (mutex.try_lock())
	{ // ���� ��������� �������, ������ ������ ���� �� �������
		if (!b_active)
		{ // �������������
			b_active = true; // ���������
			cv_condition.notify_one();
		}
		mutex.unlock();
	}
}

/// <summary>
/// ����������
/// </summary>
task_t::~task_t()
{}

/// <summary>
/// ����������� �� ���������
/// </summary>
thread_t::thread_t(std::condition_variable& cv_wakeUp, std::mutex& mtx_wakeUp) : thread([this](){ task.Work(); }), task(cv_wakeUp, mtx_wakeUp)
{}

///����������
thread_t::~thread_t()
{
	task.Stop(); // �������� �����
	thread.join(); // ���� ��������� ������
}

/// <summary>
/// �����������
/// </summary>
/// <param name="cv_wakeUp"> - �������� ���������� �������� ��������� - ������ � ���������� ������ </param>
/// <param name="mtx_wakeUp"> - ���������� ������ ���������� </param>
poolThread_t::poolThread_t(std::condition_variable& cv_wakeUp, std::mutex& mtx_wakeUp, const size_t SIZE) : SIZE(SIZE), free(false)
{
	v_thread.reserve(SIZE);
	v_taskID.reserve(SIZE);
	for (size_t index = 0; index < SIZE; ++index)
	{   // ������������ ��������� ������, ��� ���������� ������ �������
		v_thread.push_back(std::make_shared<thread_t>(cv_wakeUp, mtx_wakeUp));
		v_taskID.push_back(0);
	}
}

/// <summary>
/// ���� ���������� �����
/// </summary>
/// <param name="userTask"> - ��������� �� ������ ���������������� ����� </param>
/// <returns> INVALID_TASK - ��� ��������� �������; ID>INVALID_TASK - ID ����� </returns>
bool poolThread_t::UpdateTask(std::shared_ptr<ABStask> userTask, taskID ID)
{
	for (size_t index = 0; index < SIZE; ++index) // ���� ��������� �����
		if (!v_thread[index]->task.GetActive())
			if (v_thread[index]->task.UpdateTask(userTask)) // ������� ������ �
			{
				v_taskID[index] = ID; // ��������� ID
				free = false;
				return true;
			}

	return free = false;
}

/// <summary>
/// ����� �������� ���������� �����
/// </summary>
/// <param name="ID"> - ID ������� ����� </param>
/// <returns> 1 - ����� ������� </returns>
bool poolThread_t::GetActiveTask(taskID ID)
{
	for (size_t index = 0; index < SIZE; ++index)
		if (v_taskID[index] == ID)
			return v_thread[index]->task.GetActive();

	return false;
}

/// <summary>
/// ����� ������ ������� ���������� ������
/// </summary>
/// <returns> 1 - ���� ���� �� ���� ��������� ����� </returns>
bool poolThread_t::GetFree() const
{
	bool result = free;
	if (!result)
		for (size_t index = 0; index < SIZE; ++index) // ���� ��������� �����
			if (!v_thread[index]->task.GetActive())
				free = result = true;

	return result;
}

/// <summary>
/// ����� ������ ������������ ������
/// </summary>
void poolThread_manager_t::Update()
{
	while (!stop)
	{
		std::unique_lock<std::mutex> lock(mutex);
		if (!stop) // ������� ������ �� ���������� ������ / ������������ ����� � ����
			cv_condition.wait(lock, [this]() { return stop || slave_pool.GetFree(); });
		if (!stop)
		{
			std::map<taskID, status_p_task_t>::iterator iter = m_task.begin();
			std::map<taskID, status_p_task_t>::iterator end = m_task.end();
			while (iter != end) // �������� �� ����� ���� �� ����������� taskID ��� ���������� ����������� ����������� �����
				if (iter->second.status == EXCEPTION && slave_pool.UpdateTask(iter->second.p_task, iter->first))
				{ // ���� ������ � ������� � ��� ������� ����� � ��� - ��� ��� �������
					iter->second.status = ACTIVE;
					++iter;
				}
				else if (iter->second.status == ACTIVE && !slave_pool.GetActiveTask(iter->first))
				{ // ���� ������ �������� ��������, � ��� ������� �� �������� - ������ ��������� 
					iter = m_task.erase(iter);
				}
				else // ���� �� �������� ��������� ���� 
					++iter;
		}
	}
}

/// <summary>
/// ����������� �� ���������
/// </summary>
poolThread_manager_t::poolThread_manager_t(const size_t SIZE) : slave_pool(cv_condition, mutex, SIZE), counter(0), master_thread([this]() { Update(); }), stop(false)
{}

/// <summary>
/// ����������
/// </summary>
poolThread_manager_t::~poolThread_manager_t()
{   // ����������� �� ��������� ������������ ������
	stop = true;
	mutex.lock(); // ����� ���, ���� �� ����
	cv_condition.notify_one();
	mutex.unlock();

	master_thread.join(); // ���� ��������� ������
}

/// <summary>
/// ����� ���������� ����� ������
/// </summary>
/// <param name="p_task"> - smart_ptr �� ���������������� ������ </param>
/// <returns> ����������� ���������������� ������ ���������� ����� (����������) </returns>
taskID poolThread_manager_t::AddTask(std::shared_ptr<ABStask> p_task)
{
	taskID result = ++counter;
	// ��������� � ��� ����� ������
	std::lock_guard<std::mutex> lock(mutex);
	m_task[result].p_task = p_task;
	m_task[result].status = EXCEPTION;
	cv_condition.notify_one(); // ����� ����������� �����

	return result;
}

/// <summary>
/// ����� ��������� ������� ������
/// </summary>
/// <param name="ID"></param>
/// <returns> NON_DEFINE (0) - �� ����������(���������� taskID)
///           ACTIVE (1) - ������ � �������� ���������� 
///           COMPLECTED (2) - ������ ���������
///           EXCEPTION (3) - ������ � ������� �� ����������  </returns>
int poolThread_manager_t::GetStatusTask(taskID ID)
{
	int result = NON_DEFINE;

	if (ID < counter)
	{   // ���� �������� ��������
		std::lock_guard<std::mutex> lock(mutex);
		if (m_task.find(ID) != m_task.end()) // ���� ���
			result = m_task[ID].status;
		else // ���� �� ����� - ������ ���� ���������
			result = COMPLECTED;
	}

	return result;
}
