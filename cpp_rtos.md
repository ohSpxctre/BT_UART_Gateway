# Why `pthread` is used instead of the C++ FreeRTOS wrapper

## ⚠️ Background

This project initially used a C++ FreeRTOS task wrapper (e.g., `rtos::Task::build().spawn(...)`) to create tasks using modern C++ lambda syntax. While this approach is elegant and expressive, it caused a **runtime crash (Guru Meditation Error)** on the **ESP32-C6** running **ESP-IDF v5.5-dev**.

## ❌ The Problem

Using a lambda-based task with `ESP_LOGI()` or other logging early in task execution caused the device to crash with:

```
abort() was called at PC 0x40801a27 → lock_acquire_generic()
```

This happened deep inside:
- `ESP_LOGI(...)`
- `vfprintf_r()`
- `newlib` locking functions

The root cause appears to be a **newlib lock acquisition failure** triggered by:
- Logging in a task created with a C++ lambda (likely before locks are fully initialized)
- Or a bug/regression in `newlib` + C++ support on the ESP32-C6 platform

This issue **does not occur** when tasks are created using:
- Plain C (`xTaskCreate`)
- Or `pthread_create` (Espressif's POSIX-compatible FreeRTOS wrapper)

---

## ✅ The Fix: Use `pthread` for task creation

To ensure stability and avoid runtime lock failures, this project now uses **`pthread`** instead of the C++ task wrapper:

```cpp
pthread_t thread;
pthread_create(&thread, nullptr, my_task_function, nullptr);
```

This guarantees:
- Clean FreeRTOS integration
- Proper `newlib` locking behavior
- Compatibility with `ESP_LOGx()` logging macros
- Avoids subtle C++ runtime init issues

---

## 📌 Summary

| Approach              | Result       |
|-----------------------|--------------|
| `rtos::Task::spawn()` | ❌ Crashes on ESP32-C6 (logging or early C++ init issues) |
| `xTaskCreate`         | ✅ Stable     |
| `pthread_create`      | ✅ Stable and POSIX-style task creation |

---

## 📚 Resources

- [ESP-IDF pthread docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/cplusplus.html)
- [ESP Logging Guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/log.html)

