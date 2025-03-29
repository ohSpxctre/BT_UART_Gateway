/**
 * @file MessageHandler.hpp
 * @brief Header file for the MessageHandler class.
 * 
 * This file contains the declaration of the MessageHandler class, which is
 * responsible for managing message queues for different communication types,
 * such as UART, BLE, and data parsing.
 * 
 * The MessageHandler class provides methods to send and receive messages
 * from specific queues, as well as to set and retrieve an externally created
 * UART event queue. It uses the FreeRTOS Cpp wrapper for message queues.
 * 
 * @note This class is designed to work with the ESP-IDF framework and FreeRTOS.
 * 
 * @author meths1
 * @date 29.03.2025
 */

#pragma once

#include <memory>
#include "rtos.h"

/**
 * @class MessageHandler
 * @brief A class to handle message queues for different communication types.
 */
class MessageHandler {
public:
    /* Default size of the message queue. */
    static constexpr size_t DefaultQueueSize = 10;

    /* Maximum length of a message. */
    static constexpr size_t MaxMessageLength = 256;

    /**
     * @enum QueueType
     * @brief Enum representing the types of queues managed by the MessageHandler.
     */
    enum class QueueType {
        UART_QUEUE,         /**< Queue for UART communication. */
        BLE_QUEUE,          /**< Queue for BLE communication. */
        DATA_PARSER_QUEUE   /**< Queue for data parser communication. */
    };

    /**
     * @typedef Message
     * @brief Alias for a fixed-size array representing a message.
     */
    using Message = std::array<char, MaxMessageLength>;

    /**
     * @brief Constructor for the MessageHandler class.
     * @param queueSize The size of the message queues. Defaults to DefaultQueueSize.
     */
    MessageHandler(size_t queueSize = DefaultQueueSize);

    /**
     * @brief Sends a message to the specified queue.
     * @param queueType The type of queue to send the message to.
     * @param message The message to send.
     * @return True if the message was successfully sent, false otherwise.
     */
    bool send(QueueType queueType, const Message& message);

    /**
     * @brief Receives a message from the specified queue.
     * @param queueType The type of queue to receive the message from.
     * @param message Reference to store the received message.
     * @return True if a message was successfully received, false otherwise.
     */
    bool receive(QueueType queueType, Message& message);

    /**
     * @brief Sets an externally created UART event queue.
     * @param queue Pointer to the externally created UART event queue.
     *              The MessageHandler does not take ownership of this queue.
     */
    void setUartEventQueue(QueueHandle_t queue) {
        _uartEventQueue = queue;
    }

    /**
     * @brief Gets the UART event queue.
     * @return Pointer to the UART event queue.
     */
    QueueHandle_t* getUartEventQueue() {
        return &_uartEventQueue;
    }

private:
    /* Shared pointers to message queues for different communication types. */
    std::shared_ptr<rtos::MessageQueue<Message>> _uartQueue;
    std::shared_ptr<rtos::MessageQueue<Message>> _bleQueue;
    std::shared_ptr<rtos::MessageQueue<Message>> _dataParserQueue;

    /* Pointer to the externally created UART event queue. */
    QueueHandle_t _uartEventQueue = nullptr;  // Not owned

    /**
     * @brief Retrieves the message queue corresponding to the specified queue type.
     * @param queueType The type of queue to retrieve.
     * @return Reference to the shared pointer of the requested message queue.
     */
    std::shared_ptr<rtos::MessageQueue<Message>>& getQueue(QueueType queueType);
};
