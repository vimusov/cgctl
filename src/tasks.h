#ifndef SRC_TASKS_H_
#define SRC_TASKS_H_

#include <stdbool.h>

/*
 * \fn void kill_all_tasks(const char *const dir_path)
 * \brief Прибивает все дочерние процессы, оставшиеся после завершения главного процесса.
 * \param const char *const dir_path: Путь к каталогу cgroup.
 * \warning Функция должна вызываться только когда cgroup "заморожен".
 */
void kill_all_tasks(const char *const dir_path);

/*
 * \fn void save_pid2tasks(const char *const dir_path)
 * \brief Добавляет текущий процесс в созданный cgroup.
 * \param const char *const dir_path: Путь к каталогу cgroup.
 */
void save_pid2tasks(const char *const dir_path);

/*
 * \fn bool are_alive_tasks_exist(const char *const dir_path)
 * \brief Проверяет, есть ли в заданном cgroup хотя бы один процесс.
 * \param const char *const dir_path: Путь к каталогу cgroup.
 * \return true - если в cgroup есть хотя бы один процесс; false - если нет.
 */
bool are_alive_tasks_exist(const char *const dir_path);

#endif /* SRC_TASKS_H_ */
