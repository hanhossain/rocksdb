use cmake::Config;

fn main() {
    let dst = Config::new("rocksdb-cxx")
        .generator("Ninja")
        .build_target("rocksdb")
        .build();

    println!("cargo:rustc-link-search=native={}/build", dst.display());
    println!("cargo:rustc-link-lib=static=rocksdb");
}
