
# tahani

Simple (meaning not all implemented) and opinionated (meaning implemented
only what I needed) [LevelDB][2] wrapper for [Janet][1] language.

## Status

Mid Alpha.

## Goal

This library is mostly building block for my another project called
[mansion][3].
But if you need just the wrapper around LevelDB C API, this is all you need.

As I have implemented functionality for my purposes, I am not planning any
significant
additions to library API, but I welcome any suggestions/PR. Biggest omission
right now are Comparator and Filters facilities.

[1]: https://github.com/janet-lang/janet
[2]: https://github.com/google/leveldb
[3]: https://github.com/pepe/mansion

## Usage

### TLDR

Just open [test suite](test/suite0.janet). If you ever used any key value store 
and know a little about Janet, you will be at home.

### Basic database facilities

#### Opening the database

`(tahani/open db-name &opt opts)` opens the LevelDB database. `db-name` must
be the `string` same as the directory name, where database resides on the disk.
Optional `opts` have only one defined value `:eie` for setting `error_if_exists`
LevelDB option. Function returns Janet AbstractType `tahani/db`. This
AbstractType instance is used as a parameter when calling most of the API.

Panics if any LevelDB error occurs.

#### Closing the database

`(tahani/close db)` closes the LevelDB database. `db` must be an instance
of `tahani/db` AbstractType returned from the `tahani/open` function mentioned
above.

Panics if any LevelDB error occurs.

#### Putting to the database

`(tahani/record/put db key value)` puts value under the key into the database.
`db` must be an instance of `tahani/db` AbstractType returned from the
`tahani/open` function mentioned above. `key` and `value` must be `string` and
can contain `\0` characters. Returns nil on success.

Panics if any LevelDB error occurs.

You can call his function as a method on database AbstractType
`(:put db key value)`.

#### Getting from the database

`(tahani/record/get db key)` gets value under the key from the database.
`db` must be an instance of `tahani/db` AbstractType returned from the
`tahani/open` function mentioned above. `key` must be `string` and can contain
`\0` characters. Returns `string` value from the database.

Panics if any LevelDB error occurs.

You can call his function as a method on database AbstractType
`(:get db key)`.

#### Deleting from the database

`(tahani/record/delete db key)` deletes value under the key from the database.
`key` must be `string` and can contain `\0` characters. Returns `nil` on
success

Panics if any LevelDB error occurs.

You can call his function as a method on database AbstractType
`(:delete db key)`.

### Database management facilities

#### Repairing the database

`(tahani/manage/repair db-name)` repairs the database. `db-name` must
be the `string` same as the directory name, where the database resides on the
disk.
The database cannot be open.

Panics if any LevelDB error occurs.

#### Destroying the database

`(tahani/manage/destroy db-name)` destroys the database. `db-name` must
be the `string` same as the directory name, where the database resides on the
disk.

The database cannot be open.

Panics if any LevelDB error occurs.

THIS FUNCTION IS VERY DANGEROUS AS YOU WILL LOSE EVERYTHING!

### Batch facilities

LevelDB batches are the way for issuing multiple commands to the database, which
it transacts as one atomic change. They are similar to relation database
transactions.

#### Creating the batch

`(tahani/batch/create)` creates a new batch. It does not have any parameters as
batch initialization is database independent. Returns Janet AbstractType `tahani/batch`.

#### Adding put into the batch

`(tahani/batch/put key value)` adds put values under the key into the batch.
`key`
and `value` must be `string` and can contain `\0` characters. Returns the
`tahani/batch` on success so that it can be easily chained.

You can call his function as a method on `tahani/batch` AbstractType
`(:put batch key value)`.

#### Adding delete into the batch

`(tahani/batch/delete key)` adds a delete command with the key into the batch.
`key` must be
`string` and can contain `\0` characters. Returns the `tahani/batch` on success
so that it can be easily chained.

You can call his function as a method on `tahani/batch` AbstractType
`(:delete batch key)`.

#### Writing batch to the database

`(tahani/batch/write batch db)` writes the batch to the database. `db` must be
an instance of `tahani/db` AbstractType returned from the `tahani/open` function
mentioned above. Returns the `tahani/batch` on success so that it can be easily
chained.

Panics if any LevelDB error occurs.

You can call his function as a method on `tahani/batch` AbstractType
`(:write batch key)`.

#### Destroying the batch

`(tahani/batch/destroy batch)` destroys the batch. `batch` must be an instance
of
`tahani/batch` returned from `create` function, mentioned above.

### Snapshot facilities

Snapshot is the temporary content of the database in time. It can be used
instead of
the db for `get` function mentioned above, or for the iterator mentioned
bellow.

#### Creating the snapshot

`(tahani/snapshot/create db)` creates new snapshot of the database. `db` must be
instance of `tahani/db` AbstractType returned from the `tahani/open` function
mentioned above. Returns `tahani/snapshot` Janet AbstractType.

#### Releasing the snapshot

`(tahani/snapshot/release snapshot)` releases the snapshot. `snapshot` must be
instance of `tahani/snapshot` AbstractType.

You can call his function as a method on `tahani/iterator` AbstractType
`(:release batch)`.

### Iterator facilities

Iterators are the primary tool for traversing snapshot of the database.

#### Creating the iterator

`(tahani/iterator/create db &opt snapshot)` creates the iterator. `db` must be
an instance of `tahani/db` AbstractType returned from the `tahani/open` function
mentioned above. Optional `snapshot` must be an instance of `tahani/snapshot`
AbstractType, when you do not provide one, an implicit snapshot is created.
Returns
`tahani/snapshot` Janet AbstractType.

#### Checking the iterator validity

`(tahani/iterator/valid? iterator)` checks if the iterator is in the valid
state. `iterator` must be an instance of `tahani/iterator` returned by the
`create` method mentioned above. Returns true when the iterator is in the valid
state.

You can call his function as a method on `tahani/iterator` AbstractType
`(:valid? iterator)`

#### Getting current iterator position key

`(tahani/iterator/key iterator)` returns the key of the current iterator
position. `iterator` must be an instance of `tahani/iterator` returned by the
`create` method mentioned above. Returns `string` with the key of the current
iterator record.

You can call his function as a method on `tahani/iterator` AbstractType
`(:key iterator)`

#### Getting current iterator position value

`(tahani/iterator/value iterator)` returns the value of the current iterator
position. `iterator` must be an instance of `tahani/iterator` returned by the
`create` method mentioned above. Returns `string` with the value of the current
iterator record.

You can call his function as a method on `tahani/iterator` AbstractType
`(:value iterator)`

#### Moving to the next record in the iterator

`(tahani/iterator/next iterator)` moves the current position in the iterator to
the next record. `iterator` must be an instance of `tahani/iterator` returned by
the `create` method mentioned above. Returns `iterator` so it can be easily
chained.

You can call his function as a method on `tahani/iterator` AbstractType
`(:next iterator)`

#### Seeking the first record

`(tahani/iterator/seek-first iterator)` moves the current position in the
iterator to the first one.`iterator` must be an instance of `tahani/iterator`
returned by the `create` method mentioned above. Returns `iterator` so it can
be easily chained.

You can call his function as a method on `tahani/iterator` AbstractType
`(:seek-first iterator)`

#### Seeking last record

`(tahani/iterator/seek-last iterator)` moves the current position in the
iterator to the last record. `iterator` must be an instance of `tahani/iterator`
returned by the `create` method mentioned above. Returns `iterator` so it can
be easily chained.

You can call his function as a method on `tahani/iterator` AbstractType
`(:seek-last iterator)`

#### Seeking record

`(tahani/iterator/seek iterator key)` moves the current position in the
iterator to the record with `key`. `iterator` must be an instance of
`tahani/iterator` returned by the `create` method mentioned above. `key`must be
a string.  Returns `iterator` so it can be easily chained.

You can call his function as a method on `tahani/iterator` AbstractType
`(:seek iterator key)`

## TODOs

- [  ] @todo add open, read and write optional options
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
