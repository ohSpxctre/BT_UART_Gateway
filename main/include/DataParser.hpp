/**
 * @file DataParser.hpp
 * @brief Declaration of the DataParser class for parsing and executing commands.
 * 
 * The DataParser class is responsible for analyzing incoming messages,
 * detecting known commands prefixed by "CMD", executing them through the
 * CommandHandler interface, or forwarding them appropriately to another system.
 * 
 * Usage:
 * - Construct with a reference to a CommandHandler instance.
 * - Call `parse()` to extract and identify commands.
 * - Use `executeCommand()` to handle valid commands.
 * - Run `dataParserTask()` inside a FreeRTOS task to continuously process messages.
 * 
 * Command structure:
 * - Recognized commands: prefixed with "CMD", e.g., "CMD CHIP_INFO"
 * - Forwarding: "CMD OTHER <command>" is forwarded as "CMD <command>"
 * 
 * @note Designed for use with ESP-IDF and FreeRTOS.
 * 
 * @author meths1
 * @date 02.04.2025
 */

#pragma once

#include "CommandHandler.hpp"
#include "MessageHandler.hpp"

class DataParser {
public:
    /**
     * @brief Constructs a DataParser instance.
     * 
     * @param commandHandler Reference to an external CommandHandler used for command execution.
     */
    explicit DataParser(CommandHandler& commandHandler);

    /**
     * @brief Default destructor.
     */
    ~DataParser() = default;

    /**
     * @brief Parses the input data string to detect and classify command keywords.
     * 
     * The function checks if the string starts with the prefix "CMD" (case-insensitive).
     * - If followed by a known command (e.g. "RESET"), it returns the corresponding Command enum.
     * - If followed by the keyword "OTHER", it strips "OTHER" and rewrites the input as a new CMD message
     *   (e.g. "CMD OTHER something" → "CMD something") and returns NO_COMMAND to indicate forwarding.
     * - If followed by an unknown command, it returns UNKNOWN.
     * - If the message does not start with "CMD", it returns NO_COMMAND.
     * 
     * @param[in,out] data The incoming message to be parsed. Modified if forwarding is detected.
     * @return CommandHandler::Command The extracted command classification.
     */
    CommandHandler::Command parse(std::string& data) const;

    /**
     * @brief Executes the given command and stores the result in the output string.
     * 
     * @param command The command to execute.
     * @param[out] data String to be populated with the response.
     * @return true if the command was executed; false if it was NO_COMMAND.
     */
    bool executeCommand(CommandHandler::Command command, std::string& data);

    /**
     * @brief Runs a continuous task that receives and processes messages from the parser queue.
     * 
     * This function is designed to be executed as a FreeRTOS task. It continuously waits for messages
     * on the `DATA_PARSER_QUEUE`, parses each incoming message, and either:
     * - Executes the command and returns the result to the source (UART/BLE), or
     * - Forwards unrecognized "OTHER" messages to the opposite interface.
     * 
     * @param msgHandler Pointer to the shared MessageHandler instance.
     */
    void dataParserTask(MessageHandler* msgHandler);

private:
    CommandHandler& _commandHandler; // Aggregation with CommandHandler
};
