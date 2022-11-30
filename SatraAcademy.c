#define _CRT_SECURE_NO_WARNINGS
#define DEFAULT_BUFLEN 0x512

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <shlwapi.h>

int checkSocketError(int ret) {

	if (ret == SOCKET_ERROR) {
		printf("[-] Error. Exiting...\n");
		return 0;
	}
	if (ret == INVALID_SOCKET) {
		printf("[-] Error. Exiting...\n");
		return 0;
	}

	return 1;

}

int handleOTP(SOCKET clientSocket) {
	// Receive OTP from client
	char OTPBuf[0x10];
	long long OTPRecv;
	memset(OTPBuf, 0, sizeof(OTPBuf));
	recv(clientSocket, OTPBuf, sizeof(OTPBuf) , 0);

	// convert to DWORD
	OTPRecv = ((OTPBuf[3] << 24) & 0xff000000) | ((OTPBuf[2] << 16) & 0xff0000) | ((OTPBuf[1] << 8) & 0xff00) | (OTPBuf[0] & 0xff);
	printf("[+] OTP Received: %x\n", OTPRecv);

	// Generate OTP (changes every 10 seconds)
	long long seconds = time(0) / 10;
	long long OTPGen = ((0x186343 ^ seconds) << 4) ^ 0x45124021;

	if (OTPRecv != OTPGen) {
		return 0;
	}
}

void listCourse(SOCKET clientSocket, char courseName[10][50], char lecturer[10][50], int courseArrayLen) {

	// calculate strLen of all courses in courseName
	printf("[+] Selected option : LIST\n");
	int courseNameLen = 0;
	for (int i = 0; i < courseArrayLen; i++) {
		courseNameLen += strlen(courseName[i]);
		courseNameLen += strlen(lecturer[i]);
	}

	//adding 50 chars to compensate for extra text in sprintf
	courseNameLen += 70;

	// allocating memory for strings that will hold tmp & final response
	char *tmpListing = (char *)malloc(courseNameLen); 
	char *finalListing = (char *)malloc(courseNameLen);
	memset(tmpListing, 0, sizeof(tmpListing));
	memset(finalListing, 0, sizeof(finalListing));

	// appending each course to finalListing
	for (int i = 0; i < courseArrayLen ; i++) {
		sprintf(tmpListing, "%d. %s - Professor %s\n", i + 1, courseName[i], lecturer[i]);
		strcpy(finalListing + strlen(finalListing), tmpListing);
	}

	// Send result to user
	send(clientSocket, finalListing, strlen(finalListing), 0);

	// free allocated memory
	free(tmpListing);
	free(finalListing);
}

int addCourse(SOCKET clientSocket, char courseName[10][50], char lecturer[10][50], char buildingName[10][50], int courseArrayLen) {

	printf("[+] Selected option : ADD\n");

	// variables to receive & store user input
	char newCourseName[500], newLecturer[500], newBuildingName[500];
	char recvInput[400];
	memset(recvInput, 0, sizeof(recvInput));
	memset(newCourseName, 0, sizeof(newCourseName));
	memset(newLecturer, 0, sizeof(newLecturer));
	memset(newBuildingName, 0, sizeof(newBuildingName));

	// receive another 400 bytes from user
	int recvLen = recv(clientSocket, recvInput, sizeof(recvInput), 0); 
	printf("[+] Received input : 0x%x bytes (%s)\n", recvLen, recvInput);

	// transform received input and store in temp char[]
	int m = sscanf(recvInput, "Course:%[^,],Lecturer:%[^,],Building:%s", newCourseName, newLecturer, newBuildingName);

	// checking if input is correct (sscanf should return with 3 variables filled)
	if (m != 3) {
		printf("[-] ADD request failed. Invalid submission\n");
		return 0;
	}


	// update new coursename,lecturer,buildingname
	strcpy(courseName[courseArrayLen], newCourseName);
	strcpy(lecturer[courseArrayLen], newLecturer);
	strcpy(buildingName[courseArrayLen], newBuildingName);

	printf("[+] Course successfully added\n");
	return 1;
}

void searchCourse(SOCKET clientSocket, char courseName[10][50], int courseArrayLen) {

	printf("[+] Selected option : SEARCH\n");

	// receive 30 bytes (searchString) from user
	char searchString[30];
	memset(searchString, 0, sizeof(searchString));
	int recvLen = recv(clientSocket, searchString, sizeof(searchString), 0);
	printf("[+] Received input : 0x%x bytes (%s)\n", recvLen, searchString);

	for (int i = 0; i < courseArrayLen; i++) {
		char *res = StrStrIA(courseName[i], searchString);
		if (res != NULL) {
			char searchResponse[60];
			memset(searchResponse, 0, sizeof(searchResponse));
			sprintf(searchResponse, "\nThis course exists!\nCourseName: %s", courseName[i]);
			printf("[+] SEARCH request successful\n");
			send(clientSocket, searchResponse, sizeof(searchResponse), 0);
			return 1;
		}
	}

	printf("[-] SEARCH request failed. Course doesn't exist\n");
	send(clientSocket, "\nThis course doesn't exist", 27, 0);
	return 0;
}

int editCourse(SOCKET clientSocket, char courseName[10][50], char lecturer[10][50], char buildingName[10][50], int courseArrayLen) {
	printf("[+] Selected option: EDIT\n");

	// receive 512 bytes from user
	char recvInput[DEFAULT_BUFLEN];
	memset(recvInput, 0, sizeof(recvInput));

	int recvLen = recv(clientSocket, recvInput, sizeof(recvInput), 0); 
	printf("[+] Received input : 0x%x bytes (%s)\n", recvLen, recvInput);

	// transform received input and store in char[]
	char editNum[0x4];
	char editCourseName[0x512];
	char editLecturerName[0x512];
	char editBuildingName[0x512];
	memset(editNum, 0, sizeof(editNum));
	memset(editCourseName, 0, sizeof(editCourseName));
	memset(editLecturerName, 0, sizeof(editLecturerName));
	memset(editBuildingName, 0, sizeof(editBuildingName));

	int m = sscanf(recvInput, "%[^,],Course:%[^,],Lecturer:%[^,],Building:%s", editNum, editCourseName, editLecturerName, editBuildingName);


	// check sscanf result = 4, editNum is valid, then update courseName
	int num = atoi(editNum);
	if (m == 4 && num <= courseArrayLen && num > 0) {

		memset(courseName[num - 1], 0, sizeof(courseName[num - 1]));
		strcpy(courseName[num - 1], editCourseName);

		memset(lecturer[num - 1], 0, sizeof(lecturer[num - 1]));
		strcpy(lecturer[num - 1], editLecturerName);

		memset(buildingName[num - 1], 0, sizeof(buildingName[num - 1]));
		strcpy(buildingName[num - 1], editBuildingName);

		printf("[+] Course successfully edited\n");
		return 1;
	}

	printf("[-] EDIT request failed - Course doesn't exist\n");
	return 0;
}


int main()
{
	char courseName[10][50] = { "Introduction to Levitation", "Good And Evil", "Advanced Alchemy" };
	char lecturer[10][50] = { "Rasputin", "Nietzsche", "Strindberg" };
	char buildingName[10][50] = { "Nevsky Monastery", "Uppsala Chambers", "Schulpforta" };
	int courseArrayLen = 3;

	printf("[+] Satra Academy Course Management Portal - Server Started\n");

	// Set up Winsock : https://learn.microsoft.com/en-us/windows/win32/winsock/initializing-winsock
	// Socket initialization
	WSADATA wsaData;
	if (checkSocketError(WSAStartup(2.2, &wsaData)) != 1) {
		return 0;
	}

	// Socket object instantiation for use by server
	struct addrinfo *ppResult = NULL;
	struct addrinfo pHints;

	ZeroMemory(&pHints, sizeof(pHints));
	pHints.ai_family = AF_INET;
	pHints.ai_flags = AI_PASSIVE;
	pHints.ai_socktype = SOCK_STREAM;
	pHints.ai_protocol = IPPROTO_TCP;


	if (checkSocketError(getaddrinfo(NULL, "9112", &pHints, &ppResult)) != 1) {
		WSACleanup();
		return 0;
	}

	SOCKET listenSocket;
	if (checkSocketError(listenSocket = socket(ppResult->ai_family, ppResult->ai_socktype, ppResult->ai_protocol)) != 1) {
		freeaddrinfo(ppResult);
		WSACleanup();
		return 0;
	}

	// Binding socket to an IP address and port
	int bindResult;
	if (checkSocketError(bind(listenSocket, ppResult->ai_addr, ppResult->ai_addrlen)) != 1) {
		freeaddrinfo(ppResult);
		closesocket(listenSocket);
		WSACleanup();
		return 0;
	}

	// free memory in ppResult addrInfo struct
	freeaddrinfo(ppResult);

	// Listening on socket
	if (checkSocketError(listen(listenSocket, SOMAXCONN)) != 1) {
		closesocket(listenSocket);
		WSACleanup();
		return 0;
	}

	// Accept a connection
	SOCKET clientSocket; // temporary SOCKET object to accept connections
	if (checkSocketError(clientSocket = accept(listenSocket, NULL, NULL)) == 1) {

		//Connection established
		printf("[+] Connection Established\n");

		// Send welcome message 
		char welcomeMessage[] = "Welcome to Satra Academy Course Management Portal\nValid Options : LIST ADD SEARCH EDIT";
		send(clientSocket, welcomeMessage, sizeof(welcomeMessage), 0);

		// Check OTP and exit if incorrect
		if (handleOTP(clientSocket) == 0) {
			printf("[-] OTP Incorrect\n");
			return 0;
		}

		printf("[+] OTP Authentication Successful!\n");

		// Receive user option input
		char OptionBuf[0x10];
		memset(OptionBuf, 0, sizeof(OptionBuf));
		recv(clientSocket, OptionBuf, sizeof(OptionBuf), 0); 

		
		// parse input
		if (strncmp("LIST", OptionBuf, 4) == 0) 
		{
			listCourse(clientSocket, courseName, lecturer, courseArrayLen); 
		}
		else if (strncmp("ADD", OptionBuf, 3) == 0)
		{
			addCourse(clientSocket, courseName, lecturer, buildingName, courseArrayLen);
		}
		else if (strncmp("SEARCH", OptionBuf, 6) == 0)
		{
			searchCourse(clientSocket, courseName, courseArrayLen);
		}
		else if (strncmp("EDIT", OptionBuf, 4) == 0)
		{
			editCourse(clientSocket, courseName, lecturer, buildingName, courseArrayLen);
		}
		else {
			printf("[-] Invalid Action\n");
		}

		printf("[-] Connection Closed\n");
		return 1;
	}


	// Program exiting
	printf("[-] Error. Server shutting down\n");
	shutdown(clientSocket, SD_SEND);
	closesocket(clientSocket);
	closesocket(listenSocket);
	WSACleanup();
	return 1;
}


