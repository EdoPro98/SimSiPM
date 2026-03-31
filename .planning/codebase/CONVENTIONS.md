# Coding Conventions

**Analysis Date:** 2026-04-01

## Naming Patterns

**Namespaces:**
- Single namespace `sipm` used throughout the codebase
- Class names use PascalCase: `SiPMProperties`, `SiPMSensor`, `SiPMAnalogSignal`, `SiPMRandom`
- Free functions use lowercase with descriptive names: `integral()`, `peak()`, `tot()`, `toa()`

**Files:**
- Source files: `<Name>.cpp` in `src/` directory
- Header files: `<Name>.h` in `include/` directory
- Test files: `<module>.cpp` in `tests/` directory
- Python bindings: `<ClassName>Py.cpp` in `python/` directory
- Benchmarks: `sensor.cpp`, `random.cpp` in `bench/` directory

**Functions:**
- Getter methods use `()` suffix: `size()`, `pitch()`, `nCells()`, `dcr()`
- Setter methods use `set` prefix: `setSize()`, `setDcr()`, `setProperty()`
- Const methods include `const` qualifier
- Inline functions in header files for performance

**Variables:**
- Member variables use `m_` prefix: `m_Size`, `m_Pitch`, `m_Hits`
- Local variables use descriptive names
- Enum values use `k` prefix: `kUniform`, `kCircle`, `kGaussian`

**Types:**
- Use `uint32_t` for counters and indices
- Use `double` for time values in ns
- Use `float` for signal waveform data
- Use `std::vector` for collections
- Prefer `std::pair` for hit position coordinates

## Code Style

**Formatting:**
- Tool: `.clang-format` configuration
- Indentation: 2 spaces
- Column limit: 120 characters
- Brace style: K&R style (`{` attached to declaration)
- Trailing commas: None
- Include blocks: Preserved order
- Tab width: 8 (in .clang-format, but source uses spaces)

**Key clang-format settings:**
- `AccessModifierOffset: -2` (indent private/protected after opening brace)
- `Cpp11BracedListStyle: true`
- `AlwaysBreakTemplateDeclarations: true`
- `BreakBeforeTernaryOperators: true`
- `PointerAlignment: Left`

**Linting:**
- No dedicated linting tool configured
- Relies on clang-format for formatting consistency
- GCC compiler warnings enabled via `-ffast-math` optimization flag

## Import Organization

**Order:**
1. Class/namespace headers (same project)
2. Standard library headers
3. Third-party headers (gtest, pybind11)

**Path Aliases:**
- No path aliases used
- Direct include paths with `<project/Header.h>` or `"project/Header.h"`
- Standard library uses `<>` brackets
- Project headers use `""` quotes

## Error Handling

**Patterns:**
- Return-based error handling (no exceptions)
- Use `-1` for invalid/error returns from signal feature methods
- Check boolean flags for disabled features (e.g., `hasDcr()`, `hasXt()`)
- Use `continue` in loops to skip invalid inputs
- Error messages via `std::cerr` with descriptive text

**Validation:**
- Range checks on input values
- Early return for empty/invalid inputs
- Bounds checking with `isInSensor()` helper

## Logging

**Framework:**
- Console output only (no external logging library)

**Patterns:**
- Use `std::cout` for debug output and class printing
- Use `std::cerr` for error messages
- Output includes memory address via `std::hex << std::addressof(obj)`
- Fixed precision (2 decimal places) for floating-point output

## Comments

**When to Comment:**
- Class documentation at top of header files with `@class`, `@brief`, `@author`, `@date`
- Method documentation with `@brief` for complex functions
- Parameter documentation with `@param`
- Return documentation with `@return` (not commonly used)
- TODO/FIXME comments not detected in codebase

**JSDoc/TSDoc:**
- Uses doxygen-style comments with `/** @class ... */` and `@brief` tags
- Not consistently applied to all functions
- Example:
  ```cpp
  /**
  * Integral of the signal defined as the sum of all samples in the integration
  * window normalized for the sampling time.
  * @param intstart   Starting time of integration in ns
  * @param intgate    Length of the integration gate
  * @param threshold  Process only if above the threshold
  */
  double SiPMAnalogSignal::integral(...) const;
  ```

**Header File Structure:**
- Doxygen-style header at top with class description
- Class declaration with inline member documentation
- Free function implementations with documentation
- Operator overloads with documentation

## Function Design

**Size:**
- Functions are typically concise (1-50 lines)
- Long functions (like `addPhotoelectrons()`) are split by PDE type
- Helper functions extracted for complex logic

**Parameters:**
- Prefer value parameters over pointers/references when const
- Use `noexcept` for functions that cannot throw
- Use `constexpr` for compile-time constants and simple getters

**Return Values:**
- Return by value for small objects
- Return by const reference for vectors (`const std::vector<float>&`)
- Return `void` for setters
- Return error indicator (`-1`) for invalid results

## Module Design

**Exports:**
- Single header per module in `include/`
- Implementation in corresponding `.cpp` files in `src/`
- `SiPM.h` includes all other headers as convenience

**Barrel Files:**
- No barrel files (`.hpp`) used
- Direct include of specific headers preferred
- `SiPM.h` acts as top-level convenience header

## Output Streams

**Pattern:**
- All classes implement `operator<<` for easy printing
- Output formatted with `std::setprecision(2)` and `std::fixed`
- Memory addresses included for debugging
- Example:
  ```cpp
  std::ostream& operator<<(std::ostream& out, const SiPMAnalogSignal& obj) {
    out << std::setprecision(2) << std::fixed;
    out << "===> SiPM Analog Signal <===\n";
    out << "Signal length is: " << std::dec << obj.m_Waveform.size() / obj.m_Sampling << " ns\n";
    return out;
  }
  ```

## Memory Management

**Smart Pointers:**
- Not used in current codebase
- Manual `new` and `delete` for `SiPMHit` objects
- `resetState()` method handles cleanup by deleting hits

**RAII:**
- Not consistently used
- Objects typically stack-allocated or stored in vectors

---

*Convention analysis: 2026-04-01*
