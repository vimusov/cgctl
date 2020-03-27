#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include <stdint.h>

// максимальная длина пути в /cgroup.
#define MAX_FILE_PATH (256)

// максимальная длина строки, содержащая uint64_t.
// на самом деле 20, но округляем по степени 2.
#define MAX_UINT64_STR_SIZE (32)

/*
 * \fn uint64_t str2uint(const char *const value)
 * \brief Конвертирует строку в целое положительное число, игнорируя конец строки если он есть.
 * \param const char *const value: Строка с числом.
 * \return Значение числа в строке. В случае любых ошибок возвращается 0. Это приемлемо т.к.
 *         программа работает со значениями, созданными системой cgroup, которым можно доверять.
 */
uint64_t str2uint(const char *const value);

/*
 * \fn unsigned int get_cpu_usage(const char *const value)
 * \brief Конвертирует из строки и возвращает ограничение по CPU в процентах.
 * \param const char *const value: Ограничение в виде строки.
 * \return Числовое значение ограничения, в процентах.
 * \warning Если значение некорректно, функция завершает программу с кодом 1.
 */
unsigned int get_cpu_usage(const char *const value);

/*
 * \fn unsigned int get_cpu_usage(const char *const value)
 * \brief Конвертирует из строки и возвращает ограничение по CPU в процентах.
 * \param const char *const value: Ограничение в виде строки.
 * \return Числовое значение ограничения, в процентах.
 * \warning Если значение некорректно, функция завершает программу с кодом 1.
 */
unsigned int get_mem_usage(const char *const value);

/*
 * \fn void format_path(char *file_path, const char *const dir_path, const char *const entry_name)
 * \brief Объединяет два элемента пути к файлу или каталогу.
 * \param char *file_path: Указатель на массив, в который будет помещён результат объединения.
 * \param const char *const dir_path: Путь к каталогу.
 * \param const char *const entry_name: Название файла или каталога.
 */
void format_path(char *file_path, const char *const dir_path, const char *const entry_name);

/*
 * \fn int read_num(uint64_t *out_value, const char *const dir_path, const char *const file_name)
 * \brief Читает целочисленное значение из файла в /cgroup/$group_name.
 * \param uint64_t *out_value: Указатель на переменную, в которую будет помещено прочитанное значение.
 * \param const char *const dir_path: Путь к каталогу /cgroup/$group_name.
 * \param const char *const file_name: Название файла.
 * \return 1 в случае ошибок; 0 если значение прочитано успешно.
 */
int read_num(uint64_t *out_value, const char *const dir_path, const char *const file_name);

/*
 * \fn int write_num(const uint64_t value, const char *const dir_path, const char *const file_name)
 * \brief Записывает целочисленное значение в файл в /cgroup.
 * \param const uint64_t value: Записываемое значение.
 * \param const char *const dir_path: Путь к каталогу /cgroup/$group_name.
 * \param const char *const file_name: Название файла.
 * \return 1 в случае ошибок; 0 если значение записано успешно.
 */
int write_num(const uint64_t value, const char *const dir_path, const char *const file_name);

/*
 * \fn void get_group_name(const char *const file_path, char *group)
 * \brief Получает название группы из пути к файлу или имени файла запускаемой программы.
 * \param const char *const file_path: Путь к файлу или имя файла запускаемой программы.
 * \param char *group: Название группы.
 * \warning В случае ошибок функция завершает программу с кодом 1.
 */
void get_group_name(const char *const file_path, char *group);

/*
 * \fn void copy_raw_content(const char *const src_dir_path, const char *const dst_dir_path, const char *const file_name)
 * \brief Копирует содержимое файла.
 * \param const char *const src_dir_path: Каталог с исходным файлом (откуда копируем).
 * \param const char *const dst_dir_path: Каталог с файлом назначения (куда копируем).
 * \param const char *const file_name: Название файла;
 * \warning В случае ошибок вызывает функцию abort().
 */
void copy_raw_content(const char *const src_dir_path, const char *const dst_dir_path, const char *const file_name);

#endif /* SRC_UTILS_H_ */
