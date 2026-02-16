# Contributing to Live Face Deep Fake

Thank you for your interest in contributing! This document provides guidelines and instructions.

## Code of Conduct

This project adheres to ethical standards. All contributions must:
- Respect privacy and consent
- Follow legal guidelines
- Not facilitate harmful or fraudulent activities
- Include proper documentation

## How to Contribute

### 1. Report Issues

Found a bug? Please open an issue with:
- Clear title and description
- Steps to reproduce
- Expected vs. actual behavior
- Your environment details (OS, OpenCV version, etc.)
- Any error logs or screenshots

### 2. Feature Requests

Have an idea? Create an issue with:
- Clear description of the feature
- Use cases and benefits
- Any potential implementation notes

### 3. Submit Code Changes

#### Setup Development Environment

```bash
git clone https://github.com/yourusername/live-face-deep-fake.git
cd live-face-deep-fake
git checkout -b feature/your-feature-name
```

#### Code Style Guidelines

- **C++**: Follow Google C++ Style Guide
- **Naming**: 
  - Classes: `PascalCase` (e.g., `FaceSwapper`)
  - Functions/methods: `camelCase` (e.g., `detectFaces()`)
  - Variables: `camelCase` (e.g., `frameCount`)
  - Constants: `UPPER_CASE` (e.g., `MAX_FACES`)

- **Comments**: Use clear, descriptive comments
  ```cpp
  // Detect faces in current frame using YuNet
  std::vector<cv::Rect> faces = detector->detect(frame);
  ```

- **Error Handling**: Always check for empty/invalid data
  ```cpp
  if (mat.empty()) {
      std::cerr << "Error: Invalid matrix" << std::endl;
      return;
  }
  ```

#### Commit Messages

Follow conventional commits:
```
feat: add GPU acceleration support
fix: resolve face detection crash on empty frames
docs: update installation guide
refactor: improve INSwapper conversion pipeline
test: add unit tests for embedding extraction
```

#### Testing

Before submitting:
```bash
# Build with debug symbols
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Test with various inputs
./LiveFaceSwapper --arcface ../models/arcface.onnx --inswapper ../models/inswapper_128.onnx
```

### 4. Pull Request Process

1. **Fork the repository** on GitHub
2. **Create a feature branch**:
   ```bash
   git checkout -b feature/amazing-feature
   ```

3. **Commit your changes** with clear messages:
   ```bash
   git add .
   git commit -m "feat: add amazing feature"
   ```

4. **Push to your fork**:
   ```bash
   git push origin feature/amazing-feature
   ```

5. **Create a Pull Request** with:
   - Clear title describing changes
   - Description of what was changed and why
   - Reference to any related issues (#123)
   - Screenshots if UI changes
   - Performance impact details

6. **Address review feedback** promptly

### 5. Documentation Contributions

Improvements to documentation are always welcome!

- **README.md**: Installation, quick start, overview
- **ADVANCED_PIPELINE.md**: Technical architecture details
- **docs/**: Additional tutorials and guides

## Development Areas

### High Priority

- [ ] GPU acceleration (CUDA support)
- [ ] Performance optimization (target 30 FPS)
- [ ] Multi-face swapping
- [ ] Video file batch processing

### Medium Priority

- [ ] Web UI interface
- [ ] Docker containerization
- [ ] Unit test coverage
- [ ] Memory optimization

### Low Priority

- [ ] Alternative model support
- [ ] Mobile app version
- [ ] REST API

## Testing Your Changes

### Local Testing

```bash
# Build project
mkdir build && cd build
cmake ..
make -j$(nproc)

# Test with virtual camera
./LiveFaceSwapper --arcface ../models/arcface.onnx --inswapper ../models/inswapper_128.onnx

# Monitor performance
top -p $(pgrep LiveFaceSwapper)
```

### Performance Benchmarking

```cpp
// Add timing code to measure performance
auto start = std::chrono::high_resolution_clock::now();
// ... function to measure ...
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
std::cout << "Function took " << duration.count() << " ms" << std::endl;
```

## Getting Help

- **Documentation**: Check [docs/](./docs/) folder
- **Issues**: Search existing issues first
- **Discussions**: Start a discussion for general questions
- **Email**: Contact through GitHub profile

## Recognition

Contributors will be recognized in:
- `CONTRIBUTORS.md` file
- Release notes
- GitHub contributors page

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

---

Thank you for making Live Face Deep Fake better! 🎉
