// Gives the cdylib an explicit SONAME. Without it the C++ executable records the
// (relative) path used on the link line as its dependency, and then only starts when
// the current directory happens to match. With a SONAME the dynamic loader finds
// libffi_rust.so through the RUNPATH that CMake sets for the build tree.
fn main() {
    if std::env::var("CARGO_CFG_TARGET_OS").as_deref() == Ok("linux") {
        println!("cargo:rustc-cdylib-link-arg=-Wl,-soname,libffi_rust.so");
    }
    println!("cargo:rerun-if-changed=build.rs");
}
