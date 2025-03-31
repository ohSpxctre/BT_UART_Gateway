#include "DataParser.hpp"
#include "esp_log.h"
#include <algorithm> // For std::transform

DataParser::DataParser(CommandHandler& commandHandler) : _commandHandler(commandHandler) {
}

CommandHandler::Command DataParser::parse(const std::string& data) const {
    // Return command if the message has the keyword CMD or cmd at the beginning
    if (data.find("CMD") == 0 || data.find("cmd") == 0) {
        std::string commandStr = data.substr(3); // Skip "CMD" or "cmd"
        commandStr.erase(std::remove_if(commandStr.begin(), commandStr.end(), ::isspace), commandStr.end()); // Remove spaces
        // Convert to uppercase for case-insensitive comparison
        std::transform(commandStr.begin(), commandStr.end(), commandStr.begin(), ::toupper); // Convert to uppercase
        if (commandStr == "CHIP_INFO") {
            return CommandHandler::Command::CHIP_INFO;
        } else if (commandStr == "IDF_VERSION") {
            return CommandHandler::Command::IDF_VERSION;
        } else if (commandStr == "FREE_HEAP") {
            return CommandHandler::Command::FREE_HEAP;
        } else if (commandStr == "FREE_INTERNAL_HEAP") {
            return CommandHandler::Command::FREE_INTERNAL_HEAP;
        } else if (commandStr == "FREE_MIN_HEAP") {
            return CommandHandler::Command::FREE_MIN_HEAP;
        } else if (commandStr == "CLOCK_SPEED") {
            return CommandHandler::Command::CLOCK_SPEED;
        } else if (commandStr == "RESET") {
            return CommandHandler::Command::RESET;
        }
        else {
            return CommandHandler::Command::UNKNOWN;
        }
    }
    // If no command is found
    return CommandHandler::Command::NO_COMMAND;
}

bool DataParser::executeCommand(CommandHandler::Command command, std::string& data) {
    // If a command is found, process it
    if (command != CommandHandler::Command::NO_COMMAND) {
        data = _commandHandler.processCommand(command);
        return true;
    }
    // If no command is found, return false
    return false;
}

void DataParser::dataParserTask(MessageHandler* msgHandler) {
    MessageHandler::Message message;
    if (msgHandler->receive(MessageHandler::QueueType::DATA_PARSER_QUEUE, message)) {
        std::string data(message.data());
        CommandHandler::Command command = parse(data);
        std::string cmdResponse;
        if (executeCommand(command, cmdResponse)) {
            // Send the command response to the UART or BLE queue
            MessageHandler::Message responseMessage;
            std::copy(cmdResponse.begin(), cmdResponse.end(), responseMessage.begin());
            msgHandler->send(MessageHandler::QueueType::UART_QUEUE, responseMessage);
        } else {
            // Forward the message to the UART or BLE queue
            msgHandler->send(MessageHandler::QueueType::UART_QUEUE, message);
        }
    } else {
        ESP_LOGW("DataParser", "Failed to receive message from DATA_PARSER_QUEUE");
    }
}