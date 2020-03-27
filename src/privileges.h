#ifndef PRIVILEGES_H_
#define PRIVILEGES_H_

/**
 * \fn int drop_privileges(const char *const user_name)
 * \brief Сброс привилегий на заданного пользователя.
 * \param const char *const user_name: Имя пользователя.
 * \return 0 при успешном завершении операции, 1 в случае ошибки.
 */
int drop_privileges(const char *const user_name);

#endif /* PRIVILEGES_H_ */
