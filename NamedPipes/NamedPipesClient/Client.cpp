#pragma region Includes
#include <stdio.h>
#include <Windows.h>
#pragma endregion


#define SERVER_NAME L"."
#define PIPE_NAME L"SamplePipe"
#define FULL_PIPE_NAME L"\\\\" SERVER_NAME L"\\pipe\\" PIPE_NAME


#define BUFFER_SIZE 1024

#define REQUEST_MESSAGE L"Default request from client"

int wmain(int argc, wchar_t * argv[])
{
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	DWORD dwError = ERROR_SUCCESS;

	while (TRUE)
	{
		hPipe = CreateFile(
			FULL_PIPE_NAME,
			GENERIC_READ | GENERIC_WRITE,
			0, // no sharing
			nullptr, //default security attributes
			OPEN_EXISTING, //open existing pipe
			0,  //default attributes
			nullptr // no template file
			);


		if (hPipe != INVALID_HANDLE_VALUE)
		{
			wprintf_s(L"The named pipe %s is now connected. \n", FULL_PIPE_NAME);
			break;
		}

		dwError = GetLastError();

		if (ERROR_PIPE_BUSY != dwError)  //reverse RValue == LValue prevents accidentally assignement.
		{

			wprintf_s(L"Unable to open named pipe with error %08lx \n", dwError);
			goto Cleanup;
		}

		//all pipe instances are busy so wait for 5 secs
		if (!WaitNamedPipe(FULL_PIPE_NAME, 5000))
		{
			dwError = GetLastError();
			wprintf_s(L"Could not open named pipe. 5 sec timeout expired \n");
			goto Cleanup;
		}
	}

	
	//Set the read mode and blocking mode of the named pipe. Here , we set data to be read from the pipe as a stream of messages.
	DWORD dwMode = PIPE_READMODE_MESSAGE;
	if (!SetNamedPipeHandleState(hPipe, &dwMode, nullptr, nullptr))
	{
		dwError = GetLastError();
		wprintf_s(L"SetNamedPipeHandleState failed with error %08lx \n", dwError);
		goto Cleanup;
	}


	//Send request from client to server
	wchar_t chRequest[] = REQUEST_MESSAGE;
	DWORD cbRequest, cbWritten;

	cbRequest = sizeof(chRequest);

	if (!WriteFile(
		hPipe,
		chRequest,
		cbRequest,
		&cbWritten,
		nullptr

		))
	{
		dwError = GetLastError();
		wprintf_s(L"WriteFile failed with error message %08lx \n", dwError);
		goto Cleanup;
	}

	wprintf_s(L"Sent %ld bytes to server: %s \n", cbWritten, chRequest);

	//Receive a response from the server
	BOOL fFinishRead = TRUE;


	do
	{
		wchar_t chResponse[BUFFER_SIZE];
		DWORD cbResponse, cbRead;

		cbResponse = sizeof(chResponse);

		fFinishRead = ReadFile(
			hPipe,
			chResponse,
			cbResponse,
			&cbRead,
			nullptr
			);

		dwError = GetLastError();
		if (!fFinishRead && ERROR_MORE_DATA != dwError)
		{
			wprintf_s(L"Readfile from pipe %s failed with error %08lx \n", FULL_PIPE_NAME, dwError);
			break;
		}

		wprintf(L"Receive %ld bytes fom server : %s \n", cbRead, chResponse);

	} while (!fFinishRead);

Cleanup:
	if (hPipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hPipe);
		hPipe = INVALID_HANDLE_VALUE;

	}

	return dwError;

}