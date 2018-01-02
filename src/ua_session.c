/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_session.h"
#include "ua_types_generated_handling.h"
#include "ua_util.h"
#ifdef UA_ENABLE_SUBSCRIPTIONS
#include "server/ua_subscription.h"
#endif

UA_Session adminSession = {
    {{0, NULL},{0, NULL},
     {{0, NULL},{0, NULL}},
     UA_APPLICATIONTYPE_CLIENT,
     {0, NULL},{0, NULL},
     0, NULL}, /* .clientDescription */
    {sizeof("Administrator Session")-1, (UA_Byte*)"Administrator Session"}, /* .sessionName */
    false, /* .activated */
    NULL, /* .sessionHandle */
    {0,UA_NODEIDTYPE_NUMERIC,{1}}, /* .authenticationToken */
    {0,UA_NODEIDTYPE_NUMERIC,{1}}, /* .sessionId */
    UA_UINT32_MAX, /* .maxRequestMessageSize */
    UA_UINT32_MAX, /* .maxResponseMessageSize */
    (UA_Double)UA_INT64_MAX, /* .timeout */
    UA_INT64_MAX, /* .validTill */
    {0, NULL},
    NULL, /* .channel */
    UA_MAXCONTINUATIONPOINTS, /* .availableContinuationPoints */
    {NULL}, /* .continuationPoints */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    0, /* .lastSubscriptionID */
    0, /* .lastSeenSubscriptionID */
    {NULL}, /* .serverSubscriptions */
    {NULL, NULL}, /* .responseQueue */
    0, /* numSubscriptions */
    0  /* numPublishReq */
#endif
};

void UA_Session_init(UA_Session *session) {
    UA_ApplicationDescription_init(&session->clientDescription);
    session->activated = false;
    UA_NodeId_init(&session->authenticationToken);
    UA_NodeId_init(&session->sessionId);
    UA_String_init(&session->sessionName);
    UA_ByteString_init(&session->serverNonce);
    session->maxRequestMessageSize  = 0;
    session->maxResponseMessageSize = 0;
    session->timeout = 0;
    UA_DateTime_init(&session->validTill);
    session->channel = NULL;
    session->availableContinuationPoints = UA_MAXCONTINUATIONPOINTS;
    LIST_INIT(&session->continuationPoints);
#ifdef UA_ENABLE_SUBSCRIPTIONS
    LIST_INIT(&session->serverSubscriptions);
    session->lastSubscriptionID = 0;
    session->lastSeenSubscriptionID = 0;
    SIMPLEQ_INIT(&session->responseQueue);
    session->numSubscriptions = 0;
    session->numPublishReq = 0;
#endif
}

void UA_Session_deleteMembersCleanup(UA_Session *session, UA_Server* server) {
    UA_ApplicationDescription_deleteMembers(&session->clientDescription);
    UA_NodeId_deleteMembers(&session->authenticationToken);
    UA_NodeId_deleteMembers(&session->sessionId);
    UA_String_deleteMembers(&session->sessionName);
    UA_ByteString_deleteMembers(&session->serverNonce);
    struct ContinuationPointEntry *cp, *temp;
    LIST_FOREACH_SAFE(cp, &session->continuationPoints, pointers, temp) {
        LIST_REMOVE(cp, pointers);
        UA_ByteString_deleteMembers(&cp->identifier);
        UA_BrowseDescription_deleteMembers(&cp->browseDescription);
        UA_free(cp);
    }
    if(session->channel)
        UA_SecureChannel_detachSession(session->channel, session);
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_Subscription *currents, *temps;
    LIST_FOREACH_SAFE(currents, &session->serverSubscriptions, listEntry, temps) {
        LIST_REMOVE(currents, listEntry);
        UA_Subscription_deleteMembers(currents, server);
        UA_free(currents);
    }
    UA_PublishResponseEntry *entry;
    while((entry = UA_Session_getPublishReq(session))) {
        UA_Session_removePublishReq(session,entry);
        UA_PublishResponse_deleteMembers(&entry->response);
        UA_free(entry);
    }
#endif
}

void UA_Session_updateLifetime(UA_Session *session) {
    session->validTill = UA_DateTime_nowMonotonic() +
        (UA_DateTime)(session->timeout * UA_DATETIME_MSEC);
}

#ifdef UA_ENABLE_SUBSCRIPTIONS

void UA_Session_addSubscription(UA_Session *session, UA_Subscription *newSubscription) {
    session->numSubscriptions++;
    LIST_INSERT_HEAD(&session->serverSubscriptions, newSubscription, listEntry);
}

UA_StatusCode
UA_Session_deleteSubscription(UA_Server *server, UA_Session *session,
                              UA_UInt32 subscriptionID) {
    UA_Subscription *sub = UA_Session_getSubscriptionByID(session, subscriptionID);
    if(!sub)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;

    LIST_REMOVE(sub, listEntry);
    UA_Subscription_deleteMembers(sub, server);
    UA_free(sub);
    if(session->numSubscriptions > 0) {
        session->numSubscriptions--;
    }
    else {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

UA_UInt32
UA_Session_getNumSubscriptions( UA_Session *session ) {
   return session->numSubscriptions;
}

UA_Subscription *
UA_Session_getSubscriptionByID(UA_Session *session, UA_UInt32 subscriptionID) {
    UA_Subscription *sub;
    LIST_FOREACH(sub, &session->serverSubscriptions, listEntry) {
        if(sub->subscriptionID == subscriptionID)
            break;
    }
    return sub;
}

UA_UInt32 UA_Session_getUniqueSubscriptionID(UA_Session *session) {
    return ++(session->lastSubscriptionID);
}

UA_UInt32
UA_Session_getNumPublishReq(UA_Session *session) {
    return session->numPublishReq;
}

UA_PublishResponseEntry*
UA_Session_getPublishReq(UA_Session *session) {
    return SIMPLEQ_FIRST(&session->responseQueue);
}

void
UA_Session_removePublishReq( UA_Session *session, UA_PublishResponseEntry* entry){
    UA_PublishResponseEntry* firstEntry;
    firstEntry = SIMPLEQ_FIRST(&session->responseQueue);

    /* Remove the response from the response queue */
    if((firstEntry != 0) && (firstEntry == entry)) {
        SIMPLEQ_REMOVE_HEAD(&session->responseQueue, listEntry);
        session->numPublishReq--;
    }
}

void UA_Session_addPublishReq( UA_Session *session, UA_PublishResponseEntry* entry){
    SIMPLEQ_INSERT_TAIL(&session->responseQueue, entry, listEntry);
    session->numPublishReq++;
}

#endif
