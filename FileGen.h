#if 0
#ifndef __VFILEGEN_H__
#define __VFILEGEN_H__

#include "../playlist3/playlist3.h"


class vFileGen : public vPlaylist
{
public:
	vFileGen();

	void BuildFile(char *filename);

protected:
	void parseFile(char *fn, char *filename);
	void parseOptions(char *opt);
	void evalOption(char *opt);

	void writePlaylistName();
	void writePlayingStatus();
	void writePlaylist();
	void writeListPlaylist();
	void writeSearch();
	void writeThemes();


private:
	FILE *fRead;
	FILE *fWrite;

	char viewList[256];
};

#endif
#endif