#include <janet.h>
#include <string.h>

#include "leveldb/include/leveldb/c.h"

#define FLAG_OPENED 0
#define FLAG_CLOSED 1
#define FLAG_CREATED 2
#define FLAG_DESTROYED 3
#define null_err char *err = NULL


typedef struct {
    const char *name;
    leveldb_t* handle;
    leveldb_options_t* options;
    leveldb_readoptions_t* readoptions;
    leveldb_writeoptions_t* writeoptions;
    int flags;
} Db;

typedef struct {
    leveldb_writebatch_t* handle;
    int flags;
} Batch;

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

static void destroybatch(Batch *batch) {
    if (!(batch->flags & FLAG_DESTROYED)) {
        batch->flags |= FLAG_DESTROYED;
        leveldb_writebatch_clear(batch->handle);
        leveldb_writebatch_destroy(batch->handle);
    }
}

static int gcdb(void *p, size_t s) {
    (void) s;
    Db *db = (Db *)p;
    closedb(db);
    return 0;
}

static int gcbatch(void *p, size_t s) {
    (void) s;
    Batch *b = (Batch *) p;
    destroybatch(b);
    return 0;
}

static const JanetAbstractType AT_db = {
    "tahani/db",
    gcdb,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static const JanetAbstractType AT_batch = {
    "tahani/batch",
    gcbatch,
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
    }
    free_err(err);
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

static Batch* initbatch(leveldb_writebatch_t *wb) {
    Batch* batch = (Batch *) janet_abstract(&AT_batch, sizeof(Batch));
    batch->handle = wb;
    batch->flags = FLAG_CREATED;
    return batch;
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

static Janet cfun_record_put(int32_t argc, Janet *argv) {
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

static Janet cfun_record_get(int32_t argc, Janet *argv) {
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

static Janet cfun_record_delete(int32_t argc, Janet *argv) {
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

static Janet cfun_create_batch(int32_t argc, Janet *argv) {
    (void) argv;
    janet_fixarity(argc, 0);
    leveldb_writebatch_t *wb = leveldb_writebatch_create();
    Batch *batch = initbatch(wb);

    return janet_wrap_abstract(batch);
}

static Janet cfun_destroy_batch(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);
    Batch *batch = janet_getabstract(argv, 0, &AT_batch);

    destroybatch(batch);
    return janet_wrap_nil();
}

static const JanetReg db_cfuns[] = {
    {"open", cfun_open, "(tahani/open name)\n\nOpens a level DB connection with the name. A name must be a string"},
    {"close", cfun_close, "(tahani/close db)\n\nCloses a level DB connection. A db must be a tahani/db."},
    {NULL, NULL, NULL}
};

static const JanetReg record_cfuns[] = {
    {"record/put", cfun_record_put, "(tahani/record/put db key val)\n\nPut the val under the key. A db must be a tahani/db, key and val must be a string"},
    {"record/get", cfun_record_get, "(tahani/record/get db key)\n\nGet val under the key. A key must be a string"},
    {"record/delete", cfun_record_delete, "(tahani/record/delete db key)\n\nGet val under the key. A key must be a string"},
    {NULL, NULL, NULL}
};

static const JanetReg batch_cfuns[] = {
    {"batch/create", cfun_create_batch, "(tahani/batch/create)\n\nCreate batch to which you can add operations."},
    {"batch/destroy", cfun_destroy_batch, "(tahani/batch/destroy batch)\n\nDestroy batch."},
    {NULL, NULL, NULL}
};

static const JanetReg manage_cfuns[] = {
    {"manage/destroy", cfun_destroy, "(tahani/destroy db)\n\nDestroy the level DB with the name. A name must be a string."},
    {"manage/repair", cfun_repair, "(tahani/repair db)\n\nDestroy the level DB with the name. A name must be a string."},
    {NULL, NULL, NULL}
};

JANET_MODULE_ENTRY(JanetTable *env) {
    janet_cfuns(env, "tahani", db_cfuns);
    janet_cfuns(env, "tahani", record_cfuns);
    janet_cfuns(env, "tahani", batch_cfuns);
    janet_cfuns(env, "tahani", manage_cfuns);
}
