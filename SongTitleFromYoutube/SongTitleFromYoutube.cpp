// YouTubeTitleReader.cpp : Defines the entry point for the console application.
//

#include <stdafx.h>
#include <string>
#include <iostream>
#include <fstream>
#include <windows.h>
#include <chrono>
#include <thread>
#include <wchar.h>
#include <stdlib.h>
#include <codecvt>

//#define DEBUG
//#define UTF_16

using namespace std;

#ifdef UTF_16
wostream& wendl(wostream& out){
	return out.put(L'\r').put(L'\n').flush();
}

const codecvt_mode le_bom = static_cast<codecvt_mode>(std::little_endian | std::generate_header | std::consume_header);
typedef codecvt_utf16<wchar_t, 0x10ffff, le_bom> wcvt_utf16le_bom;
#endif // UTF-16

typedef struct enumWindowCallbackParams {
	HWND hYoutubeWindow;
	bool isYoutubeWindowCatched;
	int interval;
	wofstream *file;
	wstring filename;
	wstring title;
} EWCP;

#ifdef UTF_16
void writeSongTitle(EWCP* ewcp, wstring title_text) {
	wcvt_utf16le_bom cvt(1);
	locale wloc((*ewcp->file).getloc(), &cvt);
	(*ewcp->file).imbue(wloc);
	(*ewcp->file).open(ewcp->filename, ios::out | ios::binary | ios::trunc);
	(*ewcp->file) << title_text << L"                         " << wendl;
	(*ewcp->file).close();
}
#else
void writeSongTitle(EWCP* ewcp, wstring title_text) {
	(*ewcp->file).open(ewcp->filename, ios::out | ios::trunc);
	(*ewcp->file) << title_text << L"                         ";
	(*ewcp->file).close();
}
#endif // UTF_16

static BOOL CALLBACK enumWindowCallback(HWND hWnd, LPARAM lparam) {
	EWCP* ewcp = (EWCP*)lparam;

	if (ewcp->isYoutubeWindowCatched) {
		if (ewcp->hYoutubeWindow == hWnd) {
			int length = GetWindowTextLength(hWnd);
			wchar_t* buffer = new wchar_t[length + 1];
			GetWindowText(hWnd, buffer, length + 1);
			wstring windowTitle = buffer;

			if (IsWindowVisible(hWnd) && length != 0) {
				if (windowTitle == ewcp->title) {
					delete buffer;
					return FALSE;
				}
				size_t position;
				if (!(string::npos == (position = windowTitle.find(L" - YouTube")))) {
					ewcp->title = windowTitle;
					writeSongTitle(ewcp, windowTitle.erase(position, windowTitle.length() - position));

					#ifdef DEBUG
					wcout << windowTitle.erase(position, windowTitle.length() - position) << wendl;
					#endif // DEBUG

					delete buffer;
					return FALSE;
				}
				ewcp->isYoutubeWindowCatched = false;
			}

			delete buffer;
			return TRUE;
		}
	}
	else {
		int length = GetWindowTextLength(hWnd);
		wchar_t* buffer = new wchar_t[length + 1];
		GetWindowText(hWnd, buffer, length + 1);
		wstring windowTitle(buffer);

		if (IsWindowVisible(hWnd) && length != 0) {
			size_t position;
			if (!(string::npos == (position = windowTitle.find(L" - YouTube")))) {
				ewcp->hYoutubeWindow = hWnd;
				ewcp->isYoutubeWindowCatched = true;
				ewcp->title = windowTitle;
				writeSongTitle(ewcp, windowTitle.erase(position, windowTitle.length() - position));

				#ifdef DEBUG
				wcout << windowTitle.erase(position, windowTitle.length() - position) << wendl;
				#endif // DEBUG

				delete buffer;
				return FALSE;
			}
		}

		delete buffer;
		return TRUE;
	}
}

void listenTitleChangeSync(LPARAM lparam) {
	EWCP* ewcp = (EWCP*)lparam;
	while (true) {
		EnumWindows(enumWindowCallback, lparam);
		this_thread::sleep_for(chrono::milliseconds(((EWCP*)lparam)->interval));
	}
}

void parseArgs(EWCP* ewcp, int argc, wchar_t *wargv[]) {
	try {

		ewcp->interval = 5000;
		ewcp->filename = L"songtitle.txt";

		if (argc < 4) {
			if (argc > 1) {
				ewcp->interval = _wtoi(wargv[1]);

				if (ewcp->interval < 1)
					throw exception("Not proper interval argument");
			}

			if (argc > 2) {
				ewcp->filename = wargv[2];
			}
		}

		ewcp->file = new wofstream(ewcp->filename, wofstream::out | wofstream::trunc);
		if (!(*ewcp->file).is_open()) {
			throw exception("File wasn't created. Try another file name.");
		}
		ewcp->title = L"Looking for YouTube window...   ";
		(*ewcp->file) << ewcp->title;
		(*ewcp->file).close();
	}
	catch (const exception &e) {
		throw e;
	}
}

int wmain(int argc, wchar_t *wargv[]) {
	EWCP* ewcp = new EWCP{ NULL, false, -1, NULL };

	try {
		parseArgs(ewcp, argc, wargv);
		listenTitleChangeSync((LPARAM)ewcp);
	}
	catch (const exception &e) {
		delete ewcp->file;
		delete ewcp;
		wcerr << "Error: " << e.what() << endl;
		wcerr << L"Note: Proper execution structure: SingleTitleFromYoutube.exe <milliseconds> <filename>";
		cin.ignore();
		return 1;
	}
	catch (...) {
		delete ewcp->file;
		delete ewcp;
		wcerr << "Unknown error occurred." << endl;
		wcerr << L"Note: Proper argument strutcutre for execution: SingleTitleFromYoutube.exe <milliseconds> <filename>";
		cin.ignore();
		return 1;
	}

	delete ewcp->file;
	delete ewcp;
	return 0;
}

