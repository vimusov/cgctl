#ifndef SRC_FREEZER_H_
#define SRC_FREEZER_H_

/*
 * \fn int freeze_group(const char *const dir_path)
 * \brief "Замораживает" заданную cgroup.
 * \param const char *const dir_path: Путь к каталогу с cgroup.
 * \return 1 в случае ошибки; 0 если всё хорошо.
 */
int freeze_group(const char *const dir_path);

/*
 * \fn int freeze_group(const char *const dir_path)
 * \brief "Размораживает" заданную cgroup.
 * \param const char *const dir_path: Путь к каталогу с cgroup.
 * \return 1 в случае ошибки; 0 если всё хорошо.
 */
int unfreeze_group(const char *const dir_path);

#endif /* SRC_FREEZER_H_ */
