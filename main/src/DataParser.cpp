/**
 * @file DataParser.cpp
 * @brief Implementation of the DataParser class.
 * 
 * This file provides the logic to parse command messages, determine command types,
 * and execute them via the CommandHandler interface. It also includes the parser task,
 * which handles incoming messages from the MessageHandler's data parser queue.
 * 
 * The class supports forwarding via the "CMD OTHER ..." format and performs command
 * execution or rerouting depending on the command type and origin.
 * 
 * @author meths1
 * @date 02.04.2025
 */

#include "DataParser.hpp"
#include "esp_log.h"
#include <algorithm> // For std::transform

static const char* TAG = "DataParser";

DataParser::DataParser(CommandHandler& commandHandler) : _commandHandler(commandHandler) {
}

CommandHandler::Command DataParser::parse(std::string& data) const {
    // Case-insensitive check for "CMD" prefix
    if (data.size() >= 3 && 
        std::equal(data.begin(), data.begin() + 3, "CMD", [](char a, char b) {
        return std::toupper(a) == b;
    })) {
        
        std::string rest = data.substr(3);
        rest.erase(0, rest.find_first_not_of(" \t"));  // Trim leading whitespace

        // Get the first word (keyword)
        size_t keywordEnd = rest.find_first_of(" \t\r\n");
        std::string keyword = rest.substr(0, keywordEnd);
        std::string remaining = (keywordEnd != std::string::npos) ? rest.substr(keywordEnd) : "";

        // Convert keyword to uppercase for comparison
        std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::toupper);

        // Handle forwarding via "OTHER"
        if (keyword == "OTHER") {
            data = "CMD" + remaining;
            return CommandHandler::Command::OTHER;
        }

        // Match known commands
        using Cmd = CommandHandler::Command;
        if (keyword == "CHIP_INFO")               return Cmd::CHIP_INFO;
        else if (keyword == "IDF_VERSION")        return Cmd::IDF_VERSION;
        else if (keyword == "FREE_HEAP")          return Cmd::FREE_HEAP;
        else if (keyword == "FREE_INTERNAL_HEAP") return Cmd::FREE_INTERNAL_HEAP;
        else if (keyword == "FREE_MIN_HEAP")      return Cmd::FREE_MIN_HEAP;
        else if (keyword == "CLOCK_SPEED")        return Cmd::CLOCK_SPEED;
        else if (keyword == "RESET")              return Cmd::RESET;

        // Unknown keyword
        return Cmd::UNKNOWN;
    }

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
    MessageHandler::ParserMessageID messageID;
    std::string cmdResponse;
    MessageHandler::Message responseMessage;
    // Clear the message buffer
    std::fill(message.begin(), message.end(), '\0');
    // Wait for a message from the DATA_PARSER_QUEUE
    if (msgHandler->receive(MessageHandler::QueueType::DATA_PARSER_QUEUE, message, &messageID)) {
        std::string data(message.data());
        CommandHandler::Command command = parse(data);
        // If a command is found, execute it
        if (command != CommandHandler::Command::NO_COMMAND && command != CommandHandler::Command::OTHER) {
            // Execute the command and get the response
            cmdResponse.clear();
            executeCommand(command, cmdResponse);
            // Send the command response to the UART or BLE queue
            std::fill(responseMessage.begin(), responseMessage.end(), '\0');
            std::copy(cmdResponse.begin(), cmdResponse.end(), responseMessage.begin());
            switch (messageID) {
                case MessageHandler::ParserMessageID::MSG_ID_UART:
                    msgHandler->send(MessageHandler::QueueType::UART_QUEUE, responseMessage);
                    break;
                case MessageHandler::ParserMessageID::MSG_ID_BLE:
                    msgHandler->send(MessageHandler::QueueType::BLE_QUEUE, responseMessage);
                    break;
                default:
                    ESP_LOGW(TAG, "Unknown message ID");
                    return;
            }
        } 
        else if (command == CommandHandler::Command::OTHER) {
            // Forward the parsed data to the BLE queue
            std::fill(responseMessage.begin(), responseMessage.end(), '\0');
            std::copy(data.begin(), data.end(), responseMessage.begin());
            msgHandler->send(MessageHandler::QueueType::BLE_QUEUE, responseMessage);            
        }
        else {
            // Forward the message to the UART or BLE queue
            switch (messageID) {
                case MessageHandler::ParserMessageID::MSG_ID_UART:
                    msgHandler->send(MessageHandler::QueueType::BLE_QUEUE, message);
                    break;
                case MessageHandler::ParserMessageID::MSG_ID_BLE:
                    msgHandler->send(MessageHandler::QueueType::UART_QUEUE, message);
                    break;
                default:
                    ESP_LOGW(TAG, "Unknown message ID");
                    return;
            }
        }
    } else {
        ESP_LOGW(TAG, "Failed to receive message from DATA_PARSER_QUEUE");
    }
}