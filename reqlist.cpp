#if 0
#include "httpa.h"
#include "../../../../vBase/list.h"
//static LPREQUEST lpHead = NULL;

void FreeRequest(LPREQUEST lpReq);


vList <LPREQUEST>list;



LPREQUEST	AddRequest(SOCKET Socket, LPSOCKADDR lpSockAddr, int nAddrLen)
{
	LPREQUEST req = new REQUEST();
	req->lpSockAddr = new SOCKADDR();

	req->dwConnectTime = GetTickCount();
	req->Socket = Socket;
	req->hFile = HFILE_ERROR;
//	req->lpNext = NULL;
	req->fFullResponse = FALSE;
	req->dwRecv = 0;
	req->dwSend = 0;
	memcpy(req->lpSockAddr, lpSockAddr, nAddrLen);

	list.addToEnd(req);

	return req;
}


LPREQUEST GetRequest(SOCKET Socket) 
{
	LPREQUEST item = list.getFirst();
	if (item == NULL) return NULL;

	while (1)
	{
		if (item->Socket == Socket) return item;
		item = list.getNext(item);
		if (item == NULL) break;
	}
	return NULL;
}


void DelRequest(LPREQUEST lpThis)
{
	list.delItem(lpThis);
	FreeRequest(lpThis);
}


LPREQUEST	GetFirstRequest() { return list.getFirst(); }
LPREQUEST	GetNextRequest(LPREQUEST lpThis) { return list.getNext(lpThis); }


void DelAllRequests()
{
	while (list.hasItems())
	{
		LPREQUEST item = list.getFirst();
		FreeRequest(item);
		list.delFirst();
	}
}


void FreeRequest(LPREQUEST lpReq)
{
	free(lpReq->lpSockAddr);
	free(lpReq);
}

/*
	lpThis->dwConnectTime = GetTickCount();
	lpThis->Socket = Socket;
	lpThis->hFile = HFILE_ERROR;
	lpThis->lpNext = NULL;
	lpThis->fFullResponse = FALSE;
	lpThis->dwRecv = 0;;
	lpThis->dwSend = 0;
	memcpy(lpThis->lpSockAddr, lpSockAddr, nAddrLen);
	return (lpThis);
*/
/*
	LPREQUEST lpReq = NULL;
	LPREQUEST lpThis = NULL;

	lpThis = (LPREQUEST)malloc(sizeof(REQUEST));
	if (lpThis == NULL)
		return(NULL);

	lpThis->lpSockAddr = (LPSOCKADDR)malloc(nAddrLen);
	if (lpThis->lpSockAddr == NULL)
	{
		free(lpThis);
		return(NULL);
	}

	if (lpHead == NULL) 
	{
		lpHead = lpThis;
	}
	else 
	{
		lpReq = lpHead;
		while(lpReq->lpNext)
			lpReq = lpReq->lpNext;

		lpReq->lpNext = lpThis;
	}
  
	lpThis->dwConnectTime = GetTickCount();
	lpThis->Socket = Socket;
	lpThis->hFile = HFILE_ERROR;
	lpThis->lpNext = NULL;
	lpThis->fFullResponse = FALSE;
	lpThis->dwRecv = 0;;
	lpThis->dwSend = 0;
	memcpy(lpThis->lpSockAddr, lpSockAddr, nAddrLen);
	return (lpThis);
}*/

/*
LPREQUEST GetRequest(SOCKET Socket) 
{
	LPREQUEST lpReq;
  
	lpReq = lpHead;
	while(lpReq != NULL)
	{
		if (lpReq->Socket == Socket) break;
		lpReq = lpReq->lpNext;
	}

	return(lpReq);
}


void DelRequest(LPREQUEST lpThis) 
{
	LPREQUEST lpReq;
	BOOL bRet = FALSE;


	if (lpThis == lpHead) 
	{
		lpHead = lpThis->lpNext;
	}
	else 
	{  
		for (lpReq = lpHead; lpReq; lpReq = lpReq->lpNext) 
		{
			if (lpReq->lpNext == lpThis)
			{
				lpReq->lpNext = lpThis->lpNext;
				break;
			}
		}
	}

	FreeRequest(lpThis);
}


LPREQUEST	GetFirstRequest() { return(lpHead); }
LPREQUEST	GetNextRequest(LPREQUEST lpThis) { return(lpThis->lpNext); }


void DelAllRequests(void)
{
	LPREQUEST lpReq;
	LPREQUEST lpNext;

	for (lpReq = lpHead; lpReq; ) 
	{
		lpNext = lpReq->lpNext;
		FreeRequest(lpReq);
		lpReq = lpNext;
	}
	lpHead = NULL;
}


static void FreeRequest(LPREQUEST lpReq)
{
	free(lpReq->lpSockAddr);
	free(lpReq);
}
*/
#endif