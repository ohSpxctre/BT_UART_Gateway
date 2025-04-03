/**
 * @file CommandHandler.hpp
 * @brief Declaration of the CommandHandler class, which provides methods to process commands
 *        and retrieve system information such as chip details, IDF version, and heap statistics.
 * 
 * This file defines the CommandHandler class, which is responsible for handling commands
 * and providing various system-related information. It includes methods to process commands
 * and retrieve details like chip information, IDF version, and heap memory statistics.
 * 
 * Usage:
 * - Create an instance of the CommandHandler class.
 * - Use the `processCommand` method to handle commands.
 * - Use the getter methods to retrieve system information.
 * 
 * @note This class is designed to work with the ESP-IDF framework and FreeRTOS.
 * 
 * @author meths1
 * @date 31.03.2025
 */

#pragma once

#include <string>

class CommandHandler {
public:
    /**
     * @brief Constructor for the CommandHandler class.
     */
    CommandHandler() = default;

    /**
     * @brief Destructor for the CommandHandler class.
     */
    ~CommandHandler() = default;

    /**
     * @brief Enum representing supported commands.
     */
    enum class Command {
        CHIP_INFO,
        IDF_VERSION,
        FREE_HEAP,
        FREE_INTERNAL_HEAP,
        FREE_MIN_HEAP,
        CLOCK_SPEED,
        RESET,
        UNKNOWN,
        OTHER,
        NO_COMMAND
    };
    
    /**
     * @brief Processes a command.
     * 
     * Possible commands:
     * - CHIP_INFO: Retrieves chip information.
     * - IDF_VERSION: Retrieves the IDF version.
     * - FREE_HEAP: Retrieves the free heap size.
     * - FREE_INTERNAL_HEAP: Retrieves the free internal heap size.
     * - FREE_MIN_HEAP: Retrieves the minimum free heap size.
     * - CLOCK_SPEED: Retrieves the clock speed in MHz.
     * - RESET: Restarts the system.
     * - UNKNOWN: Represents an unknown or unsupported command.
     * 
     * @param command The command to process.
     */
    std::string processCommand(Command command);

private:
    /** 
     * @brief Gets the chip information.
     * @return A string containing the chip information. (Model, features, revision, cores)
     */
    std::string getChipInfo() const;

    /**
     * @brief Gets the IDF version.
     * @return A string containing the IDF version.
     */
    std::string getIdfVersion() const;

    /**
     * @brief Gets the free heap size.
     * @return A string containing the free heap size.
     */
    std::string getFreeHeap() const;

    /**
     * @brief Gets the free internal heap size.
     * @return A string containing the free internal heap size.
     */
    std::string getFreeInternalHeap() const;

    /**
     * @brief Gets the free minimum heap size.
     * @return A string containing the minimum heap that has ever been available.
     */
    std::string getFreeMinHeap() const;

    /**
     * @brief Gets the clock speed in MHz.
     * @return A string containing the clock speed in MHz.
     */
    std::string getClockSpeed() const;
};