#if 0
#pragma comment(lib, "WS2_32.lib")
#include "httpa.h"

static SOCKET listenSocket;
static char szWebRoot[_MAX_PATH];
static HWND	ghwnd;
static UINT guMsgAsy;
static UINT guMsgApp;


BOOL StartHTTP(LPHTTPSERVINFO lpInfo)
{
	SOCKADDR_IN		saServer;		
	LPSERVENT			lpServEnt;		
	DWORD					dwAddrStrLen;
	char					szBuf[256];		
	char					szAddress[128];
	int						nRet;			

	ghwnd    = lpInfo->hwnd;
	guMsgAsy = lpInfo->uMsgAsy;
	guMsgApp = lpInfo->uMsgApp;

	if (lpInfo->lpRootDir != NULL)
		strcpy(szWebRoot, lpInfo->lpRootDir);
	else
		strcpy(szWebRoot, "/WebPages");

	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
		return FALSE;

	nRet = WSAAsyncSelect(listenSocket, ghwnd, guMsgAsy, FD_ACCEPT | FD_READ | FD_WRITE | FD_CLOSE);
	if (nRet == SOCKET_ERROR)
	{
		closesocket(listenSocket);
		return FALSE;
	}

	if (lpInfo->nPort != 0)
		saServer.sin_port = htons(lpInfo->nPort);
	else
	{
		lpServEnt = getservbyname("http", "tcp");
		if (lpServEnt != NULL)
			saServer.sin_port = lpServEnt->s_port;
		else
			saServer.sin_port = htons(HTTPPORT);
	}

	saServer.sin_family = AF_INET;
	saServer.sin_addr.s_addr = INADDR_ANY;

	nRet = bind(listenSocket, (LPSOCKADDR)&saServer, sizeof(struct sockaddr));
	if (nRet == SOCKET_ERROR)
	{
		closesocket(listenSocket);
		return FALSE;
	}

	nRet = listen(listenSocket, SOMAXCONN);
	if (nRet == SOCKET_ERROR)
	{
		closesocket(listenSocket);
		return FALSE;
	}

	gethostname(szBuf, sizeof(szBuf));
	dwAddrStrLen = sizeof(szAddress);
	GetLocalAddress(szAddress, &dwAddrStrLen);

	return TRUE;
}


void StopHTTP()
{
	LPREQUEST lpReq;

	closesocket(listenSocket);

	lpReq = GetFirstRequest();
	while(lpReq)
	{
		closesocket(lpReq->Socket);
		if (lpReq->hFile != HFILE_ERROR)
			_lclose(lpReq->hFile);
		lpReq = GetNextRequest(lpReq);
	}

	DelAllRequests();
}


void HandleAsyncMsg(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	int nErrorCode = WSAGETSELECTERROR(lParam);

	switch(WSAGETSELECTEVENT(lParam))
	{
		case FD_ACCEPT:
			OnAccept(hwnd, (SOCKET)wParam, nErrorCode);
			break;

		case FD_READ:
			OnRead((SOCKET)wParam, nErrorCode);
			break;

		case FD_WRITE:
			OnWrite((SOCKET)wParam, nErrorCode);
			break;

		case FD_CLOSE:
			OnClose((SOCKET)wParam, nErrorCode);
			break;
	}
}

////////////////////////////////////////////////////////////

void OnAccept(HWND hwnd, SOCKET socket, int nErrorCode)
{
	SOCKADDR_IN	SockAddr;
	LPREQUEST	lpReq;
	SOCKET		peerSocket;
	int			nRet;
	int			nLen;

	nLen = sizeof(SOCKADDR_IN);
	peerSocket = accept(listenSocket, (LPSOCKADDR)&SockAddr, &nLen);
	if (peerSocket == SOCKET_ERROR)
	{
		nRet = WSAGetLastError();
		if (nRet != WSAEWOULDBLOCK)
		{ return; }
	}

	nRet = WSAAsyncSelect(peerSocket, hwnd, guMsgAsy, FD_READ | FD_WRITE | FD_CLOSE);
	if (peerSocket == SOCKET_ERROR) {
		nRet = WSAGetLastError();
		return;	}

	lpReq = AddRequest(peerSocket, (LPSOCKADDR)&SockAddr, nLen);
	if (lpReq == NULL)
	{ closesocket(peerSocket); }
}

////////////////////////////////////////////////////////////

void OnRead(SOCKET socket, int nErrorCode)
{
	static BYTE		buf[2048];
	LPREQUEST			lpReq;
	int						nRet;

	ZeroMemory(buf, sizeof(buf));

	lpReq = GetRequest(socket);
	if (lpReq == NULL)
	{
		nRet = 0;
		while(nRet != SOCKET_ERROR)
			nRet = recv(socket, (char*)buf, sizeof(buf)-1, 0);
		closesocket(socket);
		return;
	}

	nRet = recv(socket, (char*)buf, sizeof(buf)-1, 0);
	if (nRet == SOCKET_ERROR)
	{
		if (WSAGetLastError() == WSAEWOULDBLOCK) return;
		CloseConnection(lpReq);
		return;
	}

	lpReq->dwRecv += nRet;
	ParseRequest(lpReq, buf);
}


void OnWrite(SOCKET socket, int nErrorCode)
{
	LPREQUEST lpReq;
	BYTE buf[1024];
	int nRet;

	lpReq = GetRequest(socket);
	if (lpReq == NULL)
	{
		//
		// Not in our list!?
		//
		nRet = 0;
		while(nRet != SOCKET_ERROR)
			nRet = recv(socket, (char*)buf, sizeof(buf)-1, 0);
		closesocket(socket);
		return;		
	}

	//
	// Is this the first FD_WRITE 
	// or did we fill the protocol
	// stack buffers?
	//
	if (lpReq->hFile == HFILE_ERROR)
		return;

	//
	// Continue sending a file
	//
	SendFileContents(lpReq);
}

////////////////////////////////////////////////////////////

void OnClose(SOCKET socket, int nErrorCode)
{
	LPREQUEST lpReq;

	//
	// Have we already deleted this entry?
	//
	lpReq = GetRequest(socket);
	if (lpReq == NULL)
		return;		

	// 
	// It's still in our list
	// The client must have reset the connection.
	// Clean up.
	//
	CloseConnection(lpReq);
}

////////////////////////////////////////////////////////////

void ParseRequest(LPREQUEST lpReq, LPBYTE lpBuf)
{
	char szFileName[_MAX_PATH];
	char szSeps[] = " \n";
	char *cpToken;

	if (strstr((const char*)lpBuf, "..") != NULL) {
		SendError(lpReq, HTTP_STATUS_BADREQUEST);
		CloseConnection(lpReq);
		return;	}

	cpToken = strtok((char*)lpBuf, szSeps);
	if (lstrcmpi(cpToken, "GET")==0)
	{
		lpReq->nMethod = METHOD_GET;
	}
	else if (lstrcmpi(cpToken, "POST")==0)
	{
		ParsePost(lpReq, lpBuf);
		lpReq->nMethod = METHOD_GET;
	}
	else
	{
		SendError(lpReq, HTTP_STATUS_NOTIMPLEMENTED);
		CloseConnection(lpReq);
		return;
	}

	cpToken = strtok(NULL, szSeps);
	if (cpToken == NULL)
	{
		SendError(lpReq, HTTP_STATUS_BADREQUEST);
		CloseConnection(lpReq);
		return;
	}

	if (lstrlen(cpToken) < 2) lstrcat(cpToken, "index.html");
	
	BuildFile(cpToken);

	int strLen = lstrlen(cpToken);
	for (int i=0; i<strLen; i++)
		if (cpToken[i] == '?') cpToken[i] = NULL;

	strcpy(szFileName, szWebRoot);
	strcat(szFileName, cpToken);

	SendFile(lpReq, szFileName);
}


void CloseConnection(LPREQUEST lpReq)
{
	closesocket(lpReq->Socket);
	if (lpReq->hFile != HFILE_ERROR) _lclose(lpReq->hFile);
	DelRequest(lpReq);
}


void SendFile(LPREQUEST lpReq, LPCSTR lpFileName)
{
	lpReq->hFile = _lopen(lpFileName, OF_READ|OF_SHARE_COMPAT);

	if (lpReq->hFile == HFILE_ERROR)
	{
		// Send "404 Not Found" error
		SendError(lpReq, HTTP_STATUS_NOTFOUND);
		CloseConnection(lpReq);
		return;
	}

	lpReq->dwFilePtr = 0L;
	SendFileContents(lpReq);
}


void SendError(LPREQUEST lpReq, UINT uError)
{
	int nRet;
	static char szMsg[512];
	static char *szStatMsgs [] = {
								"200 OK",
								"201 Created",
								"202 Accepted",
								"204 No Content",
								"301 Moved Permanently",
								"302 Moved Temporarily",
								"304 Not Modified",
								"400 Bad Request",
								"401 Unauthorized",
								"403 Forbidden",
								"404 Not Found",
								"500 Internal Server Error",
								"501 Not Implemented",
								"502 Bad Gateway",
								"503 Service Unavailable"
								};
	#define NUMSTATMSGS sizeof(szStatMsgs) / sizeof(szStatMsgs[0])

	if (uError < 0 || uError > NUMSTATMSGS) return;

	wsprintf(szMsg, "<body><h1>%s</h1></body>", szStatMsgs[uError]);
	nRet = send(lpReq->Socket, szMsg, strlen(szMsg), 0);
	if (nRet == SOCKET_ERROR)
		if (WSAGetLastError() != WSAEWOULDBLOCK)
			nRet = 0;
	lpReq->dwSend += nRet;
}


void SendFileContents(LPREQUEST lpReq)
{
	static BYTE buf[1024];
	UINT uBytes;
	BOOL fEof;
	int nBytesSent;

	fEof = FALSE;

	if (lpReq->dwFilePtr > 0)
		_llseek(lpReq->hFile, lpReq->dwFilePtr, FILE_BEGIN);


	while(1)
	{
		uBytes = _lread(lpReq->hFile, buf, sizeof(buf));
		if (uBytes == HFILE_ERROR)
		{
			// Send "500 Internal server" error
			SendError(lpReq, HTTP_STATUS_SERVERERROR);
			CloseConnection(lpReq);
			_lclose(lpReq->hFile);
			lpReq->hFile = HFILE_ERROR;
			return;
		}

		if (uBytes < sizeof(buf))
			fEof = TRUE;

		nBytesSent = send(lpReq->Socket, (char*)buf, uBytes, 0);
		if (nBytesSent == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSAEWOULDBLOCK)
			{
				CloseConnection(lpReq);
				_lclose(lpReq->hFile);
				lpReq->hFile = HFILE_ERROR;
				return;
			}
			nBytesSent = 0;
		}

		lpReq->dwFilePtr += nBytesSent;

		lpReq->dwSend += nBytesSent;

		if (nBytesSent < (int)uBytes) return;

		if (fEof)
		{
			CloseConnection(lpReq);
			_lclose(lpReq->hFile);
			lpReq->hFile = HFILE_ERROR;
			return;
		}
	}
}


int GetLocalAddress(LPSTR lpStr, LPDWORD lpdwStrLen)
{
	struct in_addr *pinAddr;
	LPHOSTENT	lpHostEnt;
	int			nRet;
	int			nLen;

	nRet = gethostname(lpStr, *lpdwStrLen);
	if (nRet == SOCKET_ERROR) {
		lpStr[0] = '\0';
		return SOCKET_ERROR; }

	lpHostEnt = gethostbyname(lpStr);
	if (lpHostEnt == NULL) {
		lpStr[0] = '\0';
		return SOCKET_ERROR; }

	pinAddr = ((LPIN_ADDR)lpHostEnt->h_addr);
	nLen = strlen(inet_ntoa(*pinAddr));
	if ((DWORD)nLen > *lpdwStrLen) {
		*lpdwStrLen = nLen;
		WSASetLastError(WSAEINVAL);
		return SOCKET_ERROR; }

	*lpdwStrLen = nLen;
	strcpy(lpStr, inet_ntoa(*pinAddr));
	return 0;
}


void ParsePost(LPREQUEST lpReq, LPBYTE lpBuf)
{
//	cpToken = strtok((char*)lpBuf, szSeps);
	char line[1024];
	char cmd[256];
	char sep[] = "\n";

	ZeroMemory(line, sizeof(line));
	ZeroMemory(cmd, sizeof(cmd));

}
#endif
