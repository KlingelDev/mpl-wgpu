# mpl-wgpu: wgpu Rendering Backend for matplotplusplus

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

GPU-accelerated rendering backend for [matplotplusplus](https://github.com/alandefreitas/matplotplusplus) using wgpu.

## Features

- 🚀 **GPU-Accelerated** - Hardware-accelerated rendering via wgpu
- 🦀 **Rust Integration** - Safe Rust bindings to matplotplusplus
- 🎨 **Full matplot++ API** - Access all 100+ plot types
- 💻 **Cross-platform** - Windows, Linux, macOS, WASM
- ⚡ **High Performance** - Batched rendering, optimized for large datasets

## Architecture

```
matplotplusplus (C++)     ← Plotting library (all plot types)
        ↓
  WgpuBackend              ← This project: GPU backend
        ↓
    WgpuRenderer           ← Abstract interface
        ↓
  Your Implementation      ← PrimitiveRenderer, TextRenderer
```

**Components:**
1. **C++ Backend** - Implements matplot++ `backend_interface`
2. **C FFI Layer** - C API for language bindings
3. **Rust Bindings** - Safe, idiomatic Rust wrapper

## Project Structure

```
mpl-wgpu/
├── include/matplot/backend/  # Public C++ headers
├── src/backend/              # C++ implementation  
├── src/ffi/                  # C FFI and Rust bindings
├── examples/
│   ├── cpp/                  # C++ examples
│   └── rust/                 # Rust examples
├── vendor/matplotplusplus/   # Git submodule
└── tests/                    # Unit tests
```

## Building

### C++ Backend

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Rust Bindings

```bash
cargo build --release
```

## Quick Start (C++)

```cpp
#include <matplot/backend/wgpu_backend.h>
#include <matplot/matplot.h>

// Implement WgpuRenderer for your GPU setup
class MyRenderer : public matplot::backend::WgpuRenderer {
  // ... implement draw methods ...
};

int main() {
  auto renderer = std::make_shared<MyRenderer>();
  auto backend = std::make_shared<matplot::backend::WgpuBackend>(
    renderer
  );
  
  auto fig = matplot::figure();
  fig->backend(backend);
  
  matplot::plot({1, 2, 3}, {1, 4, 9});
  fig->draw();
}
```

## Quick Start (Rust)

```rust
use mpl_wgpu::{Figure, WgpuBackend};

fn main() {
  let mut fig = Figure::new();
  fig.plot(&[1.0, 2.0, 3.0], &[1.0, 4.0, 9.0]);
  fig.render();
}
```

## Documentation

- [API Documentation](docs/api.md)
- [Architecture Details](docs/architecture.md)
- [Contributing Guide](CONTRIBUTING.md)
- [Roadmap](ROADMAP.md)

## Design for Upstream

This backend is designed to be contributed upstream to matplotplusplus:
- Minimal dependencies (matplot++, webgpu.hpp)
- Abstract renderer interface
- Well-documented, tested code
- Google C++ style guide compliant

## License

MIT License - see [LICENSE](LICENSE) for details

## Acknowledgments

- [matplotplusplus](https://github.com/alandefreitas/matplotplusplus) - 
  Plotting library
- [wgpu](https://wgpu.rs/) - GPU API implementation
- Based on work from OTC.SDK.Visualization