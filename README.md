# tahani

Janet wrapper to leveldb C API

## Usage

The basic `open`, `close`, `put` and `get` operations are inplemented. 

The management `repair` and `destroy` operations are implemented.

The batch `create`, `destroy`, `write` operations are implemented.

The batch `put` and `delete` operations are implemented.


You can run test with `janet test.janet`

## TODOs

- [ ] @todo make batches chainable
- [ ] @todo make put get to DB abstract
- [ ] @todo add open, read and write optional options
- [ ] @todo add iterator functions
- [ ] @todo add marshaling/unmarshaling store module
- [x] add batch functions
- [x] split to more modules
- [x] add delete function
- [x] add management functions
- [x] add error checks

