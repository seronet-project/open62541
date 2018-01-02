/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */



#include "ua_client_async.h"
#include "ua_client_internal.h"


#ifdef UA_ENABLE_ASYNC_CLIENT_API


#ifdef UA_ENABLE_SUBSCRIPTIONS

void callback_PublishResponce(UA_Client *client, void *userdata,
	UA_UInt32 requestId, const void *response)
{
	UA_PublishResponse* resp = (UA_PublishResponse*)response;
	UA_PublishRequest* requ = (UA_PublishRequest*)userdata;
	UA_Client_processPublishResponse(client, requ, resp);
	
	UA_PublishRequest_deleteMembers(requ);
}

UA_StatusCode
UA_Client_Subscriptions_manuallySendPublishRequest_Async(UA_Client *client) {
	if (client->state < UA_CLIENTSTATE_SESSION)
		return UA_STATUSCODE_BADSERVERNOTCONNECTED;

	UA_StatusCode retval = UA_STATUSCODE_GOOD;

	UA_PublishRequest request;
	UA_PublishRequest_init(&request);
	request.subscriptionAcknowledgementsSize = 0;

	UA_Client_NotificationsAckNumber *ack;
	LIST_FOREACH(ack, &client->pendingNotificationsAcks, listEntry)
		++request.subscriptionAcknowledgementsSize;
	if (request.subscriptionAcknowledgementsSize > 0) {
		request.subscriptionAcknowledgements = (UA_SubscriptionAcknowledgement*)
			UA_malloc(sizeof(UA_SubscriptionAcknowledgement) * request.subscriptionAcknowledgementsSize);
		if (!request.subscriptionAcknowledgements)
			return UA_STATUSCODE_BADOUTOFMEMORY;
	}

	int i = 0;
	LIST_FOREACH(ack, &client->pendingNotificationsAcks, listEntry) {
		request.subscriptionAcknowledgements[i].sequenceNumber = ack->subAck.sequenceNumber;
		request.subscriptionAcknowledgements[i].subscriptionId = ack->subAck.subscriptionId;
		++i;
	}

	UA_UInt32 reqId;
	retval = UA_Client_AsyncService_publish(client, request, callback_PublishResponce, &request, &reqId);

	if (client->state < UA_CLIENTSTATE_SESSION)
		return UA_STATUSCODE_BADSERVERNOTCONNECTED;

	return retval;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
#endif /* UA_ENABLE_ASYNC_CLIENT_API */
