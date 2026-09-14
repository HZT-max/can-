#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <canard.h>
#include <uavcan.tunnel.Targetted.h>

static struct uavcan_tunnel_Targetted received_message;
static uint32_t received_count;

/* 主机测试回调模拟拓展器收到飞控发来的完整Targetted传输。 */
static void on_received(CanardInstance *ins, CanardRxTransfer *transfer)
{
    (void)ins;
    assert(transfer->data_type_id == UAVCAN_TUNNEL_TARGETTED_ID);
    assert(!uavcan_tunnel_Targetted_decode(transfer, &received_message));
    received_count++;
}

static bool should_accept(const CanardInstance *ins,
                          uint64_t *signature,
                          uint16_t data_type_id,
                          CanardTransferType transfer_type,
                          uint8_t source_node_id)
{
    (void)ins;
    (void)source_node_id;
    if ((transfer_type == CanardTransferTypeBroadcast) &&
        (data_type_id == UAVCAN_TUNNEL_TARGETTED_ID)) {
        *signature = UAVCAN_TUNNEL_TARGETTED_SIGNATURE;
        return true;
    }
    return false;
}

int main(void)
{
    CanardInstance sender;
    CanardInstance receiver;
    uint8_t sender_pool[4096];
    uint8_t receiver_pool[4096];
    uint8_t transfer_id = 0U;

    canardInit(&sender, sender_pool, sizeof(sender_pool), NULL, NULL, NULL);
    canardInit(&receiver, receiver_pool, sizeof(receiver_pool), on_received, should_accept, NULL);
    canardSetLocalNodeID(&sender, 10U);
    canardSetLocalNodeID(&receiver, 125U);

    struct uavcan_tunnel_Targetted sent_message;
    memset(&sent_message, 0, sizeof(sent_message));
    sent_message.protocol.protocol = UAVCAN_TUNNEL_PROTOCOL_GPS_GENERIC;
    sent_message.target_node = 125U;
    sent_message.serial_id = 3;
    sent_message.options = UAVCAN_TUNNEL_TARGETTED_OPTION_LOCK_PORT;
    sent_message.baudrate = 115200U;
    sent_message.buffer.len = 120U;
    for (uint8_t i = 0U; i < sent_message.buffer.len; i++) {
        sent_message.buffer.data[i] = (uint8_t)(i ^ 0xA5U);
    }

    uint8_t payload[UAVCAN_TUNNEL_TARGETTED_MAX_SIZE];
    const uint16_t payload_length =
        (uint16_t)uavcan_tunnel_Targetted_encode(&sent_message, payload, true);
    const int16_t queued_frames = canardBroadcast(&sender,
                                                   UAVCAN_TUNNEL_TARGETTED_SIGNATURE,
                                                   UAVCAN_TUNNEL_TARGETTED_ID,
                                                   &transfer_id,
                                                   CANARD_TRANSFER_PRIORITY_MEDIUM,
                                                   payload,
                                                   payload_length);
    assert(payload_length == 126U);
    assert(queued_frames == 19);

    uint64_t timestamp_us = 1000U;
    uint32_t delivered_frames = 0U;
    while (canardPeekTxQueue(&sender) != NULL) {
        const CanardCANFrame frame = *canardPeekTxQueue(&sender);
        canardPopTxQueue(&sender);
        assert(canardHandleRxFrame(&receiver, &frame, timestamp_us++) >= 0);
        delivered_frames++;
    }

    assert(delivered_frames == 19U);
    assert(received_count == 1U);
    assert(received_message.protocol.protocol == sent_message.protocol.protocol);
    assert(received_message.target_node == sent_message.target_node);
    assert(received_message.serial_id == sent_message.serial_id);
    assert(received_message.options == sent_message.options);
    assert(received_message.baudrate == sent_message.baudrate);
    assert(received_message.buffer.len == sent_message.buffer.len);
    assert(memcmp(received_message.buffer.data,
                  sent_message.buffer.data,
                  sent_message.buffer.len) == 0);

    printf("PASS: payload=%u bytes, CAN frames=%u, decoded serial_id=%d\n",
           payload_length,
           delivered_frames,
           received_message.serial_id);
    return 0;
}
