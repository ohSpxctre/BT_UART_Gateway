idf_component_register(SRCS "src/Mutex.cpp"
                            "src/rtos.cpp"
                            "src/Semaphore.cpp"
                            "src/Task.cpp"
                            "src/Timer.cpp"
                            "src/util/New.cpp"
                            "src/util/CoreStub.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "freertos")
