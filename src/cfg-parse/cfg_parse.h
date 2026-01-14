/**
 * @file
 * @brief Header file for cfg_parse.
 *
 * This file should be included to use any features of cfg_parse. Typically, a
 * user should create a pointer to a cfg_struct, initialize it with cfg_init(),
 * and then perform actions on that object (lookup, add, delete) by passing the
 * pointer to the functions here.
 * At end of use, call cfg_delete to clean up the object.
 */
#ifndef CFG_PARSE_H_
#define CFG_PARSE_H_

#include <stddef.h>

/**
 * @brief Sets the maximum size of a line in a configuration file.
 *
 * cfg_load uses this definition to limit the size of its read buffer.
 * Lines which exceed the length do not crash outright, but probably won't
 * load correctly.
 */
#define CFG_MAX_LINE 256

/* Opaque data structure holding config in memory */
struct cfg_struct;

/* Declare C-style name mangling,
    this makes mixing with c++ compilers possible */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates a cfg_struct.
 */
struct cfg_struct* cfg_init();

/**
 * @brief Frees a cfg_struct.
 */
void cfg_free(struct cfg_struct* cfg);

/**
 * @brief Loads key=value pairs from a file into cfg_struct.
 */
int cfg_load(struct cfg_struct* cfg, const char* filename);

/**
 * @brief Saves a cfg_struct to a file as key=value pairs.
 */
int cfg_save(const struct cfg_struct* cfg, const char* filename);

/**
 * @brief Retrieves a value from a cfg_struct for a specified key.
 */
const char* cfg_get(const struct cfg_struct* cfg, const char* key);

/**
 * @brief Retrieves a list of all keys in a cfg_struct.
 */
char** cfg_get_keys(const struct cfg_struct* cfg, size_t* count);

/**
 * @brief Sets a key, value pair in a cfg_struct.
 */
void cfg_set(struct cfg_struct* cfg, const char* key, const char* value);

/**
 * @brief Sets multiple key, value pairs in a cfg_struct.
 */
void cfg_set_array(struct cfg_struct* cfg, const char* keys[], const char* values[], size_t count);

/**
 * @brief Deletes a key (and associated value) from a cfg_struct.
 */
void cfg_delete(struct cfg_struct* cfg, const char* key);

/**
 * @brief Deletes multiple keys (and associated values) from a cfg_struct.
 */
void cfg_delete_array(struct cfg_struct* cfg, const char* keys[], size_t count);

/**
 * @brief Deletes all entries not found in keys[] from a cfg_struct.
 */
void cfg_prune(struct cfg_struct* cfg, const char* keys[], size_t count);

#ifdef __cplusplus
}
#endif

#endif
