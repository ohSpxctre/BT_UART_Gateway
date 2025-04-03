/**
 * @file MessageHandler.cpp
 * @brief Implementation of the MessageHandler class.
 * 
 * This file contains the logic for managing multiple categorized FreeRTOS queues.
 * It enables sending and receiving messages tagged by source and destination type,
 * and handles routing between UART, BLE, and the parser system.
 * 
 * For the DATA_PARSER_QUEUE, a special message structure is used to include an ID
 * indicating the message's origin, allowing for bi-directional routing after parsing.
 * 
 * @author meths1
 * @date 01.04.2025
 */

#include "MessageHandler.hpp"
#include "esp_log.h"

static const char* TAG = "MessageHandler";

MessageHandler::MessageHandler(size_t queueSize) {
    _uartQueue = std::make_unique<rtos::MessageQueue<Message>>(queueSize);
    _bleQueue = std::make_unique<rtos::MessageQueue<Message>>(queueSize);
    _dataParserQueue = std::make_unique<rtos::MessageQueue<ParserMessage>>(queueSize);
}

bool MessageHandler::send(QueueType queueType, const Message& message, ParserMessageID id) {
    if (queueType == QueueType::DATA_PARSER_QUEUE) {
        if (!_dataParserQueue) {
            ESP_LOGW(TAG, "Send failed: parser queue not initialized");
            return false;
        }
        ParserMessage parserMessage{ id, message };
        return _dataParserQueue->send(parserMessage);
    }

    auto& queue = getQueue(queueType);
    if (!queue) {
        ESP_LOGW(TAG, "Send failed: queue not initialized");
        return false;
    }

    return queue->send(message);
}


bool MessageHandler::receive(QueueType queueType, Message& message, ParserMessageID* id) {
    // Clear the message buffer
    std::fill(message.begin(), message.end(), '\0');
    if (queueType == QueueType::DATA_PARSER_QUEUE) {
        if (!_dataParserQueue) {
            ESP_LOGW(TAG, "Receive failed: parser queue not initialized");
            return false;
        }
        ParserMessage parserMessage;
        if (_dataParserQueue->receive(parserMessage)) {
            message = parserMessage.message;
            if (id) {
                *id = parserMessage.id;
            }
            else {
                ESP_LOGD(TAG, "ParserMessageID not provided, ignoring");
            }
            return true;
        }
        return false;
    }

    auto& queue = getQueue(queueType);
    if (!queue) {
        ESP_LOGW(TAG, "Receive failed: queue not initialized");
        return false;
    }
    return queue->receive(message);
}

std::unique_ptr<rtos::MessageQueue<MessageHandler::Message>>&
MessageHandler::getQueue(QueueType queueType) {
    switch (queueType) {
        case QueueType::UART_QUEUE: return _uartQueue;
        case QueueType::BLE_QUEUE: return _bleQueue;
        default:
            ESP_LOGE(TAG, "Invalid queue type");
            static std::unique_ptr<rtos::MessageQueue<Message>> nullQueue = nullptr;
            return nullQueue;
    }
}

