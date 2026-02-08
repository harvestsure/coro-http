# CI/CD 和测试指南

## 一、GitHub Actions CI 工作流

### 工作流特性

#### 1. **多编译器支持**
- **Linux**: GCC 11, Clang 14
- **macOS**: Apple Clang (最新)
- **Windows**: MSVC 2022

#### 2. **Sanitizer 构建 (ASAN + UBSAN)**
- 独立的 `sanitizer-build` job
- 自动检测：
  - 内存泄漏
  - 使用后释放 (use-after-free)
  - 未定义行为 (undefined behavior)
  - 缓冲区溢出

#### 3. **自动化测试**
- 编译后自动运行所有测试用例
- 超时控制 (30 秒/测试)
- 并行编译 (4 核)

#### 4. **示例运行验证**
- 对每个示例程序设置 5 秒超时
- 测试：coro, https, keepalive, proxy, retry, sse

### 工作流配置

**触发条件**：
```yaml
on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main, develop ]
```

**矩阵构建**：
```
- OS: ubuntu-latest, macos-latest, windows-latest
- Compiler: gcc, clang, msvc
```

## 二、本地构建和测试

### 1. 启用测试编译

```bash
cd build
cmake .. -DBUILD_TESTS=ON -DENABLE_SANITIZER=ON
cmake --build . --parallel 4
```

### 2. 运行所有测试

```bash
ctest --output-on-failure --timeout 30
```

### 3. 运行特定测试

```bash
# 只运行超时相关测试
ctest -R timeout --output-on-failure

# 只运行连接池测试
ctest -R connection_pool --output-on-failure

# 详细输出
ctest --verbose
```

### 4. 带 Sanitizer 的测试

```bash
# ASAN - 检测内存问题
export ASAN_OPTIONS=detect_leaks=1:abort_on_error=1

# UBSAN - 检测未定义行为
export UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1

ctest --output-on-failure
```

### 5. 禁用 Sanitizer（如需要）

```bash
cmake .. -DENABLE_SANITIZER=OFF
```

## 三、核心测试用例

### 1. **超时和取消 (test_timeout.cpp)**

```cpp
场景                    意义
├─ Basic timeout       检查超时是否正确取消协程
├─ Timeout + Retry     验证重试机制与超时的交互
├─ Concurrent timeout  多个请求独立超时处理
└─ Promise release     验证 promise 在取消时被正确释放
```

**关键点**：
- Promise 必须在 cancel 时释放
- 使用 ASAN 检测 use-after-free
- 无内存泄漏

### 2. **重定向处理 (test_redirect.cpp)**

```cpp
场景                      意义
├─ Single redirect       301/302 重定向
├─ Redirect chain        多次连续重定向 (3+ hops)
├─ Redirect + auth       重定向时保持认证状态
├─ Concurrent redirects  并发请求不同的重定向链
└─ Loop detection        检测重定向循环
```

**关键点**（协程特有）：
- 多次 co_await 必须保持协程状态
- Promise 在多个 suspend/resume 点存活
- 本地变量在所有 co_await 跨越时保持有效

### 3. **连接池重用 (test_connection_pool.cpp)**

```cpp
场景                          意义
├─ Connection reuse         10 个请求重用 ≤5 个连接
├─ Concurrent pool access   20 个并发请求，队列调度
├─ Stale connection         检测并移除死连接
├─ Pool exhaustion          资源不足时等待
├─ Different hosts          不同 host 使用独立池
├─ No stagnation            资源最终释放 (用 ASAN/Valgrind 验证)
└─ Exception releases       异常时也要释放连接
```

**关键点**：
- 每个协程完成后必须释放连接
- 即使异常也要清理
- 防止连接泄漏导致资源枯竭

### 4. **错误处理 (test_error_handling.cpp)**

```cpp
场景                          意义
├─ Network error           无法连接的正确处理
├─ Timeout exception       定时器到期时异常
├─ TLS error              SSL/TLS 握手失败
├─ Invalid URL            参数验证失败
├─ Partial response       连接中断的处理
├─ Concurrent errors      错误不影响其他请求
├─ Error recovery         重试策略
├─ Exception in handler   用户代码异常时清理
└─ Memory limit           超大响应的内存保护
```

**关键点**：
- 所有错误路径都要正确清理资源
- No RAII 违规
- 异常安全保证

## 四、Sanitizer 报告解读

### ASAN 报告示例

```
=================================================================
==1234==ERROR: LeakSanitizer: detected memory leaks

...

SUMMARY: AddressSanitizer: 1024 byte(s) leaked in 1 allocation(s).
```

**解决方案**：
- Check promise 析构函数
- 验证所有分配都有对应的 delete
- 特别关注异常路径

### UBSAN 报告示例

```
runtime error: index 5 out of bounds for type 'int [5]'
```

**解决方案**：
- 检查数组/容器访问边界
- 验证指针运算
- 检查整数溢出

## 五、性能检查

### 运行带计时的测试

```bash
time ctest --output-on-failure
```

### 内存使用检查（macOS）

```bash
/usr/bin/time -v ./build/test_connection_pool
```

### 检测文件描述符泄漏（macOS）

```bash
lsof -p $$  # 获取进程号
# 运行测试
lsof -p $$  # 比较前后
```

## 六、CI 失败调试

### 本地复现 Linux 环境

```bash
# 使用 Docker
docker run -it --rm -v $(pwd):/work ubuntu:22.04 bash
apt-get update && apt-get install -y \
  build-essential cmake libssl-dev zlib1g-dev clang-14
cd /work && mkdir build_docker && cd build_docker
cmake .. -DENABLE_SANITIZER=ON -DBUILD_TESTS=ON
cmake --build .
ctest --output-on-failure
```

### 获取详细日志

```bash
# CMake 配置日志
cmake .. -DENABLE_SANITIZER=ON --debug-output 2>&1 | tee cmake.log

# 编译详细日志
cmake --build . --verbose 2>&1 | tee build.log

# 测试详细日志
ctest --verbose 2>&1 | tee test.log
```

## 七、添加新测试

### 步骤

1. **创建测试文件**：`tests/test_feature.cpp`

2. **在 CMakeLists.txt 中注册**：
```cmake
add_executable(test_feature tests/test_feature.cpp)
target_link_libraries(test_feature PRIVATE coro_http)
add_test(NAME feature COMMAND test_feature TIMEOUT 30)
```

3. **编译并测试**：
```bash
cmake --build .
ctest -R feature --output-on-failure
```

### 最佳实践

- ✅ 每个测试 ≤ 30 秒完成
- ✅ 独立的测试用例，可单独运行
- ✅ 明确的成功/失败条件
- ✅ 详细的注释说明测试意义
- ✅ 最小化外部依赖（如网络调用）

## 八、故障排除

### 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| ASAN 超时 | Sanitizer 开销大 | 增加超时或禁用 ASAN |
| Windows 编译失败 | MSVC 不支持 `-fsanitize` | Win 自动跳过 ASAN |
| 网络超时 | 测试依赖网络 | 使用本地 mock 服务 |
| 文件权限 | 脚本权限不足 | `chmod +x scripts/*.sh` |

## 九、集成建议

### 分支保护规则

在 GitHub repository settings 中：
```
Require status checks to pass before merging:
  ✓ CI / build-and-test (ubuntu-latest)
  ✓ CI / sanitizer-build
  ✓ code-quality
```

### Badge（在 README 中添加）

```markdown
![CI](https://github.com/.../workflows/CI/badge.svg)
```

---

**最后提醒**：Sanitizer 对于协程库特别关键，因为 promise 生命周期错误会导致很难调试的 bug。建议在 CI 中始终启用。
