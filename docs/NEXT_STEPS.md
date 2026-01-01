# mpl-wgpu: Next Steps Guide

## Current Status (15 commits)

**Completed Infrastructure:**
- ✅ C++ WgpuBackend from OTC.SDK (822 lines)
- ✅ C++ PrimitiveRenderer (443 lines)
- ✅ WGSL shader with file loading
- ✅ Window scaffolding (C++ + Rust)
- ✅ Build system configured

## Critical Next Steps

### 1. Install wgpu-native

**Option A: System Package** (recommended)
```bash
# Check if wgpu-native is available
find /usr -name "wgpu.h" 2>/dev/null
```

**Option B: Build from Source**
```bash
git clone https://github.com/gfx-rs/wgpu-native
cd wgpu-native
make package
# Copy headers and libs to system
```

**Option C: Using Dawn (Google's WebGPU)**
```bash
# Dawn is more mature but heavier
git clone https://dawn.googlesource.com/dawn
```

### 2. Update CMakeLists.txt

Once wgpu is installed:
```cmake
# Find wgpu
find_library(WGPU_LIB wgpu_native REQUIRED)
find_path(WGPU_INCLUDE wgpu.h REQUIRED)

target_include_directories(mpl_wgpu_backend
    PRIVATE ${WGPU_INCLUDE}
)

target_link_libraries(mpl_wgpu_backend
    PRIVATE ${WGPU_LIB}
)
```

### 3. Update MinimalRenderer

Replace software fallback with PrimitiveRenderer:

```cpp
// examples/cpp/minimal_renderer.h
#include "src/backend/primitive_renderer.h"

class MinimalRenderer : public matplot::backend::WgpuRenderer {
 private:
  std::unique_ptr<mpl_wgpu::PrimitiveRenderer> primitives_;
  wgpu::Device device_;
  wgpu::Queue queue_;
};
```

### 4. Test Build

```bash
mkdir build && cd build
cmake ..
cmake --build .

# Run example
./examples/cpp/simple_plot
```

## Integration Checklist

- [ ] Install wgpu-native or Dawn
- [ ] Update CMakeLists.txt with wgpu paths
- [ ] Update MinimalRenderer implementation
- [ ] Verify shader loading works
- [ ] Test rectangle rendering
- [ ] Test line rendering  
- [ ] Test matplot++ plot rendering

## Known Issues to Address

1. **Shader Loading Path**: Currently expects `src/backend/primitives.wgsl`
   - May need adjustment for build directory
   - Consider embedding shader or using install paths

2. **wgpu Initialization**: Need to create device/queue
   - Can reference Rust example in `examples/rust/simple_plot.rs`
   - Or use GLFW + wgpu integration

3. **Coordinate Transform**: May need adjustment for matplot++ coords
   - Currently assumes identity matrix
   - matplot++ might provide different coordinate system

## Files Ready for Integration

**C++ Backend:**
- `include/matplot/backend/wgpu_backend.h` - Interface
- `src/backend/wgpu_backend.cc` - Implementation
- `src/backend/primitive_renderer.h` - GPU primitives
- `src/backend/primitive_renderer.cc` - Rendering
- `src/backend/primitives.wgsl` - Shader

**Examples:**
- `examples/cpp/simple_plot.cc` - Window + matplot++
- `examples/rust/simple_plot.rs` - Rust window + wgpu

## Upstream Contribution Plan

Once tested and working:

1. **Prepare PR** to matplotplusplus:
   - Clean up TODOs
   - Add comprehensive tests
   - Document wgpu dependency

2. **Files to Contribute:**
   - `wgpu_backend.h/cc`
   - `primitive_renderer.h/cc`
   - `primitives.wgsl`
   - Examples

3. **Documentation Needed:**
   - Build instructions
   - wgpu installation guide
   - API documentation
   - Performance benchmarks

## Contact / Support

For wgpu-native help:
- GitHub: https://github.com/gfx-rs/wgpu-native
- Docs: https://wgpu.rs/

For matplotplusplus:
- GitHub: https://github.com/alandefreitas/matplotplusplus
