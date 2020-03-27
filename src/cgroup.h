#ifndef SRC_CGROUP_H_
#define SRC_CGROUP_H_

/*
 * \fn void cgroup_append(const char *const name)
 * \brief Добавляет текущий процесс в существующую cgroup.
 * \param const char *const name: Название cgroup.
 */
void cgroup_append(const char *const name);

/*
 * \fn cgroup_create(const char *const name, const unsigned int cpu_usage, const unsigned int mem_usage)
 * \brief Создаёт новый cgroup, устанавливает ограничения и помещает текущий процесс в список процессов cgroup.
 * \param const char *const name: Название cgroup.
 * \param const unsigned int cpu_usage: Ограничение по CPU, в процентах.
 * \param const unsigned int mem_usage: Ограничение по памяти, в процентах.
 */
void cgroup_create(const char *const name, const unsigned int cpu_usage, const unsigned int mem_usage);

/*
 * \fn void cgroup_destroy(const char *const name)
 * \brief Прибивает все процессы в cgroup и удаляёт cgroup.
 * \param const char *const name: Название cgroup.
 */
void cgroup_destroy(const char *const name);

#endif /* SRC_CGROUP_H_ */
