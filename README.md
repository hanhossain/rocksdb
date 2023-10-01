# Migrating RocksDB to Rust

The goal of this is to migrate RocksDB to pure Rust. Mostly, it's just used to figure out how to transition an existing codebase from C++ to Rust.

# Building

## Rust
The rust code will automatically build the C++ code as a static library (see [build.rs](./build.rs)).
```zsh
cargo build 
```

## C++
If you want to build the C++ code separately, use:
```zsh
mkdir build
cd build
cmake -GNinja ../rocksdb-cxx
```

If you want to build C++ without tests:
```zsh
mkdir build
cd build
cmake -GNinja -DWITH_TESTS=OFF ../rocksdb-cxx
```

# Environment Setup
If you're using rust-analyzer, you may need to set the following settings to keep RA from locking the `target` directory while running `cargo check`. 
```json
{
    ...
    "rust-analyzer.server.extraEnv": {
            "CARGO_TARGET_DIR": "target/analyzer"
        },
    "rust-analyzer.check.extraArgs": [
        "--target-dir=target/analyzer"
    ]
    ...
}
```
