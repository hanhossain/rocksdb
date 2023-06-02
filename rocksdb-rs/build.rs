use cmake;
use cmake::Config;

fn main() {
    let mut dst = Config::new("../rocksdb")
        .generator("Ninja")
        .build_target("rocksdb")
        .very_verbose(true)
        .build();
    dst.push("build");

    println!("cargo:rerun-if-changed=../rocksdb");
    println!("cargo:rustc-link-search=native={}", dst.display());
    println!("cargo:rustc-link-lib=static=rocksdb");
}
