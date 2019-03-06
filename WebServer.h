#ifndef _WEBSERVER_H
#define _WEBSERVER_H

#include "../../studio/wac.h"
#include "../../../../vBase/List.H"
#include <winsock.h>


#define WACNAME WACGeneric
#define WACPARENT WAComponentClient


typedef struct tagREQUEST
{
  SOCKET		Socket;
  LPSOCKADDR	lpSockAddr;
  int				nAddrLen;
  int				nMethod;
  BOOL			fFullResponse;
  DWORD			dwConnectTime;
  DWORD			dwRecv;
  DWORD			dwSend;
  FILE			*file;
  DWORD			dwFilePtr;
} vRequest, *LPvRequest;


class WACNAME : public WACPARENT, vList<LPvRequest> {
public:
  WACNAME();
  virtual ~WACNAME();

  virtual GUID getGUID();

  virtual void onCreate();
  virtual void onDestroy();

	void startServer();
	void stopServer();

	const char *getPath();

	virtual void onAccept(SOCKET socket, int nErrorCode);
	virtual void onRead(SOCKET socket, int nErrorCode);
	virtual void onWrite(SOCKET socket, int nErrorCode);
	virtual void onClose(SOCKET socket, int nErrorCode);

	void closeConnection(LPvRequest lpReq);

	LPvRequest addRequest(SOCKET Socket, LPSOCKADDR lpSockAddr, int nAddrLen);
	LPvRequest getRequest(SOCKET Socket);
	virtual void onDelete(LPvRequest data) { delete data; }

	void parseRequest(LPvRequest lpReq, LPBYTE lpBuf);
	void parsePost(LPvRequest lpReq, LPBYTE lpBuf);
	void sendFile(LPvRequest lpReq, LPCSTR lpFileName);
	void sendError(LPvRequest lpReq, UINT uError);
	void sendFileContents(LPvRequest lpReq);

	LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndProcTemp(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	HINSTANCE hInst;
	HWND hWnd;
	char path[256];

	SOCKET listener;
	SOCKADDR_IN sockAddr;
};

extern WACPARENT *the;


#define METHOD_GET 0

#define HTTP_STATUS_OK				0
#define HTTP_STATUS_CREATED			1
#define HTTP_STATUS_ACCEPTED		2
#define HTTP_STATUS_NOCONTENT		3
#define HTTP_STATUS_MOVEDPERM		4
#define HTTP_STATUS_MOVEDTEMP		5
#define HTTP_STATUS_NOTMODIFIED		6
#define HTTP_STATUS_BADREQUEST		7
#define HTTP_STATUS_UNAUTHORIZED	8
#define HTTP_STATUS_FORBIDDEN		9
#define HTTP_STATUS_NOTFOUND		10
#define HTTP_STATUS_SERVERERROR		11
#define HTTP_STATUS_NOTIMPLEMENTED	12
#define HTTP_STATUS_BADGATEWAY		13
#define HTTP_STATUS_UNAVAILABLE		14


#endif _WEBSERVER_H
