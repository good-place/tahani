#include <janet.h>
#include <string.h>

#include "leveldb/include/leveldb/c.h"

#define FLAG_CLOSED 1
#define MSG_DB_CLOSED "database already closed"
#define null_err char *err = NULL 


typedef struct {
    leveldb_t* handle;
    leveldb_options_t* options;
    leveldb_readoptions_t* readoptions;
    leveldb_writeoptions_t* writeoptions;
    int flags;
} Db;

/* Close a db, noop if already closed */
static void closedb(Db *db) {
    if (!(db->flags & FLAG_CLOSED)) {
        db->flags |= FLAG_CLOSED;
        leveldb_options_destroy(db->options);
        leveldb_readoptions_destroy(db->readoptions);
        leveldb_writeoptions_destroy(db->writeoptions);
        leveldb_close(db->handle);
    }
}

/* Called to garbage collect a sqlite3 connection */
static int gcleveldb(void *p, size_t s) {
    (void) s;
    Db *db = (Db *)p;
    closedb(db);
    return 0;
}

static const JanetAbstractType AT_db = {
    "tahani/db",
    gcleveldb,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static void free_err(char *err) {
  leveldb_free(err); err = NULL;
}

static Db* initdb(leveldb_t *conn, leveldb_options_t *options) {
  Db* db = (Db *) janet_abstract(&AT_db, sizeof(Db));
  db->handle = conn;
  db->options = options;
  db->readoptions = leveldb_readoptions_create();
  db->writeoptions = leveldb_writeoptions_create();
  db->flags = 0;
  return db;
}

static Janet cfun_open(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  const char *name = janet_getcstring(argv, 0);
  leveldb_options_t *options;
  null_err;
  options = leveldb_options_create();
  leveldb_options_set_create_if_missing(options, 1);
  leveldb_t *conn = leveldb_open(options, name, &err);
 
  Db *db = initdb(conn, options);
  free_err(err);
  return janet_wrap_abstract(db);
}

static Janet cfun_close(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 1);
  Db *db = janet_getabstract(argv, 0, &AT_db);
  closedb(db);
  return janet_wrap_nil();
}

static Janet cfun_put(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 3);
  Db *db = janet_getabstract(argv, 0, &AT_db);
  const char *key = janet_getcstring(argv, 1);
  size_t keylen = strlen(key);
  const char *val = janet_getcstring(argv, 2);
  size_t vallen = strlen(val);
  null_err;

  leveldb_put(db->handle, db->writeoptions, key, keylen, val, vallen, &err);
  free_err(err);
  return janet_wrap_nil();
}

static Janet cfun_get(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  Db *db = janet_getabstract(argv, 0, &AT_db);
  const char *key = janet_getcstring(argv, 1);
  size_t keylen = strlen(key);
  const char* val;
  size_t vallen;
  null_err;

  val = leveldb_get(db->handle, db->readoptions, key, keylen, &vallen, &err);
  free_err(err);

  if (val == NULL) {
    return janet_wrap_nil();
  } else {
    return janet_cstringv(val);
  }
}

static const JanetReg cfuns[] = {
    {"open", cfun_open, "(tahani/open name)\n\nOpens level DB connection with name."},
    {"close", cfun_close, "(tahani/close db)\n\nCloses level DB connection."},
    {"put", cfun_put, "(tahani/put db key val)\n\nPut val under key."},
    {"get", cfun_get, "(tahani/get db key val)\n\nGet val under key."},
    {NULL, NULL, NULL}
};

JANET_MODULE_ENTRY(JanetTable *env) {
    janet_cfuns(env, "tahani", cfuns);
}
