fn main() {
    let dst = cmake::Config::new(".")
        .define("MPL_WGPU_BUILD_EXAMPLES", "OFF")
        .define("MPL_WGPU_BUILD_TESTS", "OFF")
        // .define("CMAKE_CXX_STANDARD", "17") // Usually set in CMakeLists
        .build();

    println!("cargo:rustc-link-search=native={}/lib", dst.display());
    // We need to link the library created by CMake
    // CMakeLists writes 'mpl_wgpu_backend'.
    // BUT we added c_api.cpp. We need to modify CMakeLists.txt to include c_api.cpp OR build it here.
    
    // Wait, if I use cmake crate, I rely on CMakeLists.txt.
    // I should add c_api.cpp to CMakeLists.txt first!
    
    println!("cargo:rustc-link-lib=static=mpl_wgpu_backend");
    println!("cargo:rustc-link-lib=static=matplot");
    
    // C++ std lib
    #[cfg(target_os = "linux")]
    println!("cargo:rustc-link-lib=dylib=stdc++");
    #[cfg(target_os = "macos")]
    println!("cargo:rustc-link-lib=dylib=c++");

    println!("cargo:rerun-if-changed=src/c_api.cpp");
    println!("cargo:rerun-if-changed=src/c_api.h");
    println!("cargo:rerun-if-changed=src/backend/wgpu_backend.cc");
    println!("cargo:rerun-if-changed=include/matplot/backend/wgpu_backend.h");
    println!("cargo:rerun-if-changed=CMakeLists.txt");
}
