# tahani

Simple (meanining not all implemented) and opinionated (meaning implemented
only what I needed) LevelDB[1] wrapper for Janet[1] language.

## Status

Mid Alpha.

## Goal

This library is mostly building block for my another project call mansion[2].
But if you need just the wrapper around LevelDB[2] C API, this is all you need.

As functionality for my purposes is mostly done, I am not planning any big
additions to library API, but any suggestions/PR are welcomed. Biggest omission
right now are Comparator and Filters facilities.

## Usage

### Basic database facilities

#### Opening the database

`(tahani/open db-name &opt opts)` opens the LevelDB[2] database. `db-name` must
be the `string` same as the directory name, where database resides on the disk.
Optional `opts` have only one defined value `:eie` for setting `error_if_exists`
LevelDB[2] option. Function returns Janet AbstractType tahani/db. This
AbstractType instance is used as parameter when calling most of the API.

Panics if any LevelDB error occurs.

#### Closing the database

`(tahani/close db)` closes the LevelDB[2] database. `db` must be instance
of `tahani/db` AbstractType returned from the `tahani/open` function mentioned
above.

Panics if any LevelDB error occurs.

#### Putting to the database

`(tahani/record/put db key value)` puts value under the key into the database.
`db` must be instance of `tahani/db` AbstractType returned from the
`tahani/open` function mentioned above. `key` and `value` must be `string` and
can contain `\0` characters. Returns nil on success.

Panics if any LevelDB error occurs.

This function can also be called as method on database AbstractType
`(:put db key value)`.

#### Getting from the database

`(tahani/record/get db key)` gets value under the key from the database.
`db` must be instance of `tahani/db` AbstractType returned from the
`tahani/open` function mentioned above. `key` must be `string` and can contain
`\0` characters. Returns `string` value from the database.

Panics if any LevelDB error occurs.

This function can also be called as method on database AbstractType
`(:get db key)`.

#### Deleting from the database

`(tahani/record/delete db key)` deletes value under the key from the database.
`key` must be `string` and can contain `\0` characters. Returns `nil` on
success

Panics if any LevelDB error occurs.

This function can also be called as method on database AbstractType
`(:delete db key)`.

### Database management facilities

#### Repairing the database

`(tahani/manage/repair )` repairs the database. `db-name` must
be the `string` same as the directory name, where database resides on the disk.
Database cannot be open.

Panics if any LevelDB error occurs.

#### Destroying the database

`(tahani/manage/destroy db-name)` destroys the database. `db-name` must
be the `string` same as the directory name, where database resides on the disk.
Database cannot be open.

Panics if any LevelDB error occurs.

THIS IS VERY DANGEROUS AS YOU WILL LOOSE EVERYTHING!

### Batch facilities

LevelDB[2] batches are the way for issuing multiple commands to database, which
are transacted as one atomic change. They are similar to relation database
transactions.

#### Creating the batch

`(tahani/batch/create)` creates new batch. It does not have any parameters as
batches are db independent. Returns Janet AbstractType `tahani/batch`.

#### Adding put into the batch

`(tahani/batch/put key value)` adds put values under the key into the batch. `key`
and `value` must be `string` and can contain `\0` characters. Returns the
`tahani/batch` on success, so it can be easily chained.

This function can also be called as method on `tahani/batch` AbstractType
`(:put batch key value)`.

#### Adding delete into the batch

`(tahani/batch/delete key)` adds delete the key into the batch. `key` must be
`string` and can contain `\0` characters. Returns the `tahani/batch` on success,
so it can be easily chained.

This function can also be called as method on `tahani/batch` AbstractType
`(:delete batch key)`.

#### Writting batch to the database

`(tahani/batch/write batch db)` writes the batch to the database. `db` must be
instance of `tahani/db` AbstractType returned from the `tahani/open` function
mentioned above. Returns the `tahani/batch` on success, so it can be easily
chained.

Panics if any LevelDB error occurs.

This function can also be called as method on `tahani/batch` AbstractType
`(:write batch key)`.

#### Destroying the batch

`(tahani/batch/destroy batch)` destroys the batch. `batch` must be instance of
`tahani/batch` returned from `create` function, mentioned above.

### Snapshot facilities

Snapshot is temporary content of the database in time. It can be used instead of
the db for `get` function mentioned above, or for the iterator mentioned
bellow.

#### Creating the snapshot

`(tahani/snapshot/create db)` creates new snapshot of the database. `db` must be
instance of `tahani/db` AbstractType returned from the `tahani/open` function
mentioned above. Returns `tahani/snapshot` Janet AbstractType.

#### Releasing the snapshot

`(tahani/snapshot/release snapshot)` releases the snapshot. `snapshot` must be
instance of `tahani/snapsho` AbstractType.

This function can also be called as method on `tahani/iterator` AbstractType
`(:release batch)`.

### Iterator facilities

Iterators can be used to traverse snapshot of the database.

#### Creating the iterator

`(tahani/iterator/create db &opt snapshot)` creates the iterator. `db` must be
instance of `tahani/db` AbstractType returned from the `tahani/open` function
mentioned above. Optional `snapshot` must be instance of `tahani/snapshot`
AbstractType, when it is not provided, implicit snapshot is created. Returns
`tahani/snapshot` Janet AbstractType.

#### Checking the iterator validity

`(tahani/iterator/valid? iterator)` checks if validator is in the valid
state. `iterator` must be instance of `tahani/iterator` returned by the
`create` method mentioned above. Returns true when the iterator is in the valid
state.

This function can also be called as method on `tahani/iterator` AbstractType
`(:valid? iterator)`

#### Getting current iterator position key

`(tahani/iterator/key iterator)` returns the key of the current iterator
position. `iterator` must be instance of `tahani/iterator` returned by the
`create` method mentioned above. Returns `string` with the key of the current
iterator record.

This function can also be called as method on `tahani/iterator` AbstractType
`(:key iterator)`

#### Getting current iterator position value

`(tahani/iterator/value iterator)` returns the value of the current iterator
position. `iterator` must be instance of `tahani/iterator` returned by the
`create` method mentioned above. Returns `string` with the value of the current
iterator record.

This function can also be called as method on `tahani/iterator` AbstractType
`(:value iterator)`

#### Moving to the next record in the iterator

`(tahani/iterator/next iterator)` moves the current position in the iterator to
the next record. `iterator` must be instance of `tahani/iterator` returned by
the `create` method mentioned above. Returns `iterator` so it can be easily
chained.

This function can also be called as method on `tahani/iterator` AbstractType
`(:next iterator)`

#### Seeking first record

`(tahani/iterator/seek-first iterator)` moves the current position in the
iterator to the first one.`iterator` must be instance of `tahani/iterator`
returned by the `create` method mentioned above. Returns `iterator` so it can
be easily chained.

This function can also be called as method on `tahani/iterator` AbstractType
`(:seek-first iterator)`

#### Seeking last record

`(tahani/iterator/seek-last iterator)` moves the current position in the
iterator to the last record. `iterator` must be instance of `tahani/iterator`
returned by the `create` method mentioned above. Returns `iterator` so it can
be easily chained.

This function can also be called as method on `tahani/iterator` AbstractType
`(:seek-last iterator)`

#### Seeking record

`(tahani/iterator/seek iterator key)` moves the current position in the
iterator to the record with `key`. `iterator` must be instance of
`tahani/iterator` returned by the `create` method mentioned above. `key`must be
string.  Returns `iterator` so it can be easily chained.

This function can also be called as method on `tahani/iterator` AbstractType
`(:seek iterator key)`

## TODOs

- [ ] @todo add open, read and write optional options
- [x] split store to mansion
- [x] add snapshot functionality
- [x] add iterator functions
- [x] add marshaling/unmarshaling store module
- [x] make batches chained
- [x] add batch functions
- [x] split to more modules
- [x] add delete function
- [x] add management functions
- [x] add error checks

