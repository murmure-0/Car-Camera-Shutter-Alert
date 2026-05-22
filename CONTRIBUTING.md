# Contributing to Car-Camera-Shutter-Alert

Thank you for your interest in contributing! This guide covers how to set up the project, coding conventions, and the pull request process.

## Quick Links

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Workflow](#development-workflow)
- [Coding Standards](#coding-standards)
- [Commit Messages](#commit-messages)
- [Pull Request Process](#pull-request-process)
- [Reporting Issues](#reporting-issues)

## Code of Conduct

Be respectful and constructive. We aim to foster an inclusive, welcoming community for everyone regardless of background.

## Getting Started

### Prerequisites

| Component | Version | Notes |
|-----------|---------|-------|
| Qt | 5.15+ | Cross-platform UI framework |
| CMake / qmake | 3.16+ / 3.1+ | Build system |
| GCC / Clang | C++17 capable | Compiler |
| OpenCV | 4.x | Image processing |
| RKNN SDK | 2.x | RV1106 AI inference |
| STM32CubeIDE | 1.12+ | STM32 firmware (optional) |

### Clone & Build

```bash
git clone https://github.com/<your-username>/Car-Camera-Shutter-Alert.git
cd Car-Camera-Shutter-Alert

# Qt UI application
cd qt_car_ui
qmake qt_car_ui.pro
make -j$(nproc)

# STM32 firmware (requires STM32CubeIDE)
# Open rv1106_sensor/ in STM32CubeIDE
```

## Development Workflow

1. **Fork** the repository
2. **Create a feature branch** from `main`:
   ```bash
   git checkout -b feature/your-feature-name
   ```
3. **Make changes** and commit with clear messages
4. **Test** your changes thoroughly
5. **Push** to your fork and open a Pull Request

### Branch Naming

| Prefix | Purpose | Example |
|--------|---------|---------|
| `feature/` | New functionality | `feature/gesture-override` |
| `fix/` | Bug fixes | `fix/serial-reconnect` |
| `refactor/` | Code restructuring | `refactor/camera-worker` |
| `docs/` | Documentation | `docs/api-reference` |

## Coding Standards

### C++ (Qt Application)

- **Standard**: C++17
- **Naming**:
  - Classes: `PascalCase` (`CameraPage`, `BaiduGestureClient`)
  - Methods: `camelCase` (`setRecording`, `processGestureResult`)
  - Member variables: `m_` prefix (`m_cameraPage`, `m_recording`)
  - Local variables: `camelCase` (`detectedGesture`, `isConnected`)
  - Constants: `UPPER_SNAKE_CASE` or `kPascalCase`
  - Qt signals: `camelCase` (`gestureDetected`)
  - Qt slots: `camelCase` (`onButtonClicked`)
- **Formatting**: 4-space indentation, no tabs
- **Comments**: Use English; avoid redundant comments that merely restate the code. Document intent, not mechanics.
- **Error messages**: Prefix with context tag in brackets, e.g., `[Gesture]`, `[OAuth]`, `[Serial]`
- **QString literals**: Use `QStringLiteral()` for compile-time optimization
- **Memory**: Prefer stack allocation and Qt parent-child ownership over raw `new`

### C (STM32 Firmware)

- **Standard**: C11 with FreeRTOS conventions
- **Naming**:
  - Functions: `snake_case` with module prefix (`serial_send_data`, `sensor_read_temp`)
  - Macros: `UPPER_SNAKE_CASE` (`PROTOCOL_HEADER`, `MAX_RETRIES`)
  - Types: `PascalCase_t` (`SensorData_t`, `FatigueState_t`)
- **Formatting**: 4-space indentation, braces on same line
- **Safety**: Use `static` for file-scoped functions/variables; validate all HAL return values

### Serial Protocol

When modifying the serial communication protocol:

1. Update [PROTOCOL.md](rv1106_sensor/PROTOCOL.md) with the new message format
2. Maintain backward compatibility when possible
3. Use JSON format: `{"type":"<category>","data":{...}}\n`
4. Keep message types lowercase (`gesture`, `fatigue`, `network`, `sensor`)

## Commit Messages

Follow the [Conventional Commits](https://www.conventionalcommits.org/) specification:

```
<type>(<scope>): <description>

[optional body]

[optional footer]
```

**Types**: `feat`, `fix`, `refactor`, `docs`, `style`, `test`, `chore`, `perf`

**Examples**:
```
feat(camera): add gesture cooldown to prevent rapid re-triggering
fix(serial): resolve buffer overflow on high-frequency sensor data
refactor(gesture): extract gesture mapping into configurable table
docs(readme): add hardware wiring diagram for STM32 sensor board
```

## Pull Request Process

1. **One PR per concern** — keep changes focused and reviewable
2. **Update documentation** if your change affects behavior or APIs
3. **Test on target hardware** when possible (RV1106 + STM32)
4. **Ensure clean build** with no compiler warnings
5. **Fill out the PR template** completely:
   - What does this change do?
   - Why is it needed?
   - How was it tested?
   - Any breaking changes?

A maintainer will review your PR. Please be responsive to feedback.

## Reporting Issues

When filing a bug report, please include:

- **Hardware**: Board model (RV1106 variant, STM32L431, etc.)
- **Software**: OS, Qt version, compiler version
- **Steps to reproduce**: Minimal, specific steps
- **Expected vs. actual behavior**
- **Logs**: Relevant console output or serial logs

For feature requests, describe the use case and expected benefit.

## License

By contributing, you agree that your contributions will be licensed under the [MIT License](LICENSE).
