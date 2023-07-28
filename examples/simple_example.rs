use rocksdb_rs::db::DB;
use rocksdb_rs::options::{ColumnFamilyOptions, DBOptions, Options, ReadOptions, WriteOptions};

const DB_PATH: &str = "/tmp/rocksdb_simple_example";

fn main() {
    let mut db_options = DBOptions::default();

    // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
    db_options.increase_parallelism(16);

    // create the DB if it's not already present
    db_options.set_create_if_missing(true);

    let mut column_family_options = ColumnFamilyOptions::default();
    column_family_options.optimize_level_style_compaction(512 * 1024 * 1024);
    let options = Options::new(&db_options, &column_family_options);

    // open DB
    let mut db = DB::open(&options, DB_PATH).unwrap();

    // Put key-value
    let write_options = WriteOptions::default();
    db.put(&write_options, "key1", "value").unwrap();

    // get value
    let read_options = ReadOptions::default();
    let value = db.get(&read_options, "key1").unwrap();
    assert_eq!(value, "value");

    //   // atomically apply a set of updates
    //   {
    //     WriteBatch batch;
    //     batch.Delete("key1");
    //     batch.Put("key2", value);
    //     s = db->Write(WriteOptions(), &batch);
    //   }
    //
    //   s = db->Get(ReadOptions(), "key1", &value);
    //   assert(s.IsNotFound());
    //
    //   db->Get(ReadOptions(), "key2", &value);
    //   assert(value == "value");
    //
    //   {
    //     PinnableSlice pinnable_val;
    //     db->Get(ReadOptions(), db->DefaultColumnFamily(), "key2", &pinnable_val);
    //     assert(pinnable_val == "value");
    //   }
    //
    //   {
    //     std::string string_val;
    //     // If it cannot pin the value, it copies the value to its internal buffer.
    //     // The intenral buffer could be set during construction.
    //     PinnableSlice pinnable_val(&string_val);
    //     db->Get(ReadOptions(), db->DefaultColumnFamily(), "key2", &pinnable_val);
    //     assert(pinnable_val == "value");
    //     // If the value is not pinned, the internal buffer must have the value.
    //     assert(pinnable_val.IsPinned() || string_val == "value");
    //   }
    //
    //   PinnableSlice pinnable_val;
    //   s = db->Get(ReadOptions(), db->DefaultColumnFamily(), "key1", &pinnable_val);
    //   assert(s.IsNotFound());
    //   // Reset PinnableSlice after each use and before each reuse
    //   pinnable_val.Reset();
    //   db->Get(ReadOptions(), db->DefaultColumnFamily(), "key2", &pinnable_val);
    //   assert(pinnable_val == "value");
    //   pinnable_val.Reset();
    //   // The Slice pointed by pinnable_val is not valid after this point
    //
    //   delete db;
    //
    //   return 0;
}
