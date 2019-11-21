#include <janet.h>
#include <string.h>

#include "leveldb/include/leveldb/c.h"

#define FLAG_OPENED 0
#define FLAG_CLOSED 1
#define MSG_DB_CLOSED "database already closed"
#define null_err char *err = NULL


typedef struct {
    const char *name;
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
    leveldb_free(err);
    err = NULL;
}

static void paniconerr(char *err) {
    if (err != NULL) {
        janet_panic(err);
    } else {
        free_err(err);
    }
}
static Db* initdb(const char *name, leveldb_t *conn, leveldb_options_t *options) {
    Db* db = (Db *) janet_abstract(&AT_db, sizeof(Db));
    db->name = name;
    db->handle = conn;
    db->options = options;
    db->readoptions = leveldb_readoptions_create();
    db->writeoptions = leveldb_writeoptions_create();
    db->flags = FLAG_OPENED;
    return db;
}

static Janet cfun_open(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);
    const char *name = janet_getcstring(argv, 0);
    leveldb_options_t *options = leveldb_options_create();
    null_err;
    leveldb_options_set_create_if_missing(options, 1);
    leveldb_t *conn = leveldb_open(options, name, &err);

    Db *db = initdb(name, conn, options);
    paniconerr(err);

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
    paniconerr(err);

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
    paniconerr(err);

    if (val == NULL) {
        return janet_wrap_nil();
    } else {
        return janet_stringv((uint8_t *) val, vallen);
    }
}

static Janet cfun_delete(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 2);
    Db *db = janet_getabstract(argv, 0, &AT_db);
    const char *key = janet_getcstring(argv, 1);
    size_t keylen = strlen(key);
    null_err;

    leveldb_delete(db->handle, db->writeoptions, key, keylen, &err);
    paniconerr(err);

    return janet_wrap_nil();
}

static Janet cfun_destroy(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);
    const char *name = janet_getcstring(argv, 0);
    null_err;

    leveldb_options_t *options = leveldb_options_create();
    leveldb_destroy_db(options, name, &err);
    paniconerr(err);

    return janet_wrap_nil();
}

static Janet cfun_repair(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);
    const char *name = janet_getcstring(argv, 0);
    null_err;

    leveldb_options_t *options = leveldb_options_create();
    leveldb_repair_db(options, name, &err);
    paniconerr(err);

    return janet_wrap_nil();
}

static const JanetReg core_cfuns[] = {
    {"open", cfun_open, "(tahani/open name)\n\nOpens a level DB connection with the name. A name must be a string"},
    {"close", cfun_close, "(tahani/close db)\n\nCloses a level DB connection. A db must be a tahani/db."},
    {"destroy", cfun_destroy, "(tahani/destroy db)\n\nDestroy the level DB with the name. A name must be a string."},
    {"repair", cfun_repair, "(tahani/repair db)\n\nDestroy the level DB with the name. A name must be a string."},
    {NULL, NULL, NULL}
};

static const JanetReg record_cfuns[] = {
    {"put", cfun_put, "(tahani/record/put db key val)\n\nPut the val under the key. A db must be a tahani/db, key and val must be a string"},
    {"get", cfun_get, "(tahani/record/get db key)\n\nGet val under the key. A key must be a string"},
    {"delete", cfun_delete, "(tahani/record/delete db key)\n\nGet val under the key. A key must be a string"},
    {NULL, NULL, NULL}
};

JANET_MODULE_ENTRY(JanetTable *env) {
    janet_cfuns(env, "tahani", core_cfuns);
    janet_cfuns(env, "tahani/record", record_cfuns);
}
