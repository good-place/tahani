#include <janet.h>
#include "leveldb/include/leveldb/c.h"

static const JanetAbstractType AT_db = {
    "tahani/db",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static Janet cfun_open(int32_t argc, const Janet *argv) {
  janet_fixarity(argc, 1);
  const char *name = janet_getcstring(argv, 0);
  leveldb_options_t *options;
  char *err = NULL;
  leveldb_t *db = janet_abstract(&AT_db, sizeof(db));

  options = leveldb_options_create();
  leveldb_options_set_create_if_missing(options, 1);
  db = leveldb_open(options, name, &err);
  return janet_wrap_abstract(db);
}

static Janet cfun_close(int32_t argc, const Janet *argv) {
  janet_fixarity(argc, 1);
  leveldb_t *db = janet_getabstract(argv, 0, &AT_db);
  char *err = NULL;
  leveldb_close(db);
  return janet_wrap_nil();
}


static const JanetReg cfuns[] = {
    {"open", cfun_open, "(tahani/open name)\n\nOpens level DB with name."},
    {"close", cfun_close, "(tahani/close db)\n\nCloses level DB."},
    {NULL, NULL, NULL}
};

JANET_MODULE_ENTRY(JanetTable *env) {
    janet_cfuns(env, "tahani", cfuns);
}
