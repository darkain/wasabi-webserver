#pragma comment(lib, "WS2_32.lib")

#include "../../bfc/std.h"
#include "WebServer.h"
#include "resource.h"
#include "../../common/xlatstr.h"
#include "../Playlist3/Playlist3.H"

#include <stdio.h>


static WACNAME wac;
WACPARENT *the = &wac;                     
vPlaylist3 *pl = NULL;


// {77D14A84-9700-4551-8AA1-6CC4E93C264B}
static const GUID guid =
{ 0x77d14a84, 0x9700, 0x4551, { 0x8a, 0xa1, 0x6c, 0xc4, 0xe9, 0x3c, 0x26, 0x4b } };


WACNAME::WACNAME() : WACPARENT("Wasabi-WebServer") {
	hInst = NULL;
	hWnd = NULL;
	listener = NULL;
	ZeroMemory(path, sizeof(path));
	ZeroMemory(&sockAddr, sizeof(SOCKADDR_IN));
}

WACNAME::~WACNAME() {}

GUID WACNAME::getGUID() {
	return guid;
}

void WACNAME::onCreate() {
	pl = new vPlaylist3();
	hInst = gethInstance();

	WORD wVersionRequested = MAKEWORD(1,1);
	WSADATA wsaData;

	int nRet = WSAStartup(wVersionRequested, &wsaData);
	if (nRet) {
		MessageBox(NULL, "Initialize WinSock Failed", getName(), MB_OK);
		return;
	}
	
	if (wsaData.wVersion != wVersionRequested) {       
		MessageBox(NULL, "Wrong WinSock Version", getName(), MB_OK);
		return;
	}

	hWnd = CreateDialog(hInst, MAKEINTRESOURCE(DLG1), NULL, (DLGPROC)WACNAME::wndProcTemp);

	startServer();
}


void WACNAME::onDestroy()
{
	stopServer();

	delete pl;
	pl = NULL;

	WSACleanup();	
}


void WACNAME::startServer() {
	int nRet;			

	listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listener == INVALID_SOCKET) return;

	sockAddr.sin_port = htons(8080);
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = INADDR_ANY;

	nRet = bind(listener, (LPSOCKADDR)&sockAddr, sizeof(struct sockaddr));
	if (nRet == SOCKET_ERROR) {
		closesocket(listener);
		return;
	}

	nRet = listen(listener, 1);
	if (nRet == SOCKET_ERROR) {
		closesocket(listener);
		return;
	}

	nRet = WSAAsyncSelect(listener, hWnd, WM_USER+1, FD_ACCEPT | FD_READ | FD_WRITE | FD_CLOSE);
	if (nRet == SOCKET_ERROR) {
		closesocket(listener);
		return;
	}
}


void WACNAME::stopServer() {
	closesocket(listener);

	LPvRequest lpReq = getFirst();
	while(lpReq) {
		closesocket(lpReq->Socket);
		if (lpReq->file) FCLOSE(lpReq->file);
		lpReq = getNext(lpReq);
	}
}


void WACNAME::onAccept(SOCKET socket, int nErrorCode) {
	SOCKADDR_IN	newSockAddr;
	SOCKET newSocket;
	int nRet;
	int nLen;

	nLen = sizeof(SOCKADDR_IN);
	newSocket = accept(listener, (LPSOCKADDR)&newSockAddr, &nLen);
	if (newSocket == SOCKET_ERROR) {
		nRet = WSAGetLastError();
		if (nRet != WSAEWOULDBLOCK) return;
	}

	nRet = WSAAsyncSelect(newSocket, hWnd, WM_USER+1, FD_READ | FD_WRITE | FD_CLOSE);
	if (newSocket == SOCKET_ERROR) {
		nRet = WSAGetLastError();
		return;
	}

	LPvRequest request = addRequest(newSocket, (LPSOCKADDR)&newSockAddr, nLen);
	if (request == NULL) closesocket(newSocket);
}


void WACNAME::onRead(SOCKET socket, int nErrorCode) {
	static BYTE buf[2048];
	int nRet;

	ZeroMemory(buf, sizeof(buf));

	LPvRequest lpReq = getRequest(socket);
	if (lpReq == NULL) {
		nRet = 0;
		while(nRet != SOCKET_ERROR) nRet = recv(socket, (char*)buf, sizeof(buf)-1, 0);
		closesocket(socket);
		return;
	}

	nRet = recv(socket, (char*)buf, sizeof(buf)-1, 0);
	if (nRet == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAEWOULDBLOCK) return;
		closeConnection(lpReq);
		return;
	}

	lpReq->dwRecv += nRet;
	parseRequest(lpReq, buf);
}


void WACNAME::onWrite(SOCKET socket, int nErrorCode) {
	BYTE buf[1024];
	LPvRequest lpReq = getRequest(socket);
	if (lpReq == NULL) {
		int nRet = 0;
		while(nRet != SOCKET_ERROR) nRet = recv(socket, (char*)buf, sizeof(buf)-1, 0);
		closesocket(socket);
		return;		
	}

	if (lpReq->file == NULL) return;

	sendFileContents(lpReq);
}


void WACNAME::onClose(SOCKET socket, int nErrorCode) {
	LPvRequest lpReq = getRequest(socket);
	if (lpReq == NULL) return;		
	closeConnection(lpReq);
}


void WACNAME::closeConnection(LPvRequest lpReq) {
	closesocket(lpReq->Socket);
	if (lpReq->file) FCLOSE(lpReq->file);
	delItem(lpReq);
}


LPvRequest WACNAME::addRequest(SOCKET Socket, LPSOCKADDR lpSockAddr, int nAddrLen) {
	LPvRequest req = new vRequest();
	req->lpSockAddr = new SOCKADDR();

	req->dwConnectTime = GetTickCount();
	req->Socket = Socket;
	req->file = NULL;
	req->fFullResponse = FALSE;
	req->dwRecv = 0;
	req->dwSend = 0;
	memcpy(req->lpSockAddr, lpSockAddr, nAddrLen);

	addToEnd(req);
	return req;
}


LPvRequest WACNAME::getRequest(SOCKET Socket) 
{
	LPvRequest item = getFirst();
	while (item) {
		if (item->Socket == Socket) return item;
		item = getNext(item);
	}
	return NULL;
}


LRESULT CALLBACK WACNAME::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) 
	{
		case WM_INITDIALOG:
			return TRUE;

		case WM_USER+1: {
				int nErrorCode = WSAGETSELECTERROR(lParam);
				switch(WSAGETSELECTEVENT(lParam)) {
					case FD_ACCEPT: onAccept((SOCKET)wParam, nErrorCode); break;
					case FD_READ: onRead((SOCKET)wParam, nErrorCode);	break;
					case FD_WRITE: onWrite((SOCKET)wParam, nErrorCode); break;
					case FD_CLOSE: onClose((SOCKET)wParam, nErrorCode); break;
				}
			}
			return TRUE;
	}
	return 0;
}



LRESULT CALLBACK WACNAME::wndProcTemp(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (the) return wac.wndProc(hwnd, msg, wParam, lParam);
	return 0;
}


const char *WACNAME::getPath() {
	if (*path) return path;
	GetModuleFileName(hInst, path, sizeof(path)-1);
	char *p = path + lstrlen(path);
	while (p >= path && *p != '\\') p--;
	if (p >= path) *p = 0;
	return path;
}


void WACNAME::parseRequest(LPvRequest lpReq, LPBYTE lpBuf) {
	char szFileName[_MAX_PATH];
	char szSeps[] = " \n";
	char *cpToken;

	if (strstr((const char*)lpBuf, "..") != NULL) {
		sendError(lpReq, HTTP_STATUS_BADREQUEST);
		closeConnection(lpReq);
		return;	}

	cpToken = strtok((char*)lpBuf, szSeps);
	if (lstrcmpi(cpToken, "GET")==0) lpReq->nMethod = METHOD_GET;
	else if (lstrcmpi(cpToken, "POST")==0) {
		parsePost(lpReq, lpBuf);
		lpReq->nMethod = METHOD_GET;
	}
	else {
		sendError(lpReq, HTTP_STATUS_NOTIMPLEMENTED);
		closeConnection(lpReq);
		return;
	}

	cpToken = strtok(NULL, szSeps);
	if (cpToken == NULL) {
		sendError(lpReq, HTTP_STATUS_BADREQUEST);
		closeConnection(lpReq);
		return;
	}

	if (lstrlen(cpToken) < 2) lstrcat(cpToken, "index.html");
	
	strcpy(szFileName, getPath());

	char *pPath = szFileName;
	while (*pPath != NULL) pPath++;

	char *rootPath = cpToken;
	while (*rootPath != NULL) {
		if (*rootPath == '%') {
			int num=0;
			rootPath++;
			if (*rootPath == NULL) break;
			SSCANF(rootPath, "%02X", &num);
			*pPath = num;
			pPath++;
			*pPath = NULL;
			rootPath++;
			if (*rootPath == NULL) break;
		}
		else {
			*pPath = *rootPath;
			pPath++;
			*pPath = NULL;
		}
		rootPath++;
	}
/*
	char root[_MAX_PATH];
	pPath = szFileName;
	rootPath = root;

	while ( (*pPath != NULL) & (*pPath != '/') & (*pPath != '\\') ) {
		*rootPath = *pPath;
		rootPath++;
		pPath++;
	}
	*rootPath == NULL;

	if (strcmpi(root, "LOGS") != 0) pPath = szFileName;
*/
	
//	BuildFile(cpToken);

	int strLen = lstrlen(cpToken);
	for (int i=0; i<strLen; i++) if (cpToken[i] == '?') cpToken[i] = NULL;

//	strcat(szFileName, cpToken);

	sendFile(lpReq, szFileName);
}


void WACNAME::sendFile(LPvRequest lpReq, LPCSTR lpFileName) {
//	lpReq->hFile = _lopen(lpFileName, OF_READ|OF_SHARE_COMPAT);
	lpReq->file = FOPEN(lpFileName, "r");

	if (lpReq->file == NULL) {
		// Send "404 Not Found" error
		sendError(lpReq, HTTP_STATUS_NOTFOUND);
		closeConnection(lpReq);
		return;
	}

	lpReq->dwFilePtr = 0L;
	sendFileContents(lpReq);
}


void WACNAME::sendError(LPvRequest lpReq, UINT uError) {
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
	if (nRet == SOCKET_ERROR) if (WSAGetLastError() != WSAEWOULDBLOCK) nRet = 0;
	lpReq->dwSend += nRet;
}


void WACNAME::sendFileContents(LPvRequest lpReq) {
	static BYTE buf[1024];
	UINT uBytes;
	BOOL fEof;
	int nBytesSent;

	fEof = FALSE;

	if (lpReq->dwFilePtr > 0) FSEEK(lpReq->file, lpReq->dwFilePtr, SEEK_SET);

	while(1) {
		uBytes = FREAD(buf, 1, sizeof(buf), lpReq->file);
		if (uBytes == HFILE_ERROR) {
			// Send "500 Internal server" error
			sendError(lpReq, HTTP_STATUS_SERVERERROR);
			closeConnection(lpReq);
			FCLOSE(lpReq->file);
			lpReq->file = NULL;
			return;
		}

		if (uBytes < sizeof(buf)) fEof = TRUE;

		nBytesSent = send(lpReq->Socket, (char*)buf, uBytes, 0);
		if (nBytesSent == SOCKET_ERROR) {
			if (WSAGetLastError() != WSAEWOULDBLOCK) {
				closeConnection(lpReq);
				FCLOSE(lpReq->file);
				lpReq->file = NULL;
				return;
			}
			nBytesSent = 0;
		}

		lpReq->dwFilePtr += nBytesSent;

		lpReq->dwSend += nBytesSent;

		if (nBytesSent < (int)uBytes) return;

		if (fEof) {
			FCLOSE(lpReq->file);
			lpReq->file = NULL;
			closeConnection(lpReq);
			return;
		}
	}
}


void WACNAME::parsePost(LPvRequest lpReq, LPBYTE lpBuf)
{
//	cpToken = strtok((char*)lpBuf, szSeps);
	char line[1024];
	char cmd[256];
	char sep[] = "\n";

	ZeroMemory(line, sizeof(line));
	ZeroMemory(cmd, sizeof(cmd));

}