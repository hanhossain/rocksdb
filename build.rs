use cmake::Config;

fn main() {
    println!("cargo:rerun-if-changed=rocksdb-cxx");
    println!("cargo:rerun-if-changed=src/lib.rs");

    let dst = Config::new("rocksdb-cxx")
        .define("WITH_GFLAGS", "OFF")
        .generator("Ninja")
        .build_target("rocksdb")
        .build();

    cxx_build::bridge("src/lib.rs")
        .include("rocksdb-cxx/include")
        .flag_if_supported("-std=c++17")
        .compile("rocksdb-cxx-cxx");

    println!("cargo:rustc-link-search=native={}/build", dst.display());
    println!("cargo:rustc-link-lib=static=rocksdb");
}
