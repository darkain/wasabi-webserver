#if 0
#include "../../common/metatags.h"
#include "../../bfc/string.h"
#include "../../bfc/db/scanner.h"
#include "httpa.h"
#include "filegen.h"
#include "../../../../vBase/base.h"
#include <stdio.h>


int vBase::msgBox(const char *txt, const char *title) { return 0; }


bool xstrcmp(char *txt1, char *txt2, int len=-1)
{
	if (len == -1) len = lstrlen(txt2);
	char tmp = txt1[len];

	bool val = false;
	for (int i=0; i<len; i++)
		if (txt1[i] != txt2[i])
		{
			txt1[len] = tmp;
			return false;
		}

	txt1[len] = tmp;
	return true;
}


char *parse(char *txt, char *begin, char *end, int *first, int *last, bool caseSense=false)
{
	static char rtrnTxt[4096];
	if (txt == NULL) return false;
	if (begin == NULL) return false;
	if (end == NULL) return false;

	int fChar = 0;
	int i = 0;

	if (first) fChar = *first;

	int bLen = lstrlen(begin);
	int eLen = lstrlen(end);

	bool foundBegin = false;
/*
	while (1)
	{
		if (lChar == 0)
			if (txt[i+first] == 0x00) return false;

//		if (caseSense)
		{
			if (foundBeing == false)
			{
				if (xstrcmp(txt, begin, bLen)
				{
					if (first != NULL) *first = fChar;
					foundBegin = true;
				}
			}
			
			else
			{
				if (xstrcmp(txt, end, eLen)
				if (last != NULL) *last = fChar+i;
				return true;
			}
		}

		i++;
	}
*/
	return false;
}



void BuildFile(char *filename)
{
	vFileGen file;
	file.BuildFile(filename);
}


vFileGen::vFileGen()
{
	fRead = NULL;
	fWrite = NULL;
	ZeroMemory(viewList, sizeof(viewList));
}


void vFileGen::writePlaylistName()
{
	if (fWrite == NULL) return;
//	int PLID = getPlFromName(viewList);
	fprintf(fWrite, getPlaylistName());
}


void vFileGen::writePlayingStatus()
{
	if (fWrite == NULL) return;
	
	CoreHandle core;
	const char *cur = currentTrack();

	if (cur)
	{
		char status[10] = "???";
		switch (core.getStatus())
		{
		case -1:	lstrcpy(status, "Paused");	break;
		case 1:		lstrcpy(status, "Playing");	break;
		default:	lstrcpy(status, "Stopped");
		}
		fprintf(fWrite, "Playing: %s<BR>\nPlaylist: %s<BR>\nStatus: %s", cur, getPlaylistName(), status);
	}
	else
		fprintf(fWrite, "Playing: nothing");
}


void vFileGen::writePlaylist()
{
	if (fWrite == NULL) return;

	PlaylistHandle handle = getPlaylistHandle();
	batchBegin(handle);
		
	int len = getPlaylistLength(handle)+1;
//	const char *pl = getPlaylistName(handle);

	for (int i=1; i<len; i++)
		fprintf(fWrite, "<A HREF=\"index.html?action=PLAY&playlist=%08X&item=%d#%d\" NAME=\"%d\"> %d - %s</A><BR>\n", handle, i, i, i, i, batchGetItem(i-1));

	batchEnd();
}


void vFileGen::writeListPlaylist()
{
	if (fWrite == NULL) return;

	int len = getNumPlaylists();
	for (int i=0; i<len; i++)
	{
		const char *listName = getPlaylistName(getPlaylistHandle(i));
		fprintf(fWrite, "<A HREF=\"index.html?VIEWLIST=%s\">%s</A><BR>\n", listName, listName);
	}
}


void vFileGen::writeSearch()
{
	if (fWrite == NULL) return;

	char name[] = "DDR";
	char query[4096] = "(";
	char tmp[256];
	fprintf(fWrite, "Searching for: %s<BR><BR>\n", name);

	sprintf(tmp, "(Playstring HAS \"%s\"", name);
	lstrcat(query, tmp);
	lstrcat(query, ") OR ");
	sprintf(tmp, "(Name HAS \"%s\"", name);
	lstrcat(query, tmp);
	lstrcat(query, ") )");

//	String query2 = String(MT_ARTIST) + " == \"" + name + "\"";
//	MessageBox(0, query, query2, 0);
	
  dbScanner scan(api->main_getGUID());
  scan.setQuery(query);
  for (scan.first(); !scan.eof(); scan.next())
	{
		const char *str = scan.getPlaystring();
    fprintf(fWrite, "<A HREF=\"Search.html?PlaySearch=%s\">%s</A><BR>\n", str, str);
	}
}
/*
void DumpArtists(char *artist) {
  dbScanner scan(api->main_getGUID());
  String query = String(MT_ARTIST) + " == \"" + artist + "\"";
  scan.setQuery(query);
  for (scan.first(); !scan.eof(); scan.next()) {
    OutputDebugString(scan.getPlaystring());
  }
}
*/

void vFileGen::writeThemes()
{
	if (fWrite == NULL) return;
	fprintf(fWrite, "Beta<BR>\n");
	fprintf(fWrite, "Wasabi<BR>\n");
}


void vFileGen::BuildFile(char *filename)
{
	if (filename == NULL) return;
	if (lstrlen(filename) < 1) return;
	if (filename[0] == '/') filename++;

//MessageBox(0, filename, filename, 0);

	int i;
	char fn[MAX_PATH];
	ZeroMemory(fn, sizeof(fn));

	int strLen = lstrlen(filename);

	{
		int curChar = 0;
		for (i=0; i<strLen; i++)
		{
			if (filename[i] == '%')
			{
				if ( (filename[i+1] == '2') & (filename[i+2] == '0') )
				{
					fn[curChar++] = ' ';
					i+=2;
				}
				else 
					fn[curChar++] = filename[i];
			}
			else
				fn[curChar++] = filename[i];
		}
		fn[curChar] = NULL;
		strLen = curChar;
	}


	for (i=0; i<strLen; i++)
	{
		if (fn[i] == '?')
		{
			fn[i] = NULL;
			parseOptions(fn+i+1);
			i = strLen;
		}
	}

	parseFile(fn, filename);
}


void vFileGen::parseFile(char *fn, char *filename)
{
	int strLen = lstrlen(fn);

	if (lstrcmpi(fn+strLen-5, ".html") == 0)
	{
		char path1[MAX_PATH];
		char path2[MAX_PATH];
		lstrcpy(path1, getPath());
		lstrcat(path1, fn);
		lstrcpy(path2, path1);
		lstrcat(path2, ".Template");


		fRead = fopen(path2, "r");
		if (fRead == NULL) return;

		fWrite = fopen(path1, "w");

		if (fWrite != FALSE)
		{
			int i;
			char buffer[1024];
			while ( !feof(fRead) )
			{
				int len = fread(buffer, sizeof(char), sizeof(buffer)-1, fRead);
				buffer[len] = NULL;

				int firstChar=0;

				for (i=0; i<len-1; i++)
				{
					if (buffer[i] == '%')
					{
						int len = fwrite(buffer+firstChar, sizeof(char), i-firstChar, fWrite);

						switch (buffer[i+1])
						{
						case 'n':		writePlaylistName();		break;
						case 't':		writePlayingStatus();		break;
						case 'p':		writePlaylist();				break;
						case 'l':		writeListPlaylist();		break;
						case 's':		writeSearch();					break;
						case 'h':		writeThemes();					break;
						case 'f':		fprintf(fWrite, "%s", filename);	break;
						case 'F':		fprintf(fWrite, "%s", fn);				break;
						default:		fwrite(buffer+firstChar+len, sizeof(char), 2, fWrite);
						}
						i+=2;
						firstChar = i;
					}
				}
				fwrite(buffer+firstChar, sizeof(char), len-firstChar, fWrite);
			}
		}
		fclose(fRead);
		fclose(fWrite);
	}
}


void vFileGen::parseOptions(char *opt)
{
	if (opt == NULL) return;
	int strLen = lstrlen(opt);
	if (strLen < 1) return;
	if (strLen >= MAX_PATH) strLen = MAX_PATH-1;

	int i, i2;
	int firstChar = 0;
	char txt[MAX_PATH];
	
	for (i=0; i<strLen; i++)
	{
		switch (opt[i])
		{
		case '&':
		case '?':
			{
				for (i2=0; i2<i-firstChar; i2++)
					txt[i2] = opt[firstChar+i2];
				txt[i2] = NULL;
				evalOption(txt);
				firstChar = ++i;
			}
			break;
		}
	}
	if (firstChar<strLen)
	{
		evalOption(opt+firstChar);
	}
}


void vFileGen::evalOption(char *opt)
{
	char option[256];
	int strLen = lstrlen(opt);
	if (strLen < 1) return;
	if (strLen > 255) strLen = 255;

	int i;
	for (i=0; i<strLen; i++)
	{
		if (opt[i] == '=')
		{
			option[i] = NULL;
			break;
		}
		option[i] = opt[i];
	}

	char *option2 = opt+i+1;

	if (lstrcmpi(option, "ACTION") == 0)
	{
		if (lstrcmpi(option2, "PLAY") == 0)
		{
//			CoreHandle core;
//			core.play();
		}

		else if (lstrcmpi(option2, "PAUSE") == 0)
		{
//			CoreHandle core;
//			core.pause();
		}

		else if (lstrcmpi(option2, "STOP") == 0)
		{
//			CoreHandle core;
//			core.stop();
		}

		else if (lstrcmpi(option2, "PREV") == 0)
		{
//			CoreHandle core;
//			core.prev();
		}


		else if (lstrcmpi(option2, "NEXT") == 0)
		{
//			CoreHandle core;
//			core.next();
		}
	}

	else if (lstrcmpi(option, "PLAYLIST") == 0)
	{
		setPlaylist((char*)option2);
	}

	else if (lstrcmpi(option, "ITEM") == 0)
	{
		int item=-1;
		int read = sscanf(option2, "%d", &item);
		if (read < 1) return;
		if (item < 0) return;
		playSong(item-1);
	}

	else if (lstrcmpi(option, "VIEWLIST") == 0)
	{
		lstrcpy(viewList, option2);
	}

	else if (lstrcmpi(option, "PLAYSEARCH") == 0)
	{
//			CoreHandle core;
//			api->main_openFile(option2);
//			core.next();
//			core.play();
	}

	else if (lstrcmpi(option, "CREATELIST") == 0)
	{
		//create a new playlist n stuffzorz

	}

	Sleep(10);
}

/*
void parseOptions(char *opt)
{
	int strLen = lstrlen(opt);
	if (strLen<1) return;
	if (strLen>1023) strLen = 1023;

	int i, i2;
	int firstChar = 0;
	char txt[1024];
	
	for (i=0; i<strLen; i++)
	{
		switch (opt[i])
		{
		case '&':
		case '?':
			{
				for (i2=0; i2<i-firstChar; i2++)
					txt[i2] = opt[firstChar+i2];
				txt[i2] = NULL;
				evalOption(txt);
				firstChar = ++i;
			}
			break;
		}
	}
	if (firstChar<strLen)
	{
		evalOption(opt+firstChar);
	}
}






void BuildFile(char *filename)
{
	int i;
	char fn[1024];

	if (filename == NULL) return;
	if (filename[0] == '/') filename++;

	int strLen = lstrlen(filename);

	viewList[0] = NULL;
	int curChar = 0;

	for (i=0; i<strLen; i++)
	{
		if (filename[i] == '%')
		{
			if ( (filename[i+1] == '2') & (filename[i+2] == '0') )
			{
				fn[curChar++] = ' ';
				i+=2;
			}
			else 
				fn[curChar++] = filename[i];
		}
		else
			fn[curChar++] = filename[i];
	}
	fn[curChar] = NULL;

	strLen = lstrlen(fn);

	for (i=0; i<strLen; i++)
	{
		if (fn[i] == '?')
		{
			fn[i] = NULL;
			parseOptions(fn+i+1);
			i = strLen;
		}
	}

	strLen = lstrlen(fn);

	if (lstrcmpi(fn+strLen-5, ".html") == 0)
	{
		char path1[1024];
		char path2[1024];
		lstrcpy(path1, getPath());
		lstrcat(path1, fn);
		lstrcpy(path2, path1);
		lstrcat(path2, ".Template");


		FILE *file2 = fopen(path2, "r");
		if (file2 == NULL) return;

		FILE *file1 = fopen(path1, "w");

		if (file1 != FALSE)
		{
			char buffer[1024];
			while ( !feof(file2) )
			{
				int len = fread(buffer, sizeof(char), sizeof(buffer)-1, file2);
				buffer[len] = NULL;

				int firstChar=0;

				for (i=0; i<len-1; i++)
				{
					if (buffer[i] == '%')
					{
						int len = fwrite(buffer+firstChar, sizeof(char), i-firstChar, file1);

						switch (buffer[i+1])
						{
						case 'n':		writePlaylistName(file1);		break;
						case 't':		writePlayingStatus(file1);	break;
						case 'p':		writePlaylist(file1);				break;
						case 'l':		writeListPlaylist(file1);		break;
						case 's':		writeSearch(file1);					break;
						default:		fwrite(buffer+firstChar+len, sizeof(char), 2, file1);
						}
						i+=2;
						firstChar = i;
					}
				}
				fwrite(buffer+firstChar, sizeof(char), len-firstChar, file1);
			}
		}
		fclose(file2);
		fclose(file1);
	}
}
*/

/*
void parseOptions(char *opt);

char viewList[1024] = "";


svc_plDir *getPlService()
{
	waServiceFactory* pFactory = api->service_getServiceByGuid( 
	nsGUID::fromChar( "{B240D9A9-A39D-410D-BFC7-22856AD9F722}" ) );

	if ( pFactory )
	{
		svc_plDir *pls = castService<svc_plDir>(pFactory);
		return pls;
	}
	return NULL;
}


void playlistPosition(int item)
{
	svc_plDir *pls = getPlService();
	if (pls == NULL) return;

	Playlist *pl = pls->getPlaylist(pls->getCurrentlyPlaying());
	//pl->setCurrent(item);

	CoreHandle core;
	if (core.getStatus() == 0) pl->startPlayback(item, FALSE);
	else pl->startPlayback(item, TRUE);
}


int getPlaylistFromName(char *name)
{
	svc_plDir *pls = getPlService();
	if (pls == NULL) return -1;

	if (name == NULL) return -1;
	if (lstrlen(name) < 1) return -1;

	int i;
	for (i=0; i<pls->getNumPlaylists(); i++)
	{
		if (lstrcmpi(pls->getPlaylistLabel(pls->enumPlaylist(i)), name) == 0)
			return i;
	}
	
	return -1;
}


const char *getPlaylistName(int ID=-1)
{
	svc_plDir *pls = getPlService();
	if (pls == NULL) return NULL;

	if (ID == -1)
		{
			if (playList == NULL)
				return pls->getPlaylist(pls->getCurrentlyPlaying())->getPlaylistName();
			else
				return playList->getPlaylistName();
		}
	else
		return pls->getPlaylist(pls->enumPlaylist(ID))->getPlaylistName();
}


int PlaylistLength(int ID=-1)
{
	svc_plDir *pls = getPlService();
	if (pls == NULL) return NULL;

	if (ID == -1)
		{
			if (playList == NULL)
				return pls->getPlaylist(pls->getCurrentlyPlaying())->getNumEntries();
			else
				return playList->getNumEntries();
		}
	else
		return pls->getPlaylist(pls->enumPlaylist(ID))->getNumEntries();
}


int numPlaylists()
{
	svc_plDir *pls = getPlService();
	if (pls == NULL) return -1;
	return pls->getNumPlaylists();		
}


void switchToPlaylist(char *plName)
{
	if (plName == NULL) return;
	int strLen = lstrlen(plName);
	if (strLen < 1) return;

	{
		CoreHandle core;
		svc_plDir *pls = getPlService();
		if (pls == NULL) return;

		int i;
		for (i=0; i<pls->getNumPlaylists(); i++)
		{
			if (lstrcmpi(pls->getPlaylistLabel(pls->enumPlaylist(i)), plName) == 0)
			{
				pls->setCurrentlyOpen(pls->enumPlaylist(i));
				Playlist *pl = pls->getPlaylist(pls->enumPlaylist(i));
				pl->startPlayback(0);

				if (core.getStatus() == 0)
					core.stop();

				i = pls->getNumPlaylists();
			}
		}
	}
}


BOOL batchPLBegin(int ID=-1)
{
	svc_plDir *pls = getPlService();

	if (ID == -1)
		playList = pls->getPlaylist(pls->getCurrentlyPlaying());
	else
		playList = pls->getPlaylist(pls->enumPlaylist(ID));
	return (playList != NULL);
}


void batchPLEnd() { playList = NULL; }


char *batchgetPLItem(int ID)
{
	if (playList == NULL) return NULL;

	PlaylistEntry* pEntry = playList->enumEntry(ID);
	const char* pPlaystring = playList->getPlayStringFromEntry(pEntry);
	api->metadb_getMetaData(pPlaystring, MT_NAME, plCache, sizeof(plCache), MDT_STRINGZ);
	return plCache;
}


char *batchgetPLItemFile(int ID)
{
	if (playList == NULL) return NULL;
	return (char*)playList->enumItem(ID);
}


*/
/*
void writePlaylistName(FILE *file)
{
	if (file == NULL) return;

	int PLID = getPlaylistFromName(viewList);
	fprintf(file, getPlaylistName(PLID));

//	const char *plName = getPlaylistName();
//	int strLen = lstrlen(plName);
//	fwrite(plName, sizeof(char), strLen, file);
}


void writePlayingStatus(FILE *file)
{
	if (file == NULL) return;
	
	CoreHandle core;

	const char *cur = core.getCurrent();
	char status[32];

	switch (core.getStatus())// // returns -1 if paused, 0 if stopped and 1 if playing
	{
	case -1:	lstrcpy(status, "Paused");	break;
	case 1:		lstrcpy(status, "Playing");	break;
	default:	lstrcpy(status, "Stopped");
	}
		
	if (cur)
		fprintf(file, "Playing: %s<BR>\nPlaylist: %s<BR>\nStatus: %s", cur, getPlaylistName(), status);
	else
		fprintf(file, "Playing: nothing");
}


void writePlaylist(FILE *file)
{
	if (file == NULL) return;

	int PLID = getPlaylistFromName(viewList);

	batchPLBegin(PLID);
		
	int len = PlaylistLength()+1;
	const char *pl = getPlaylistName();

	for (int i=1; i<len; i++)
		fprintf(file, "<A HREF=\"index.html?action=PLAY&playlist=%s&item=%d\"> %d - %s</A><BR>\n", pl, i, i, batchgetPLItem(i-1));

	batchPLEnd();
}


void writeListPlaylist(FILE *file)
{
	if (file == NULL) return;

	for (int i=0; i<numPlaylists(); i++)
	{
		const char *listName = getPlaylistName(i);
		fprintf(file, "<A HREF=\"index.html?VIEWLIST=%s\">%s</A><BR>\n", listName, listName);
	}
}


void writeSearch(FILE *file)
{
	if (file == NULL) return;
//	char search[1024] = "darkain";
}


*/
#endif