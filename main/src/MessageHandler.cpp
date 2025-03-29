#include "MessageHandler.hpp"
#include "esp_log.h"

static const char* TAG = "MessageHandler";

MessageHandler::MessageHandler(size_t queueSize) {
    _uartQueue = std::make_shared<rtos::MessageQueue<Message>>(queueSize);
    _bleQueue = std::make_shared<rtos::MessageQueue<Message>>(queueSize);
    _dataParserQueue = std::make_shared<rtos::MessageQueue<Message>>(queueSize);
}

bool MessageHandler::send(QueueType queueType, const Message& message) {
    auto& queue = getQueue(queueType);
    if (!queue) {
        ESP_LOGW(TAG, "Send failed: queue not initialized");
        return false;
    }
    return queue->send(message);
}

bool MessageHandler::receive(QueueType queueType, Message& message) {
    auto& queue = getQueue(queueType);
    if (!queue) {
        ESP_LOGW(TAG, "Receive failed: queue not initialized");
        return false;
    }
    return queue->receive(message);
}

std::shared_ptr<rtos::MessageQueue<MessageHandler::Message>>&
MessageHandler::getQueue(QueueType queueType) {
    switch (queueType) {
        case QueueType::UART_QUEUE: return _uartQueue;
        case QueueType::BLE_QUEUE: return _bleQueue;
        case QueueType::DATA_PARSER_QUEUE: return _dataParserQueue;
        default:
            ESP_LOGE(TAG, "Invalid queue type");
            static std::shared_ptr<rtos::MessageQueue<Message>> nullQueue = nullptr;
            return nullQueue;
    }
}

