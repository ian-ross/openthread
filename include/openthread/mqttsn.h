/*
 *  Copyright (c) 2018, Vit Holasek
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef OPENTHREAD_MQTTSN_H_
#define OPENTHREAD_MQTTSN_H_

#include <openthread/instance.h>
#include <openthread/ip6.h>

#define OT_DEFAULT_MQTTSN_PORT 1883

#ifndef OPENTHREAD_CONFIG_MQTTSN_ENABLE
#define OPENTHREAD_CONFIG_MQTTSN_ENABLE 0
#endif

/**
 * MQTT-SN message return code.
 *
 */
enum otMqttsnReturnCode
{
    kCodeAccepted = 0,
    kCodeRejectedCongestion = 1,
    kCodeRejectedTopicId = 2,
    kCodeRejectedNotSupported = 3,
    /**
     * Pending message timed out. this value is not returned by gateway.
     */
    kCodeTimeout = -1,
};

/**
 * MQTT-SN quality of service level.
 *
 */
enum otMqttsnQos
{
    kQos0 = 0x0,
    kQos1 = 0x1,
    kQos2 = 0x2,
    kQosm1 = 0x3
};

/**
 * Client lifecycle states.
 *
 */
enum otMqttsnClientState
{
    /**
     * Client is not connected to gateway.
     */
    kStateDisconnected,
    /**
     * Client is connected to gateway and currently alive.
     */
    kStateActive,
    /**
     * Client is in sleeping state.
     */
    kStateAsleep,
    /**
     * Client is awaken from sleep.
     */
    kStateAwake,
    /**
     * Client connection is lost due to communication error.
     */
    kStateLost,
};

/**
 * Topic ID type.
 *
 */
typedef uint16_t otMqttsnTopicId;

/**
 * This structure contains MQTT-SN connection parameters.
 *
 */
typedef struct otMqttsnConfig {
	/**
	 * Gateway IPv6 address.
	 */
	otIp6Address mAddress;
	/**
	 * Gateway interface port number.
	 */
    uint16_t mPort;
    /**
     * Client id string.
     */
    char *mClientId;
    /**
     * Keepalive period in seconds.
     */
    uint16_t mKeepAlive;
    /**
     * Clean session flag.
     */
    bool mCleanSession;
    /**
     * Retransmission timeout in milliseconds.
     */
    uint32_t mRetransmissionTimeout;
    /**
     * Retransmission count.
     */
    uint8_t mRetransmissionCount;
} otMqttsnConfig;

/**
 * Declaration of function for connection callback.
 *
 * @param[in]  aCode     CONNACK return code value or -1 when connection establishment timed out.
 * @param[in]  aContext  A pointer to connection callback context object.
 *
 */
typedef void (*otMqttsnConnectedHandler)(otMqttsnReturnCode aCode, void *aContext);
/**
 * Declaration of function for subscribe callback.
 *
 * @param[in]  aCode     SUBACK return code or -1 when subscription timed out.
 * @param[in]  aTopicId  Subscribed topic ID. The value is 0 when timed out or subscribed by short topic name.
 * @param[in]  aQos      Subscribed quality of service level.
 * @param[in]  aContext  A pointer to subscription callback context object.
 *
 */
typedef void (*otMqttsnSubscribedHandler)(otMqttsnReturnCode aCode, otMqttsnTopicId aTopicId, otMqttsnQos aQos, void* aContext);

/**
 * Declaration of function for register callback.
 *
 * @param[in]  aCode     REGACK return code or -1 when subscription timed out.
 * @param[in]  aTopicId  Registered topic ID.
 * @param[in]  aContext  A pointer to register callback context object.
 *
 */
typedef void (*otMqttsnRegisteredHandler)(otMqttsnReturnCode aCode, otMqttsnTopicId aTopicId, void* aContext);

/**
 * Start MQTT-SN service and start connection and listening.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aPort      MQTT-SN client listening port.
 *
 * @retval OT_ERROR_NONE  Successfully started the service.
 *
 */
otError otMqttsnStart(otInstance *aInstance, uint16_t aPort);

/**
 * Stop MQTT-SN service.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE  Successfully stopped the service.
 *
 */
otError otMqttsnStop(otInstance *aInstance);

/**
 * Get current MQTT-SN client state.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @return Returns current MQTT-SN client state.
 *
 */
otMqttsnClientState otMqttsnGetState(otInstance *aInstance);

/**
 * Establish MQTT-SN connection with gateway.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aConfig  A reference to configuration object with connection parameters.
 *
 * @retval OT_ERROR_NONE           Connection message successfully queued.
 * @retval OT_ERROR_INVALID_ARGS   Invalid connection parameters.
 * @retval OT_ERROR_INVALID_STATE  The client is in invalid state. It must be disconnected before new connection establishment.
 * @retval OT_ERROR_NO_BUFS        Insufficient available buffers to process.
 *
 */
otError otMqttsnConnect(otInstance *aInstance, const otMqttsnConfig *aConfig);

/**
 * Establish MQTT-SN connection with gateway with default configuration.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  mAddress   Gateway IPv6 address.
 * @param[in]  mPort      Gateway interface port number.
 *
 * @retval OT_ERROR_NONE           Connection message successfully queued.
 * @retval OT_ERROR_INVALID_ARGS   Invalid connection parameters.
 * @retval OT_ERROR_INVALID_STATE  The client is in invalid state. It must be disconnected before new connection establishment.
 * @retval OT_ERROR_NO_BUFS        Insufficient available buffers to process.
 *
 */
otError otMqttsnConnectDefault(otInstance *aInstance, otIp6Address mAddress, uint16_t mPort);

/**
 * Subscribe to the topic by topic name string.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 * @param[in]  aTopicName         A pointer to long topic name string.
 * @param[in]  aQos               Quality of service level to be subscribed.
 * @param[in]  aHandler           A function pointer to handler which is invoked when subscription is acknowledged.
 * @param[in]  aContext           A pointer to context object passed to handler.
 *
 * @retval OT_ERROR_NONE           Subscription message successfully queued.
 * @retval OT_ERROR_INVALID_ARGS   Invalid subscription parameters.
 * @retval OT_ERROR_INVALID_STATE  The client cannot connect in active state.
 * @retval OT_ERROR_NO_BUFS        Insufficient available buffers to process.
 *
 */
otError otMqttsnSubscribe(otInstance *aInstance, const char *aTopicName, otMqttsnQos aQos, otMqttsnSubscribedHandler aHandler, void *aContext);

/**
 * Subscribe to the topic by short topic name string.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 * @param[in]  aShortTopicName    A pointer to short topic name string. Must be 1 or 2 characters long.
 * @param[in]  aQos               Quality of service level to be subscribed.
 * @param[in]  aHandler           A function pointer to handler which is invoked when subscription is acknowledged.
 * @param[in]  aContext           A pointer to context object passed to handler.
 *
 * @retval OT_ERROR_NONE           Subscription message successfully queued.
 * @retval OT_ERROR_INVALID_ARGS   Invalid subscription parameters.
 * @retval OT_ERROR_INVALID_STATE  The client cannot connect in active state.
 * @retval OT_ERROR_NO_BUFS        Insufficient available buffers to process.
 *
 */
otError otMqttsnSubscribeShort(otInstance *aInstance, const char *aShortTopicName, otMqttsnQos aQos, otMqttsnSubscribedHandler aHandler, void *aContext);

/**
 * Subscribe to the topic by predefined topic id.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 * @param[in]  aTopicId           Predefined topic ID to subscribe to.
 * @param[in]  aQos               Quality of service level to be subscribed.
 * @param[in]  aHandler           A function pointer to handler which is invoked when subscription is acknowledged.
 * @param[in]  aContext           A pointer to context object passed to handler.
 *
 * @retval OT_ERROR_NONE           Subscription message successfully queued.
 * @retval OT_ERROR_INVALID_ARGS   Invalid subscription parameters.
 * @retval OT_ERROR_INVALID_STATE  The client cannot connect in active state.
 * @retval OT_ERROR_NO_BUFS        Insufficient available buffers to process.
 *
 */
otError otMqttsnSubscribeTopicId(otInstance *aInstance, otMqttsnTopicId aTopicId, otMqttsnQos aQos, otMqttsnSubscribedHandler aHandler, void *aContext);

/**
 * Register to topic with long topic name and obtain related topic ID.
 *
 * @param[in]  aInstance           A pointer to an OpenThread instance.
 * @param[in]  aTopicName          A pointer to long topic name string.
 * @param[in]  aHandler            A function pointer to callback invoked when registration is acknowledged.
 * @param[in]  aContext            A pointer to context object passed to callback.
 *
 * @retval OT_ERROR_NONE           Registration message successfully queued.
 * @retval OT_ERROR_INVALID_STATE  The client is not in active state.
 * @retval OT_ERROR_NO_BUFS        Insufficient available buffers to process.
 *
 */
otError otMqttsnRegister(otInstance *aInstance, const char* aTopicName, otMqttsnRegisteredHandler aHandler, void* aContext);

/**
 * Set handler which is invoked when connection is acknowledged.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 * @param[in]  aHandler           A function pointer to handler which is invoked when connection is acknowledged.
 * @param[in]  aContext           A pointer to context object passed to handler.
 *
 * @retval OT_ERROR_NONE          Handler correctly set.
 *
 */
otError otMqttsnSetConnectedHandler(otInstance *aInstance, otMqttsnConnectedHandler aHandler, void *aContext);

/**
 * Get string value of given return code.
 *
 * @param[in]  aCode              MQTT-SN message return code.
 * @param[out] aCodeString        A pointer to string pointer which will contain return code string value.
 *
 * @retval OT_ERROR_NONE          String value was obtained.
 * @retval OT_ERROR_INVALID_ARGS  Invalid return code value.
 *
 */
otError otMqttsnReturnCodeToString(otMqttsnReturnCode aCode, const char** aCodeString);

/**
 * Get MQTT-SN quality of service level from string value. Only values '0', '1', '2' and '-1' are allowed.
 *
 * @param[in]   aQosString        A pointer to string with MQTT-SN QoS level value
 * @param[out]  aQos              A pointer to MQTT-SN QoS level which will be set.
 *
 * @retval OT_ERROR_NONE          MQTT-SN QoS level was obtained..
 * @retval OT_ERROR_INVALID_ARGS  Invalid QoS string value.
 *
 */
otError otMqttsnStringToQos(const char* aQosString, otMqttsnQos *aQos);

/**
 * Get string value of given MQTT-SN client state.
 *
 * @param[in]  aClientState        MQTT-SN client state.
 * @param[out] aClientStateString  A pointer to string pointer which will contain client state string value.
 *
 * @retval OT_ERROR_NONE           String value was obtained.
 * @retval OT_ERROR_INVALID_ARGS   Invalid client state value.
 *
 */
otError otMqttsnClientStateToString(otMqttsnClientState aClientState, const char** aClientStateString);

#endif /* OPENTHREAD_MQTTSN_H_ */