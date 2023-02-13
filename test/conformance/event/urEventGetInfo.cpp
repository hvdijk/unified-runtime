// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: MIT

#include "fixtures.h"

/**
 * Test fixture that sets up an event with the following properties:
 * - Type: UR_COMMAND_MEM_BUFFER_WRITE
 * - Execution Status: UR_EVENT_STATUS_COMPLETE
 * - Reference Count: 1
 */
template <class T> struct urEventTestWithParam : uur::urQueueTestWithParam<T> {

    void SetUp() override {
        UUR_RETURN_ON_FATAL_FAILURE(uur::urQueueTestWithParam<T>::SetUp());
        ASSERT_SUCCESS(urMemBufferCreate(this->context, UR_MEM_FLAG_WRITE_ONLY,
                                         size, nullptr, &buffer));

        input.assign(count, 42);
        ASSERT_SUCCESS(urEnqueueMemBufferWrite(this->queue, buffer, false, 0,
                                               size, input.data(), 0, nullptr,
                                               &event));
        ASSERT_SUCCESS(urEventWait(1, &event));
    }

    void TearDown() override {
        if (buffer) {
            EXPECT_SUCCESS(urMemRelease(buffer));
        }
        if (event) {
            EXPECT_SUCCESS(urEventRelease(event));
        }
        uur::urQueueTestWithParam<T>::TearDown();
    }

    const size_t count = 1024;
    const size_t size = sizeof(uint32_t) * count;
    ur_mem_handle_t buffer = nullptr;
    ur_event_handle_t event = nullptr;
    std::vector<uint32_t> input;
};

using urEventGetInfoTest = urEventTestWithParam<ur_event_info_t>;

TEST_P(urEventGetInfoTest, Success) {

    ur_event_info_t info_type = getParam();
    size_t size;
    ASSERT_SUCCESS(urEventGetInfo(event, info_type, 0, nullptr, &size));
    ASSERT_NE(size, 0);
    std::vector<uint8_t> data(size);
    ASSERT_SUCCESS(
        urEventGetInfo(event, info_type, size, data.data(), nullptr));

    switch (info_type) {
    case UR_EVENT_INFO_COMMAND_QUEUE: {
        auto returned_queue = reinterpret_cast<ur_queue_handle_t>(data.data());
        ASSERT_EQ(queue, returned_queue);
        break;
    }
    case UR_EVENT_INFO_CONTEXT: {
        auto returned_context =
            reinterpret_cast<ur_context_handle_t>(data.data());
        ASSERT_EQ(context, returned_context);
        break;
    }
    case UR_EVENT_INFO_COMMAND_TYPE: {
        auto returned_command = reinterpret_cast<ur_command_t *>(data.data());
        ASSERT_EQ(UR_COMMAND_MEM_BUFFER_WRITE, *returned_command);
        break;
    }
    case UR_EVENT_INFO_COMMAND_EXECUTION_STATUS: {
        auto returned_status =
            reinterpret_cast<ur_event_status_t *>(data.data());
        ASSERT_EQ(UR_EVENT_STATUS_COMPLETE, *returned_status);
        break;
    }
    case UR_EVENT_INFO_REFERENCE_COUNT: {
        auto returned_reference_count =
            reinterpret_cast<uint32_t *>(data.data());
        ASSERT_EQ(1, *returned_reference_count);
        break;
    }
    default:
        FAIL() << "Invalid event info enumeration";
    }
}

UUR_TEST_SUITE_P(urEventGetInfoTest,
                 ::testing::Values(UR_EVENT_INFO_COMMAND_QUEUE,
                                   UR_EVENT_INFO_CONTEXT,
                                   UR_EVENT_INFO_COMMAND_TYPE,
                                   UR_EVENT_INFO_COMMAND_EXECUTION_STATUS,
                                   UR_EVENT_INFO_REFERENCE_COUNT),
                 uur::deviceTestWithParamPrinter<ur_event_info_t>);

using urEventGetInfoNegativeTest = uur::event::urEventTest;

TEST_P(urEventGetInfoNegativeTest, InvalidNullHandle) {
    ur_event_info_t info_type = UR_EVENT_INFO_COMMAND_QUEUE;
    size_t size;
    ASSERT_SUCCESS(urEventGetInfo(event, info_type, 0, nullptr, &size));
    ASSERT_NE(size, 0);
    std::vector<uint8_t> data(size);

    /* Invalid hEvent */
    ASSERT_EQ(
        urEventGetInfo(nullptr, UR_EVENT_INFO_COMMAND_QUEUE, 0, nullptr, &size),
        UR_RESULT_ERROR_INVALID_NULL_HANDLE);
}

TEST_P(urEventGetInfoNegativeTest, InvalidEnumeration) {
    size_t size;
    ASSERT_EQ(
        urEventGetInfo(event, UR_EVENT_INFO_FORCE_UINT32, 0, nullptr, &size),
        UR_RESULT_ERROR_INVALID_ENUMERATION);
}

TEST_P(urEventGetInfoNegativeTest, InvalidValue) {
    ur_event_info_t info_type = UR_EVENT_INFO_COMMAND_QUEUE;
    size_t size;
    ASSERT_SUCCESS(urEventGetInfo(event, info_type, 0, nullptr, &size));
    ASSERT_NE(size, 0);
    std::vector<uint8_t> data(size);

    /* Invalid propValueSize */
    ASSERT_EQ(urEventGetInfo(event, UR_EVENT_INFO_COMMAND_QUEUE, 0, data.data(),
                             nullptr),
              UR_RESULT_ERROR_INVALID_VALUE);
}

UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urEventGetInfoNegativeTest);