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

