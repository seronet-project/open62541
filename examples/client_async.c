/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <stdio.h>
#include "open62541.h"


static
void valueWritten(UA_Client *client, void *userdata,
                                 UA_UInt32 requestId, const void *response){
    printf("value written \n");
}
static
void valueRead(UA_Client *client, void *userdata,
                                 UA_UInt32 requestId, const void *response){

    printf("value Read \n");
}


static
void methodCalled(UA_Client *client, void *userdata,
	UA_UInt32 requestId, const void *response) {
	printf("Method Called \n");
}

#define OPCUA_SERVER_URI "opc.tcp://localhost:4840"

int main(int argc, char *argv[]) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);

    /* Listing endpoints */
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval = UA_Client_getEndpoints(client, OPCUA_SERVER_URI,
                                                  &endpointArraySize, &endpointArray);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Array_delete(endpointArray, endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
        UA_Client_delete(client);
        return (int)retval;
    }
    printf("%i endpoints found\n", (int)endpointArraySize);
    for(size_t i=0;i<endpointArraySize;i++){
        printf("URL of endpoint %i is %.*s\n", (int)i,
               (int)endpointArray[i].endpointUrl.length,
               endpointArray[i].endpointUrl.data);
    }
    UA_Array_delete(endpointArray,endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    /* Connect to a server */
    /* anonymous connect would be: retval = UA_Client_connect(client, OPCUA_SERVER_URI); */
    retval = UA_Client_connect_username(client, OPCUA_SERVER_URI, "user1", "password");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return (int)retval;
    }
    UA_String sValue;
    sValue.data = (UA_Byte*) malloc(90000);
    memset(sValue.data,'a',90000);
    sValue.length = 90000;

    printf("\nWriting a value of node (1, \"the.answer\"):\n");
    
	//Construct write request
	UA_WriteRequest wReq;
    UA_WriteRequest_init(&wReq);
    wReq.nodesToWrite = UA_WriteValue_new();
    wReq.nodesToWriteSize = 1;
    wReq.nodesToWrite[0].nodeId = UA_NODEID_NUMERIC(1, 51034);
    wReq.nodesToWrite[0].attributeId = UA_ATTRIBUTEID_VALUE;
    wReq.nodesToWrite[0].value.hasValue = true;
    wReq.nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_STRING];
    wReq.nodesToWrite[0].value.value.storageType = UA_VARIANT_DATA_NODELETE; /* do not free the integer on deletion */
    wReq.nodesToWrite[0].value.value.data = &sValue;

	//Construct read request
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_NUMERIC(1, 51034);
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;

	// Construct call request
	UA_CallRequest callRequ;
	UA_CallRequest_init(&callRequ);

	UA_CallMethodRequest methodRequest;
	UA_CallMethodRequest_init(&methodRequest);
	methodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
	methodRequest.methodId = UA_NODEID_NUMERIC(1, 62541);
	UA_Variant input;
	UA_String argString = UA_STRING("input_Argument");
	UA_Variant_init(&input);
	UA_Variant_setScalarCopy(&input, &argString, &UA_TYPES[UA_TYPES_STRING]);
	methodRequest.inputArguments = &input;
	methodRequest.inputArgumentsSize = 1;

	callRequ.methodsToCall = &methodRequest;
	callRequ.methodsToCallSize = 1;


    UA_UInt32 reqId;
	UA_StatusCode retVal;
    while(true){
		printf("Send Requests \n");
		retVal = UA_Client_AsyncService_write(client, wReq, valueWritten, NULL, &reqId);
		retVal = UA_Client_AsyncService_read(client, rReq, valueRead, NULL, &reqId);
		retVal = UA_Client_AsyncService_call(client, callRequ, methodCalled, NULL, &reqId);
		printf("Requests Send \n");
		UA_Client_runAsync(client, 500);
		UA_Client_runAsync(client, 500);
		UA_Client_runAsync(client, 500);
		//TODO sleep
    }

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return (int) UA_STATUSCODE_GOOD;
}
