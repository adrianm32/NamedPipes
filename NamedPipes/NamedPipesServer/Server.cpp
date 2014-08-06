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
void FreePipeSecurity(PSECURITY_ATTRIBUTES );


int wmain(int argc, wchar_t * argv[])
{
	DWORD dwError = ERROR_SUCCESS;  //error code definitions in winerr.h for windows apis
	PSECURITY_ATTRIBUTES pSA = nullptr;
	HANDLE hNamedPipe = INVALID_HANDLE_VALUE;   //handleapi.h

	// Prepare the security attributes (the lpSecurityAttributes parameter in 
	// CreateNamedPipe) for the pipe. This is optional. If the 
	// lpSecurityAttributes parameter of CreateNamedPipe is NULL, the named 
	// pipe gets a default security descriptor and the handle cannot be 
	// inherited. The ACLs in the default security descriptor of a pipe grant 
	// full control to the LocalSystem account, (elevated) administrators, 
	// and the creator owner. They also give only read access to members of 
	// the Everyone group and the anonymous account. However, if you want to 
	// customize the security permission of the pipe, (e.g. to allow 
	// Authenticated Users to read from and write to the pipe), you need to 
	// create a SECURITY_ATTRIBUTES structure.
	if (!CreatePipeSecurity(&pSA))   //passing by ref so that the actual pointer value is updated.
	{
		dwError = GetLastError();  //errohandlingapi.h
		wprintf_s(L"CreatePipeSecurity failed with error 0x%08lx\n", dwError); // printf format %[parameter][flags][width][.precision][length]type
		goto Cleanup;
	}

	//Create Named pipe
	hNamedPipe = CreateNamedPipe(
		FULL_PIPE_NAME,
		PIPE_ACCESS_DUPLEX,  //open mode
		PIPE_TYPE_MESSAGE |
			PIPE_READMODE_MESSAGE |
			PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES, //max intances
		BUFFER_SIZE, //output buffer size
		BUFFER_SIZE, // input buffer size
		NMPWAIT_USE_DEFAULT_WAIT, //time out interval
		pSA
		);

	if (hNamedPipe == INVALID_HANDLE_VALUE)
	{
		dwError = GetLastError();
		wprintf_s(L"CreateNamedPipe failed with error %08lx\n", dwError);
		goto Cleanup;
	}

	wprintf_s(L"The named pipe (%s) has been successfully created. \n", FULL_PIPE_NAME);

	//wait for the client to connect.
	wprintf_s(L"Waiting for the client to connect. \n");

	// Blocking call on server.
	//ConnectNamedPipe Enables a named pipe server process to wait for a client process to connect to an instance of a named pipe. A client process connects by calling either the CreateFile or CallNamedPipe function.
	if (!ConnectNamedPipe(hNamedPipe, nullptr))   //namedpipeapi.h
	{
		dwError = GetLastError();
		if (dwError != ERROR_PIPE_CONNECTED)
		{
			wprintf_s(L"ConnectNamedPipe failed with error %08lx \n", dwError);
			goto Cleanup;
		}
	}

	wprintf(L"Client is connected.");

	//Receive request from client.

	BOOL fFinishedRead = FALSE;

	do
	{
		wchar_t chRequest[BUFFER_SIZE];
		DWORD cbRequest, cbRead;
		cbRequest = sizeof(chRequest);

		//another blocking call.
		fFinishedRead = ReadFile(    //fileapi.h
			hNamedPipe,  //file handle which is the named pipe handle
			chRequest, //buffer
			cbRequest, // no. of bytes to read
			&cbRead, // no. of bytes read  , pass by ref so that the callee can update the value.
			nullptr
			);

		dwError = GetLastError();
		if (!fFinishedRead && dwError != ERROR_MORE_DATA)
		{
			wprintf_s(L"ReadFile failed with error %08lx \n", dwError);
			goto Cleanup;
		}

		wprintf_s(L"Received %ld bytes from client: %s \n", cbRead, chRequest);

	} while (!fFinishedRead); //repeat loop if ERROR_MORE_DATA.


	//send a response from server to client.

	wchar_t chResponse[BUFFER_SIZE];
	DWORD cbResponse, cbWritten;
	cbResponse = sizeof(chResponse);

	//another blocking call.
	if (!WriteFile(
		hNamedPipe,
		chResponse,
		cbResponse,
		&cbWritten,
		nullptr
		))

	{
		dwError = GetLastError();
		wprintf_s(L"WriteFile failed with error %08lx \n", dwError);
		goto Cleanup;
	}

	wprintf(L"Sent %ld bytes to client: %s", cbWritten, chResponse);

	//Flush the pipe to allow the client to read the pipe's contents before disconnecting.
	FlushFileBuffers(hNamedPipe);
	DisconnectNamedPipe(hNamedPipe);

Cleanup:
	if (!pSA)
	{
		FreePipeSecurity(pSA);
		pSA = nullptr;  //always null after releasing.
	}

	if (hNamedPipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hNamedPipe);
		hNamedPipe = INVALID_HANDLE_VALUE;
	}

	return dwError;

}


//
//   FUNCTION: CreatePipeSecurity(PSECURITY_ATTRIBUTES *)
//
//   PURPOSE: The CreatePipeSecurity function creates and initializes a new 
//   SECURITY_ATTRIBUTES structure to allow Authenticated Users read and 
//   write access to a pipe, and to allow the Administrators group full 
//   access to the pipe.
//
//   PARAMETERS:
//   * ppSa - output a pointer to a SECURITY_ATTRIBUTES structure that allows 
//     Authenticated Users read and write access to a pipe, and allows the 
//     Administrators group full access to the pipe. The structure must be 
//     freed by calling FreePipeSecurity.
BOOL CreatePipeSecurity(PSECURITY_ATTRIBUTES *ppSA)
{
	BOOL fSucceeded = TRUE;
	DWORD dwError = ERROR_SUCCESS;

	PSECURITY_DESCRIPTOR pSD = nullptr;
	PSECURITY_ATTRIBUTES pSA = nullptr;

	//Define SDDL for the security descriptor
	PCWSTR szSDDL = L"D:"  //Discretionary ACL
		L"(A;OICI;GRGW;;;AU)"  //allow read/write to authenticated users
		L"(A;OICI;GA;;;BA)"  //allow full control to administrators
		;

	if (!ConvertStringSecurityDescriptorToSecurityDescriptor(szSDDL, SDDL_REVISION_1, &pSD, nullptr))
	{
		fSucceeded = FALSE;
		dwError = GetLastError();
		goto Cleanup;

	}

	//allocate memory for security attributes
	pSA = (PSECURITY_ATTRIBUTES)LocalAlloc(LPTR, sizeof(*pSA));  //minwinbase.h for LPTR
	if (pSA == nullptr)
	{
		fSucceeded = FALSE;
		dwError = GetLastError();
		goto Cleanup;
	}

	pSA->nLength = sizeof(*pSA);  //sizeof(SECURITY_ATTRIBUTES)
	pSA->lpSecurityDescriptor = pSD;
	pSA->bInheritHandle = FALSE;

	*ppSA = pSA;

Cleanup:
	if (!fSucceeded)
	{
		if (pSD)
		{
			LocalFree(pSD);   //winbase.h
			pSD = nullptr;
		}

		if (pSA)
		{
			LocalFree(pSA);
			pSA = nullptr;
		}

		SetLastError(dwError);
	}

	return fSucceeded;
}


void FreePipeSecurity(PSECURITY_ATTRIBUTES pSA)
{
	if (pSA)
	{
		if (pSA->lpSecurityDescriptor)
		{
			LocalFree(pSA->lpSecurityDescriptor);  //clear its contents first.

		}
		LocalFree(pSA);  //then clear itself.

	}
}