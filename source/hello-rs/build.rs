use std::{env, path::PathBuf};

fn main() {
    tonic_build::configure()
        .compile(&["proto/helloworld.proto"], &["proto"])
        .unwrap();
}
