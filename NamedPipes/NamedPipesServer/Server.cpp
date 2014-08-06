/***************************************

PIPE_ACCESS_IMBOUND:
Client (Generic_Write)  ->  Server( Generic_Read)

PIPE_ACCESS_OUTBOUND:
Server (Generic_Write) -> Client (Generic_Read)

PIPE_ACCESS_DUPLEX:
Client (Generic_Read or Generic_Write or Both) <--> Server (Generic_Read and Generic_Write)

*******************************************/

#pragma region Includes
#include <stdio.h>
#include <windows.h>
#include <sddl.h>    //security descriptor description language
#pragma endregion


#define SERVER_NAME		L"."
#define PIPE_NAME	L"SamplePipe"
#define FULL_PIPE_NAME	L"\\\\" SERVER_NAME L"\\" PIPE_NAME     //concatenation done by compiler

#define BUFFER_SIZE 1024


#define RESPONSE_MESSAGE L"Default response from server"


//Forward declarations of methods so that they can be used in main method. These will be defined later.
BOOL CreatePipeSecurity(PSECURITY_ATTRIBUTES *);
void FreePipeSecurity(PSECURITY_ATTRIBUTES *);


int wmain(int argc, wchar_t * argv[])
{


}