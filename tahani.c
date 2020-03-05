#include <janet.h>
#include <string.h>

#include "leveldb/include/leveldb/c.h"

#define FLAG_OPENED 0
#define FLAG_CLOSED 1
#define FLAG_CREATED 0
#define FLAG_DESTROYED 1
#define FLAG_RELEASED 1
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

typedef struct {
    const leveldb_snapshot_t* handle;
    leveldb_t* dbhandle;
    int flags;
} Snapshot;

typedef struct {
    leveldb_iterator_t* handle;
    int flags;
} Iterator;

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
        leveldb_writebatch_destroy(batch->handle);
    }
}

static void releasesnapshot(Snapshot *snapshot) {
    if (!(snapshot->flags & FLAG_RELEASED)) {
        snapshot->flags |= FLAG_RELEASED;
        leveldb_release_snapshot(snapshot->dbhandle, snapshot->handle);
    }
}

static void destroyiterator(Iterator *iterator) {
    if (!(iterator->flags & FLAG_DESTROYED)) {
        iterator->flags |= FLAG_DESTROYED;
        leveldb_iter_destroy(iterator->handle);
    }
}

static int gcdb(void *p, size_t s) {
    (void) s;
    Db *db = (Db *)p;
    closedb(db);
    return 0;
}

static void printdb(void *p, JanetBuffer *b) {
    Db *db = (Db *)p;
    char *state;
    switch (db->flags) {
    case FLAG_OPENED:
        state = "opened";
        break;
    case FLAG_CLOSED:
        state = "closed";
        break;
    default:
        state = "unknown";
    }

    const int32_t buflen = 11 + strlen(db->name) + strlen(state) + 1;
    char toprint[buflen];
    sprintf(toprint, "name=%s state=%s", db->name, state);

    janet_buffer_push_cstring(b, toprint);
}

static int gcbatch(void *p, size_t s) {
    (void) s;
    Batch *b = (Batch *) p;
    destroybatch(b);
    return 0;
}

static int gciterator(void *p, size_t s) {
    (void) s;
    Iterator *it = (Iterator *) p;
    destroyiterator(it);
    return 0;
}

static int gcsnapshot(void *p, size_t s) {
    (void) s;
    Snapshot *sn = (Snapshot *) p;
    releasesnapshot(sn);
    return 0;
}

static int dbget(void *p, Janet key, Janet *out);

static const JanetAbstractType AT_db = {
    "tahani/db",
    gcdb,
    NULL,
    dbget,
    NULL,
    NULL,
    NULL,
    printdb,
    JANET_ATEND_TOSTRING
};

static int batchget(void *p, Janet key, Janet *out);

static const JanetAbstractType AT_batch = {
    "tahani/batch",
    gcbatch,
    NULL,
    batchget,
    JANET_ATEND_GET
};

static int snapshotget(void *p, Janet key, Janet *out);

static const JanetAbstractType AT_snapshot = {
    "tahani/snapshot",
    gcsnapshot,
    NULL,
    snapshotget,
    JANET_ATEND_GET
};

static int iteratorget(void *p, Janet key, Janet *out);

static void printiterator(void *p, JanetBuffer *b) {
    Iterator *iterator = (Iterator *)p;
    char *state;
    switch (iterator->flags) {
    case FLAG_CREATED:
        state = "created";
        break;
    case FLAG_DESTROYED:
        state = "destroyed";
        break;
    default:
        state = "unknown";
    }

    const int32_t buflen = 6 + strlen(state) + 1;
    char toprint[buflen];
    sprintf(toprint, "state=%s", state);

    janet_buffer_push_cstring(b, toprint);
}

static const JanetAbstractType AT_iterator = {
    "tahani/iterator",
    gciterator,
    NULL,
    iteratorget,
    NULL,
    NULL,
    NULL,
    printiterator,
    JANET_ATEND_TOSTRING

};

static void paniconerr(char *err) {
    if (err != NULL) {
        const int message_len = 24 + strlen(err) + 1;
        char message[message_len];
        sprintf(message, "LevelDB returned error: %s", err);
        leveldb_free(err);
        janet_panic(message);
    }
    leveldb_free(err);
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

static Batch* initbatch() {
    leveldb_writebatch_t *wb = leveldb_writebatch_create();
    Batch* batch = (Batch *) janet_abstract(&AT_batch, sizeof(Batch));
    batch->handle = wb;
    batch->flags = FLAG_CREATED;
    return batch;
}

static Snapshot* initsnapshot(leveldb_t *db) {
    const leveldb_snapshot_t *sn = leveldb_create_snapshot(db);
    Snapshot* snapshot = (Snapshot *) janet_abstract(&AT_snapshot, sizeof(Snapshot));
    snapshot->handle = sn;
    snapshot->dbhandle = db;
    snapshot->flags = FLAG_CREATED;
    return snapshot;
}

static Iterator* inititerator(leveldb_t* db, leveldb_readoptions_t* readoptions) {
    leveldb_iterator_t *it = leveldb_create_iterator(db, readoptions);
    Iterator* iterator = (Iterator *) janet_abstract(&AT_iterator, sizeof(Iterator));
    iterator->handle = it;
    iterator->flags = FLAG_CREATED;
    return iterator;
}

static Janet cfun_open(int32_t argc, Janet *argv) {
    janet_arity(argc, 1, 2);
    const char *name = janet_getstring(argv, 0);
    leveldb_options_t *options = leveldb_options_create();
    leveldb_options_set_create_if_missing(options, 1);
    if (argc == 2) {
        const uint8_t *opt = janet_getkeyword(argv, 1);
        if (strcmp(opt, "eie") == 0) {
            leveldb_options_set_error_if_exists(options, 1);
        } else {
            janet_panic("Unrecognized option");
        }
    }
    null_err;
    leveldb_t *conn = leveldb_open(options, name, &err);
    paniconerr(err);

    Db *db = initdb(name, conn, options);

    return janet_wrap_abstract(db);
}

static Janet cfun_close(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);
    Db *db = janet_getabstract(argv, 0, &AT_db);
    closedb(db);
    return janet_wrap_nil();
}

static void paniconclosed(int flags) {
    if (flags & FLAG_CLOSED) janet_panic("LevelDB is already closed");
}

static Janet cfun_record_put(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 3);
    Db *db = janet_getabstract(argv, 0, &AT_db);
    paniconclosed(db->flags);
    const char *key = janet_getstring(argv, 1);
    size_t keylen = janet_string_length(key);
    const char *val = janet_getstring(argv, 2);
    size_t vallen = janet_string_length(val);
    null_err;

    leveldb_put(db->handle, db->writeoptions, key, keylen, val, vallen, &err);
    paniconerr(err);

    return janet_wrap_nil();
}

static Janet cfun_record_get(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 2);
    Db *db = janet_getabstract(argv, 0, &AT_db);
    paniconclosed(db->flags);
    const char *key = janet_getstring(argv, 1);
    size_t keylen = janet_string_length(key);
    const char* val;
    size_t vallen;
    null_err;

    val = leveldb_get(db->handle, db->readoptions, key, keylen, &vallen, &err);
    paniconerr(err);

    if (val == NULL) {
        return janet_wrap_nil();
    } else {
        Janet res = janet_stringv((uint8_t *) val, vallen);
        leveldb_free((void *) val);
        return res;
    }
}

static Janet cfun_record_delete(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 2);
    Db *db = janet_getabstract(argv, 0, &AT_db);
    paniconclosed(db->flags);
    const char *key = janet_getstring(argv, 1);
    size_t keylen = janet_string_length(key);
    null_err;

    leveldb_delete(db->handle, db->writeoptions, key, keylen, &err);
    paniconerr(err);

    return janet_wrap_nil();
}

static JanetMethod db_methods[] = {
    {"close", cfun_close},
    {"get", cfun_record_get},
    {"put", cfun_record_put},
    {"delete", cfun_record_delete},
    {NULL, NULL}
};

static int dbget(void *p, Janet key, Janet *out) {
    (void) p;
    if (!janet_checktype(key, JANET_KEYWORD))
        return 0;
    return janet_getmethod(janet_unwrap_keyword(key), db_methods, out);
}

static Janet cfun_destroy(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);
    const char *name = janet_getstring(argv, 0);
    null_err;

    leveldb_options_t *options = leveldb_options_create();
    leveldb_destroy_db(options, name, &err);
    paniconerr(err);

    return janet_wrap_nil();
}

static Janet cfun_repair(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);
    const char *name = janet_getstring(argv, 0);
    null_err;

    leveldb_options_t *options = leveldb_options_create();
    leveldb_repair_db(options, name, &err);
    paniconerr(err);

    return janet_wrap_nil();
}

static Janet cfun_batch_create(int32_t argc, Janet *argv) {
    (void) argv;
    janet_fixarity(argc, 0);
    Batch *batch = initbatch();

    return janet_wrap_abstract(batch);
}

static Janet cfun_batch_destroy(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);
    Batch *batch = janet_getabstract(argv, 0, &AT_batch);

    destroybatch(batch);
    return janet_wrap_nil();
}

static Janet cfun_batch_write(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 2);
    Db *db = janet_getabstract(argv, 1, &AT_db);
    paniconclosed(db->flags);
    Batch *batch = janet_getabstract(argv, 0, &AT_batch);
    if (batch->flags & FLAG_DESTROYED) janet_panic("Batch is already destroyed");
    null_err;
    leveldb_write(db->handle, db->writeoptions, batch->handle, &err);
    paniconerr(err);

    return janet_wrap_abstract(batch);
}

static Janet cfun_batch_put(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 3);
    Batch *batch = janet_getabstract(argv, 0, &AT_batch);
    const char *key = janet_getstring(argv, 1);
    size_t keylen = janet_string_length(key);
    const char *val = janet_getstring(argv, 2);
    size_t vallen = janet_string_length(val);
    null_err;

    leveldb_writebatch_put(batch->handle, key, keylen, val, vallen);

    return janet_wrap_abstract(batch);
}

static Janet cfun_batch_delete(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 2);
    Batch *batch = janet_getabstract(argv, 0, &AT_batch);
    const char *key = janet_getstring(argv, 1);
    size_t keylen = janet_string_length(key);
    null_err;

    leveldb_writebatch_delete(batch->handle, key, keylen);

    return janet_wrap_abstract(batch);
}

static JanetMethod batch_methods[] = {
    {"write", cfun_batch_write},
    {"put", cfun_batch_put},
    {"delete", cfun_batch_delete},
    {"destroy", cfun_batch_destroy},
    {NULL, NULL}
};

static int batchget(void *p, Janet key, Janet *out) {
    (void) p;
    if (!janet_checktype(key, JANET_KEYWORD))
        return 0;
    return janet_getmethod(janet_unwrap_keyword(key), batch_methods, out);
}

static Janet cfun_snapshot_create(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);
    Db *db = janet_getabstract(argv, 0, &AT_db);
    paniconclosed(db->flags);
    Snapshot *snapshot = initsnapshot(db->handle);

    return janet_wrap_abstract(snapshot);
}

static Janet cfun_snapshot_release(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);
    Snapshot *snapshot = janet_getabstract(argv, 0, &AT_snapshot);

    releasesnapshot(snapshot);
    return janet_wrap_nil();
}

static JanetMethod snapshot_methods[] = {
    {"release", cfun_snapshot_release},
    {NULL, NULL}
};

static int snapshotget(void *p, Janet key, Janet *out) {
    (void) p;
    if (!janet_checktype(key, JANET_KEYWORD))
        return 0;
    return janet_getmethod(janet_unwrap_keyword(key), snapshot_methods, out);
}

static Janet cfun_iterator_create(int32_t argc, Janet *argv) {
    janet_arity(argc, 1, 2);
    Db *db = janet_getabstract(argv, 0, &AT_db);
    paniconclosed(db->flags);
    leveldb_readoptions_t* readoptions = leveldb_readoptions_create();
    leveldb_readoptions_set_fill_cache(readoptions, 0);
    if (argc == 2) {
        Snapshot *sn = janet_getabstract(argv, 1, &AT_snapshot);
        leveldb_readoptions_set_snapshot(readoptions, sn->handle);
    }

    Iterator *iterator = inititerator(db->handle, readoptions);

    return janet_wrap_abstract(iterator);
}

static Janet cfun_iterator_valid(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);
    Iterator *iterator = janet_getabstract(argv, 0, &AT_iterator);

    return janet_wrap_boolean(leveldb_iter_valid(iterator->handle));
}

static void panicondestroyed(int flags) {
    if (flags & FLAG_DESTROYED) janet_panic("Iterator is already destroyed");
}

static Janet cfun_iterator_seek_to_first(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);
    Iterator *iterator = janet_getabstract(argv, 0, &AT_iterator);
    panicondestroyed(iterator->flags);
    leveldb_iter_seek_to_first(iterator->handle);

    return janet_wrap_abstract(iterator);
}

static Janet cfun_iterator_seek_to_last(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);
    Iterator *iterator = janet_getabstract(argv, 0, &AT_iterator);
    panicondestroyed(iterator->flags);

    leveldb_iter_seek_to_last(iterator->handle);

    return janet_wrap_abstract(iterator);
}

static Janet cfun_iterator_next(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);
    Iterator *iterator = janet_getabstract(argv, 0, &AT_iterator);
    panicondestroyed(iterator->flags);

    leveldb_iter_next(iterator->handle);

    return janet_wrap_abstract(iterator);
}

static Janet cfun_iterator_prev(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);
    Iterator *iterator = janet_getabstract(argv, 0, &AT_iterator);
    panicondestroyed(iterator->flags);

    leveldb_iter_prev(iterator->handle);

    return janet_wrap_abstract(iterator);
}

static Janet cfun_iterator_seek(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 2);
    Iterator *iterator = janet_getabstract(argv, 0, &AT_iterator);
    panicondestroyed(iterator->flags);
    const char *key = janet_getstring(argv, 1);
    size_t keylen = janet_string_length(key);

    leveldb_iter_seek(iterator->handle, key, keylen);

    return janet_wrap_abstract(iterator);
}

static Janet cfun_iterator_key(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);
    Iterator *iterator = janet_getabstract(argv, 0, &AT_iterator);
    panicondestroyed(iterator->flags);
    size_t keylen;
    const char* key = leveldb_iter_key(iterator->handle, &keylen);
    Janet res = janet_stringv((uint8_t *) key, keylen);
    return res;
}

static Janet cfun_iterator_value(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);
    Iterator *iterator = janet_getabstract(argv, 0, &AT_iterator);
    panicondestroyed(iterator->flags);
    size_t vallen;
    const char* value = leveldb_iter_value(iterator->handle, &vallen);
    Janet res = janet_stringv((uint8_t *) value, vallen);
    return res;
}

static Janet cfun_iterator_destroy(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);
    Iterator *iterator = janet_getabstract(argv, 0, &AT_iterator);

    destroyiterator(iterator);
    return janet_wrap_nil();
}

static JanetMethod iterator_methods[] = {
    {"destroy", cfun_iterator_destroy},
    {"valid?", cfun_iterator_valid},
    {"seek-to-first", cfun_iterator_seek_to_first},
    {"seek-to-last", cfun_iterator_seek_to_last},
    {"next", cfun_iterator_next},
    {"prev", cfun_iterator_prev},
    {"seek", cfun_iterator_seek},
    {"key", cfun_iterator_key},
    {"value", cfun_iterator_value},
    {NULL, NULL}
};

static int iteratorget(void *p, Janet key, Janet *out) {
    (void) p;
    if (!janet_checktype(key, JANET_KEYWORD))
        return 0;
    return janet_getmethod(janet_unwrap_keyword(key), iterator_methods, out);
}

static const JanetReg db_cfuns[] = {
    {"open", cfun_open, "(tahani/open name &opt options)\n\nOpens a level DB connection with the name. A name must be a string. Only option is :eie which sets error_if_exists. Option create_if_missing is implicit. "},
    {"close", cfun_close, "(tahani/close db)\n\nCloses a level DB connection. A db must be a tahani/db."},
    {NULL, NULL, NULL}
};

static const JanetReg record_cfuns[] = {
    {"record/put", cfun_record_put, "(tahani/record/put db key value)\n\nPut the valur under the key. A db must be a tahani/db, key and value must be a string"},
    {"record/get", cfun_record_get, "(tahani/record/get db key)\n\nGet val under the key. A key must be a string"},
    {"record/delete", cfun_record_delete, "(tahani/record/delete db key)\n\nGet val under the key. A key must be a string"},
    {NULL, NULL, NULL}
};

static const JanetReg batch_cfuns[] = {
    {"batch/create", cfun_batch_create, "(tahani/batch/create)\n\nCreate batch to which you can add operations.\n\nReturns the batch."},
    {"batch/destroy", cfun_batch_destroy, "(tahani/batch/destroy batch)\n\nDestroy batch."},
    {"batch/write", cfun_batch_write, "(tahani/batch/write batch db)\n\nWrite batch do db.\n\nReturns the batch."},
    {"batch/put", cfun_batch_put, "(tahani/batch/put batch key value)\n\nAdd put to the batch, key and value must be string.\n\nReturns the batch."},
    {"batch/delete", cfun_batch_delete, "(tahani/batch/delete batch key value)\n\nAdd delete to the batch, key and value must be string.\n\nReturns the batch."},
    {NULL, NULL, NULL}
};

static const JanetReg snapshot_cfuns[] = {
    {"snapshot/create", cfun_snapshot_create, "(tahani/snapshot/create db)\n\nCreates snapshot for the db. Returns the snapshot."},
    {"snapshot/release", cfun_snapshot_release, "(tahani/snapshot/release snapshot)\n\nReleases the snapshot."},
    {NULL, NULL, NULL}
};

static const JanetReg iterator_cfuns[] = {
    {"iterator/create", cfun_iterator_create, "(tahani/iterator/create db)\n\nCreates iterator for the db. Returns the iterator."},
    {"iterator/destroy", cfun_iterator_destroy, "(tahani/iterator/destroy iterator)\n\nDestroy the iterator."},
    {"iterator/valid?", cfun_iterator_valid, "(tahani/iterator/valid? iterator)\n\nReturns true if validator is valid"},
    {"iterator/seek-to-first", cfun_iterator_seek_to_first, "(tahani/iterator/seek-to-first iterator)\n\nSeeks to first iterator item."},
    {"iterator/seek-to-last", cfun_iterator_seek_to_last, "(tahani/iterator/seek-to-last iterator)\n\nSeeks to last iterator item."},
    {"iterator/next", cfun_iterator_next, "(tahani/iterator/next iterator)\n\nSeeks iterator to next item"},
    {"iterator/prev", cfun_iterator_prev, "(tahani/iterator/prev iterator)\n\nSeeks iterator to prev item"},
    {"iterator/seek", cfun_iterator_seek, "(tahani/iterator/seek iterator)\n\nSeeks iterator to provided key"},
    {"iterator/key", cfun_iterator_key, "(tahani/iterator/key iterator)\n\nReturns current key in iterator"},
    {"iterator/value", cfun_iterator_value, "(tahani/iterator/value iterator)\n\nReturns current value in iterator"},
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
    janet_cfuns(env, "tahani", snapshot_cfuns);
    janet_cfuns(env, "tahani", iterator_cfuns);
    janet_cfuns(env, "tahani", manage_cfuns);
}
