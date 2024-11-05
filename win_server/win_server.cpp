// win_server.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#include "network.h"
#include "poolThread.h"

#include <list>
#include <string>
#include <mutex>
#include <memory>

#define IP_ADRES "127.0.0.1"

/// <summary>
/// класс реализация задачи приема и записи сообщения от одного клиента
/// </summary>
class taskOutPutMsg_t : public ABStask
{
private:
	log_t& r_logger; // ссылка на логгер для записи буфера в файл
	std::mutex& r_mutex; // ссылка на мьютекс для блокирования записи в файл 
	std::string s_bufer; // строковый буфер для приема сообщения от клиента
	network::TCP_socketClient_t h_client; // сокет для общения с клинетом
public:
	/// <summary>
	/// конструктор, для создания задачи
	/// </summary>
	/// <param name="logger"> - ссылка на логгер - ресурс для параллельного доступа к записи в файл </param>
	/// <param name="mutex"> - ссылка на мьютекс - защита от одновременного доступа к записи в файл </param>
	/// <param name="r_sock"> - ссылка на промежуточный сокет, полученный с помощью функции accept() </param>
	taskOutPutMsg_t(log_t& logger, std::mutex& mutex, network::TCP_socketClient_t& r_sock) : r_logger(logger), r_mutex(mutex), s_bufer(""), h_client(logger)
	{
		h_client.Move(r_sock);
	}

	/// <summary>
	/// основной метод работы задачи
	/// </summary>
	/// <param name="stop"> - флаг останова цикла, передается от пула потоков (здесь не используется, т.к. нет цикла) </param>
	void Work(const volatile std::atomic_bool& stop) override
	{
		if (0 == h_client.Recive(s_bufer))
		{// если приняли сообщение
			std::lock_guard<std::mutex> lock(r_mutex);
			r_logger.doLog(s_bufer); // записываем его с защитой от одновременно доступа
		}
	}
};

/// <summary>
/// функция разобра параметров командной строки
/// </summary>
/// <param name="argc"> - количество параметров </param>
/// <param name="argv"> - массив параметров </param>
/// <param name="r_port"> - ссылка на порт для прослушки </param>
/// <returns> 1 - праметры распознаны </returns>
bool parseParam(int argc, char* argv[], unsigned& r_port);


int main(int argc, char* argv[])
{
	printf("run_server\n");
	unsigned u32_port = 0;

	if (parseParam(argc, argv, u32_port))
	{
		log_t h_logger("log.txt", false); // объект для записи принятых сообщений в файл
		network::TCP_socketServer_t h_server(IP_ADRES, u32_port, h_logger); // сокет для работы сервера
		std::mutex h_mutex;
		poolThread_manager_t h_pool(3); // пул потоков для обработки клиентских соединений
		network::TCP_socketClient_t h_tempSock(h_logger); // промежуточный сокет для создания соединения с клиентом

		while (0 == h_server.AddClient(h_tempSock)) // если получилось получить нового клиента
			h_pool.AddTask(std::make_shared<taskOutPutMsg_t>(h_logger, h_mutex, h_tempSock)); // формируем задачу для обработки этого соединения
	}
	else
		printf("Invalid parametr's. Please enter the number_port\n");

	return EXIT_SUCCESS;
}

/// <summary>
/// функция разобра параметров командной строки
/// </summary>
/// <param name="argc"> - количество параметров </param>
/// <param name="argv"> - массив параметров </param>
/// <param name="r_port"> - ссылка на порт для прослушки </param>
/// <returns> 1 - праметры распознаны </returns>
bool parseParam(int argc, char* argv[], unsigned& r_port)
{
	bool b_result = false;

	if (argc == 2)
	{
		r_port = std::strtoul(argv[1], NULL, 10);
		b_result = r_port != 0 && r_port != ULONG_MAX;
	}

	return b_result;
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
