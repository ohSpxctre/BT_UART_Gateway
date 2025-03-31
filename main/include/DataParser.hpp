#pragma once

#include "CommandHandler.hpp"
#include "MessageHandler.hpp"

class DataParser {
public:
    /**
     * @brief Constructor for the DataParser class.
     * 
     * @param commandHandler Reference to a CommandHandler instance.
     */
    explicit DataParser(CommandHandler& commandHandler);

    /**
     * @brief Destructor for the DataParser class.
     */
    ~DataParser() = default;

    /**
     * @brief Parses the received data and extracts the command.
     * 
     * @param data The received data.
     * @return The extracted command.
     */
    CommandHandler::Command parse(const std::string& data) const;

    /**
     * @brief Parses the received data and executes the command.
     * 
     * @param command The command to execute.
     * @param data The string return data of the command.
     * @return True if a command was executed 
     */
    bool executeCommand(CommandHandler::Command command, std::string& data);

    /**
     * @brief Data parser task function.
     * 
     * This function runs in a separate thread and continuously receives data from the data parser queue.
     * It parses the data and executes or sends the message to the UART or BLE queue.
     *
     * @param dataParser Pointer to the DataParser instance.
     */
    void dataParserTask(MessageHandler* msgHandler);

private:
    CommandHandler& _commandHandler; // Aggregation with CommandHandler
};
