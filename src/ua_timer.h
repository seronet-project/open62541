/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_TIMER_H_
#define UA_TIMER_H_

#include "ua_util_internal.h"
#include "ua_workqueue.h"

_UA_BEGIN_DECLS

/* An (event) timer triggers callbacks with a recurring interval. Adding,
 * removing and changing repeated callbacks can be done from independent
 * threads. Processing the changes and dispatching callbacks must be done by a
 * single "mainloop" process.
 * Timer callbacks with the same recurring interval are batched into blocks in
 * order to reduce linear search for re-entry to the sorted list after processing.
 * Callbacks are inserted in reversed order (last callback are put first in the block)
 * to allow the monitored items of a subscription (if created in a sequence with the
 * same publish/sample interval) to be executed before the subscription publish the
 * notifications. When callbacks are entered to the timer list after execution they
 * are added in the same order as before execution. */

/* Forward declaration */
struct UA_TimerCallbackEntry;
typedef struct UA_TimerCallbackEntry UA_TimerCallbackEntry;

/* Linked-list definition */
typedef SLIST_HEAD(UA_TimerCallbackList, UA_TimerCallbackEntry) UA_TimerCallbackList;

typedef struct {
    /* The linked list of callbacks is sorted according to the execution timestamp. */
    UA_TimerCallbackList repeatedCallbacks;

    /* Changes to the repeated callbacks in a multi-producer single-consumer queue */
    UA_TimerCallbackEntry * volatile changes_head;
    UA_TimerCallbackEntry *changes_tail;
    UA_TimerCallbackEntry *changes_stub;

    UA_UInt64 idCounter;
} UA_Timer;

/* Initialize the Timer. Not thread-safe. */
void UA_Timer_init(UA_Timer *t);

/* Add a repated callback. Thread-safe, can be used in parallel and in parallel
 * with UA_Timer_process. */
UA_StatusCode
UA_Timer_addRepeatedCallback(UA_Timer *t, UA_ApplicationCallback callback, void *application,
                             void *data, UA_Double interval_ms, UA_UInt64 *callbackId);

/* Change the callback interval. If this is called from within the callback. The
 * adjustment is made during the next _process call. */
UA_StatusCode
UA_Timer_changeRepeatedCallbackInterval(UA_Timer *t, UA_UInt64 callbackId,
                                        UA_Double interval_ms);

/* Remove a repated callback. Thread-safe, can be used in parallel and in
 * parallel with UA_Timer_process. */
UA_StatusCode
UA_Timer_removeRepeatedCallback(UA_Timer *t, UA_UInt64 callbackId);

/* Process (dispatch) the repeated callbacks that have timed out. Returns the
 * timestamp of the next scheduled repeated callback. Not thread-safe.
 * Application is a pointer to the client / server environment for the callback.
 * Dispatched is set to true when at least one callback was run / dispatched. */
typedef void
(*UA_TimerExecutionCallback)(void *executionApplication, UA_ApplicationCallback cb,
                             void *callbackApplication, void *data);

UA_DateTime
UA_Timer_process(UA_Timer *t, UA_DateTime nowMonotonic,
                 UA_TimerExecutionCallback executionCallback,
                 void *executionApplication);

/* Remove all repeated callbacks. Not thread-safe. */
void UA_Timer_deleteMembers(UA_Timer *t);

_UA_END_DECLS

#endif /* UA_TIMER_H_ */
