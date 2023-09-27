use rocksdb_rs::db::DB;
use rocksdb_rs::options::Options;

const DB_PATH: &str = "/tmp/rocksdb_column_families_example";

fn main() {
    {
        // open DB
        let mut options = Options::default();
        options.as_db_options().set_create_if_missing(true);
        let mut db = DB::open(&options, DB_PATH).unwrap();

        // create column family
        let cf_handle = db
            .create_column_family(options.as_column_family_options(), "new_cf")
            .unwrap();

        // close DB
        db.destroy_column_family_handle(cf_handle).unwrap();
    }
}
