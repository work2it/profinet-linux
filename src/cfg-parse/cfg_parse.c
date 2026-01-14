#include "cfg_parse.h"

/* for malloc, EXIT_SUCCESS and _FAILURE, exit */
#include <stdlib.h>
/* for FILE*, fgets, fputs */
#include <stdio.h>
/* for memset, strlen, strchr etc */
#include <string.h>
/* for tolower, isspace */
#include <ctype.h>

/* SIZE_MAX */
#if __STDC_VERSION__ >= 199901L
  #include <stdint.h>
#elif !defined(SIZE_MAX)
  #define SIZE_MAX ((size_t)-1)
#endif

/* ************************************************************************* */
/* implementation details of (opaque) config structures */
/* ************************************************************************* */

struct cfg_node
{
  char* key;
  char* value;

  struct cfg_node* next;
};

struct cfg_struct
{
  struct cfg_node* head;
};

/* ************************************************************************* */
/* Helper functions */
/* ************************************************************************* */

/* A malloc() wrapper which handles null return values */
static void* cfg_malloc(const size_t size)
{
  void* ptr =
    malloc(size);

  if (ptr == NULL)
  {
    perror("cfg_parse: ERROR: malloc() returned NULL");
    exit(EXIT_FAILURE);
  }

  return ptr;
}

/* Returns a duplicate of input str, without leading / trailing whitespace
    Input str *MUST* be null-terminated, or disaster will result */
static char* cfg_trim(const char* str)
{
  size_t tlen;
  char* tstr;

  /* advance start pointer to first non-whitespace char */
  while (isspace(*str))
    str ++;

  /* roll back length until we run out of whitespace */
  tlen = strlen(str);
  while (tlen > 0 && isspace(str[tlen - 1]))
    tlen --;

  /* copy portion of string to new string */
  tstr = cfg_malloc(tlen + 1);
  tstr[tlen] = '\0';
  if (tlen > 0) memcpy(tstr, str, tlen);

  return tstr;
}

/* Returns a duplicate of input str, without leading / trailing whitespace
    Also lowercases the string, AND returns NULL instead of empty str */
static char* cfg_norm_key(const char* key)
{
  size_t i, tlen;
  char* tkey;

  /* advance start pointer to first non-whitespace char */
  while (isspace(*key))
    key ++;

  /* roll back length until we run out of whitespace */
  tlen = strlen(key);
  while (tlen > 0 && isspace(key[tlen - 1]))
    tlen --;

  /* Exclude empty key */
  if (tlen == 0) return NULL;

  /* copy portion of string to new string */
  tkey = cfg_malloc(tlen + 1);
  tkey[tlen] = '\0';
  /* Lowercase key and copy */
  for (i = 0; i < tlen; i++)
    tkey[i] = tolower(key[i]);

  return tkey;
}

/* Creates a node struct with key and value, and returns it */
static struct cfg_node* cfg_create_node(char* key, char* value)
{
  struct cfg_node* cur =
    cfg_malloc(sizeof(struct cfg_node));

  /* assign key, value */
  cur->key = key;
  cur->value = value;
  cur->next = NULL;

  return cur;
}

/* ************************************************************************* */
/* Public functions */
/* ************************************************************************* */

/**
 * This function initializes a cfg_struct, and must be called before
 * performing any further operations.
 * @return Pointer to newly initialized cfg_struct object.
 */
struct cfg_struct* cfg_init()
{
  struct cfg_struct* cfg =
    cfg_malloc(sizeof(struct cfg_struct));
  cfg->head = NULL;

  return cfg;
}

/**
 * This function deletes an entire cfg_struct, clearing any memory
 * previously held by the structure.
 * @param cfg Pointer to cfg_struct to delete.
 */
void cfg_free(struct cfg_struct* cfg)
{
  struct cfg_node* temp;

  if (cfg == NULL) return;

  while ((temp = cfg->head) != NULL)
  {
    cfg->head = temp->next;

    free(temp->key);
    free(temp->value);
    free(temp);
  }

  free(cfg);
}

/**
 * This function loads data from a file, and inserts / updates the specified
 *  cfg_struct.  New keys will be inserted.  Existing keys will have values
 *  overwritten by those read from the file.
 * The format of config-files is "key=value", with any amount of whitespace.
 * Comments can be added, beginning with a # character until end-of-line.
 * The maximum line size is CFG_MAX_LINE bytes (see cfg_parse.h)
 * @param cfg Pointer to cfg_struct to update.
 * @param filename String containing filename to open and parse.
 * @return EXIT_SUCCESS (0) on success, or EXIT_FAILURE if file could not be
 *  opened.
 */
int cfg_load(struct cfg_struct* cfg, const char* filename)
{
  FILE* fp;
  char buffer[CFG_MAX_LINE + 1];

  /* safety check: null input */
  if (cfg == NULL || filename == NULL) return EXIT_FAILURE;

  /* open file for reading */
  fp = fopen(filename, "r");
  if (fp == NULL) return EXIT_FAILURE;

  while (fgets(buffer, CFG_MAX_LINE + 1, fp) != NULL)
  {
    /* locate first # sign and terminate string there (comment) */
    char* delim = strchr(buffer, '#');
    if (delim != NULL) *delim = '\0';

    /* locate first = sign and prepare to split */
    delim = strchr(buffer, '=');
    if (delim != NULL)
    {
      *delim = '\0';
      delim ++;

      cfg_set(cfg, buffer, delim);
    }
    /* else: no '=', so either a blank or invalid line */
  }

  /* print warning in case the read attempt failed but not because EOF */
  if (!feof(fp))
  {
    perror("cfg_parse: Warning: cfg_load() early termination:");
  }

  fclose(fp);
  return EXIT_SUCCESS;
}

/**
 * This function saves a complete cfg_struct to a file.
 * Comments are not preserved.
 * @param cfg Pointer to cfg_struct to save.
 * @param filename String containing filename to open and parse.
 * @return EXIT_SUCCESS (0) on success, or EXIT_FAILURE if file could not be
 *  opened or a write error occurred.
 */
int cfg_save(const struct cfg_struct* cfg, const char* filename)
{
  FILE* fp;
  struct cfg_node* cur;

  /* safety check: null input */
  if (cfg == NULL || filename == NULL) return EXIT_FAILURE;

  /* open output file for writing */
  fp = fopen(filename, "w");
  if (fp == NULL) return EXIT_FAILURE;

  /* point at first item in list */
  cur = cfg->head;
  /* step through the list, dumping each key-value pair to disk */
  while (cur != NULL)
  {
    if (fputs(cur->key, fp) == EOF ||
        fputc('=', fp) == EOF ||
        fputs(cur->value, fp) == EOF ||
        fputc('\n', fp) == EOF)
    {
      fclose(fp);
      return EXIT_FAILURE;
    }

    cur = cur->next;
  }

  fclose(fp);
  return EXIT_SUCCESS;
}

/**
 * This function performs a key-lookup on a cfg_struct, and returns the
 *  associated value.
 * @param cfg Pointer to cfg_struct to search.
 * @param key String containing key to search for.
 * @return String containing associated value, or NULL if key was not found.
 */
const char* cfg_get(const struct cfg_struct* cfg, const char* key)
{
  char* tkey;
  struct cfg_node* cur;

  /* safety check: null input */
  if (cfg == NULL || cfg->head == NULL || key == NULL) return NULL;

  /* Trim input search key */
  tkey = cfg_norm_key(key);
  /* Exclude empty key */
  if (tkey == NULL) return NULL;

  /* set up pointer to start of list */
  cur = cfg->head;
  /* loop through linked list looking for match on key
    if found, free search key, return the value */
  do
  {
    if (strcmp(tkey, cur->key) == 0)
    {
      free(tkey);
      return cur->value;
    }
  } while ((cur = cur->next) != NULL);

  free(tkey);
  return NULL;
}

/**
 * Returns an array of strings, one for each key in the config struct.
 * This array and its contents are dynamically allocated: the caller
 * should free them when done.
 * @param cfg Pointer to cfg_struct to retrieve keys from.
 * @param count Output parameter: number of keys in the returned array.
 * @return Array of (count) strings, one for each key in the struct, or NULL
 *  in case of error or empty struct.
 */
char** cfg_get_keys(const struct cfg_struct* cfg, size_t* count)
{
  size_t i;
  char** keys;
  struct cfg_node* cur;

  /* safety check: null input */
  if (cfg == NULL || cfg->head == NULL) return NULL;

  /* walk the list to count how many keys we have available */
  i = 0;
  for (cur = cfg->head; cur != NULL; cur = cur->next)
    i ++;

  /* now create the array to hold them all */
  if (SIZE_MAX / sizeof(char*) < i) return NULL;
  keys = cfg_malloc(i * sizeof(char*));

  /* walk the list again, this time allocating and copying each key */
  cur = cfg->head;
  for (*count = 0; *count < i; (*count) ++)
  {
    /* create space to hold the key, and copy it over */
    keys[*count] = cfg_malloc(strlen(cur->key) + 1);
    strcpy(keys[*count], cur->key);
    cur = cur->next;
  }

  return keys;
}

/**
 * This function sets a single key-value pair in a cfg_struct.
 * If the key already exists, its value will be updated.
 * If not, a new item is added to the cfg_struct list.
 * For convenience, a NULL value is treated as a call to cfg_delete().
 * @param cfg Pointer to cfg_struct to search.
 * @param key String containing key to search for.
 * @param value String containing new value to assign to key.
 */
void cfg_set(struct cfg_struct* cfg, const char* key, const char* value)
{
  char* tkey;
  char* tvalue;

  struct cfg_node* cur;

  /* Treat NULL value as a "delete" operation */
  if (value == NULL)
  {
    cfg_delete(cfg, key);
    return;
  }

  /* safety check: null input */
  if (cfg == NULL || key == NULL) return;

  /* Trim input search key */
  tkey = cfg_norm_key(key);
  /* Exclude empty key */
  if (tkey == NULL) return;

  /* Trim value. */
  tvalue = cfg_trim(value);

  if (cfg->head == NULL)
  {
    /* list was empty to begin with */
    cfg->head = cfg_create_node(tkey, tvalue);
  } else {
    struct cfg_node* prev;

    /* search list for existing key */
    cur = cfg->head;
    do
    {
      if (strcmp(tkey, cur->key) == 0)
      {
        /* found a match: no longer need cur key */
        free(tkey);

        /* update value */
        free(cur->value);
        cur->value = tvalue;
        return;
      }
      prev = cur;
    } while ((cur = cur->next) != NULL);

    /* not found: create new element and append it */
    prev->next = cfg_create_node(tkey, tvalue);
  }
}

/**
 * This function sets multiple key-value pairs in a cfg_struct.
 * @param cfg Pointer to cfg_struct to search.
 * @param keys Array of strings containing key to search for.
 * @param values Array of strings containing new value to assign to key.
 * @param count Length of keys / values arrays
 */
void cfg_set_array(struct cfg_struct* cfg, const char* keys[], const char* values[], const size_t count)
{
  size_t i;

  /* safety check: null input */
  if (cfg == NULL || keys == NULL || values == NULL || count == 0) return;

  /* Call cfg_set on every item in the lists */
  for (i = 0; i < count; i ++)
    cfg_set(cfg, keys[i], values[i]);
}

/**
 * This function deletes a key-value pair from a cfg_struct.
 * If the key does not exist, the function does nothing.
 * @param cfg Pointer to cfg_struct to search.
 * @param key String containing key to search for.
 */
void cfg_delete(struct cfg_struct* cfg, const char* key)
{
  char* tkey;
  struct cfg_node* cur;
  struct cfg_node* prev;

  /* safety check: null input */
  if (cfg == NULL || cfg->head == NULL || key == NULL) return;

  /* Trim input search key */
  tkey = cfg_norm_key(key);
  /* Exclude empty key */
  if (tkey == NULL) return;

  /* search list for existing key */
  cur = cfg->head;
  do
  {
    if (strcmp(tkey, cur->key) == 0)
    {
      /* found it - cleanup trimmed key */
      free(tkey);

      if (cur == cfg->head)
      {
        /* first element */
        cfg->head = cur->next;
      } else {
        /* splice out element */
        prev->next = cur->next;
      }

      /* delete element */
      free(cur->value);
      free(cur->key);
      free(cur);

      return;
    }

    prev = cur;
  } while ((cur = cur->next) != NULL);
  /* not found */

  /* cleanup trimmed key */
  free(tkey);
}

/**
 * This function deletes multiple key-value pairs from a cfg_struct.
 * @param cfg Pointer to cfg_struct to search.
 * @param keys Array of strings containing key to search for.
 * @param count Length of keys array
 */
void cfg_delete_array(struct cfg_struct* cfg, const char* keys[], const size_t count)
{
  size_t i;

  /* safety check: null input */
  if (cfg == NULL || cfg->head == NULL || keys == NULL || count == 0) return;

  /* Call cfg_delete on every item in the list */
  for (i = 0; i < count; i ++)
    cfg_delete(cfg, keys[i]);
}

/**
 * This function performs the inverse of cfg_delete_array().
 * Instead of deleting entries from cfg which match keys[],
 *  this will KEEP only those entries that match keys[].
 * It can be used to keep a config file tidy between versions or
 *  after user edits.
 * @param cfg Pointer to cfg_struct to search.
 * @param keys Array of strings containing keys to keep
 * @param count Length of keys array
 */
void cfg_prune(struct cfg_struct* cfg, const char* keys[], const size_t count)
{
  char** tkeys;
  size_t i, j;

  struct cfg_node* cur;
  struct cfg_node* prev;

  /* safety check: null input */
  if (cfg == NULL || cfg->head == NULL || keys == NULL || count == 0 ||
    SIZE_MAX / sizeof(char*) < count) return;

  /* First we must prep every key in keys[] using the normalize function. */
  tkeys = cfg_malloc(count * sizeof(char*));

  j = 0;
  for (i = 0; i < count; i ++)
  {
    char *tkey;

    if (keys[i] == NULL) continue;
    tkey = cfg_norm_key(keys[i]);
    if (tkey == NULL) continue;
    
    tkeys[j] = tkey;
    j ++;
  }

  if (j == 0)
  {
    free(tkeys);
    return;
  }

  /* Now iterate through the cfg struct and test every entry */
  cur = cfg->head;
  do
  {
    for (i = 0; i < j; i ++)
      if (strcmp(tkeys[i], cur->key) == 0)
        break;

    if (i == j)
    {
      /* Didn't find a key match - delete this */
      free(cur->value);
      free(cur->key);

      if (cur == cfg->head)
      {
        /* first element */
        cfg->head = cur->next;
        free(cur);
        cur = cfg->head;
      } else {
        /* splice out element */
        prev->next = cur->next;
        free(cur);
        cur = prev->next;
      }
    } else {
      /* matched, advance list element */
      prev = cur;
      cur = cur->next;
    }
  } while (cur != NULL);

  /* Cleanup all our trimmed keys */
  for (i = 0; i < j; i ++)
    free(tkeys[i]);
  free(tkeys);
}
