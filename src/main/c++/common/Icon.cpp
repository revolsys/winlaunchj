/*******************************************************************************
 * This program and the accompanying materials
 * are made available under the terms of the Common Public License v1.0
 * which accompanies this distribution, and is available at 
 * http://www.eclipse.org/legal/cpl-v10.html
 * 
 * Contributors:
 *     Peter Smith
 *******************************************************************************/

#include "common/Icon.h"
#include "common/Log.h"

// Since we can't replace the icon resource while the program is running we do the following:
//  1. copy exe to a random filename.
//  2. execute random filename with the argument containing the original filename
//  3. replace the icon of the original filename
//  4. execute the original to delete the random filename

// WinRun4J.exe --WinRun4J:SetIcon
// WinRun4J.Random.exe --WinRun4J:SetIcon SetIcon WinRun4J.exe
// WinRun4J.exe --seticon Delete WinRun4J.Random.exe 

#define SET_ICON_CMD               "--WinRun4J:SetIcon SetIcon"
#define SET_ICON_DELETE_EXE_CMD    "--WinRun4J:SetIcon Delete"
#define ADD_ICON_CMD               "--WinRun4J:AddIcon AddIcon"
#define ADD_ICON_DELETE_EXE_CMD    "--WinRun4J:AddIcon Delete"
#define REMOVE_ICON_CMD            "--WinRun4J:RemoveIcon RemoveIcon"
#define REMOVE_ICON_DELETE_EXE_CMD "--WinRun4J:RemoveIcon Delete"


void Icon::SetExeIcon(LPSTR commandLine)
{
	// Work out which operation
	if(strncmp(commandLine, SET_ICON_CMD, strlen(SET_ICON_CMD)) == 0) {
		SetIcon(commandLine);
	} else if(strncmp(commandLine, SET_ICON_DELETE_EXE_CMD, strlen(SET_ICON_DELETE_EXE_CMD)) == 0) {
		DeleteRandomFile(commandLine);
	} else {
		CopyToRandomAndRun(SET_ICON_CMD);
	}
}

void Icon::SetIcon(LPSTR commandLine)
{
	// Assume the ico is named "appname.ico"
	Sleep(1000);
	TCHAR filename[MAX_PATH], iconfile[MAX_PATH];
	GetFilenames(commandLine, filename, iconfile);	

	// Now set the icon
	SetIcon(filename, iconfile);

	// Now delete the random file
	RunDeleteRandom(filename, SET_ICON_DELETE_EXE_CMD);
}

void Icon::GetFilenames(LPSTR commandLine, LPSTR filename, LPSTR iconfile)
{
	// Extract original file from commandline
	commandLine = StripArg0(commandLine);
	commandLine = StripArg0(commandLine);
	strcpy(filename, commandLine);

	// Make icon filename
	strcpy(iconfile, filename);
	int len = strlen(filename);
	iconfile[len - 1] = 'o';
	iconfile[len - 2] = 'c';
	iconfile[len - 3] = 'i';

	Log::Info("Setting icon file...");
	Log::Info("Icon File: %s", iconfile);
	Log::Info("Exe File: %s", filename);
}

void Icon::RunDeleteRandom(LPSTR filename, LPSTR command)
{
	// Create command line for deleting random file
	Sleep(1000);
	TCHAR random[MAX_PATH], cmd[MAX_PATH];
	GetModuleFileName(NULL, random, MAX_PATH);
	sprintf(cmd, "\"%s\" %s %s", filename, command, random);

	// Now delete the random exe
	STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );
	if(!CreateProcess(filename, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		Log::Error("Could not run delete process");
}

// Create a random filename based on original and call set icon on this executable
void Icon::CopyToRandomAndRun(LPSTR command) 
{
	TCHAR filename[MAX_PATH], random[MAX_PATH], cmdline[MAX_PATH];
	GetModuleFileName(NULL, filename, sizeof(filename));
	srand(GetTickCount());
	int r = rand();
	sprintf(random, "%s.%d.exe", filename, r);
	sprintf(cmdline, "\"%s\" %s %s", random, command, filename);
	if(!CopyFile(filename, random, true)) {
		return;
	}

	STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );
	if(!CreateProcess(random, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) 
		Log::Error("Could not run random process");
}

// Delete the random filename
void Icon::DeleteRandomFile(LPSTR cmdLine)
{
	cmdLine = StripArg0(cmdLine);
	cmdLine = StripArg0(cmdLine);
	Sleep(1000);
	DeleteFile(cmdLine);
}

// Set icon on original exe file
bool Icon::SetIcon(LPSTR exeFile, LPSTR iconFile)
{
	// Read icon file
	ICONHEADER* pHeader;
	ICONIMAGE** pIcons;
	GRPICONHEADER* pGrpHeader;
	bool res = LoadIcon(iconFile, pHeader, pIcons, pGrpHeader);
	if(!res) {
		return false;
	}

	// Copy in resources
	HANDLE hUpdate = BeginUpdateResource(exeFile, FALSE);

	// Copy in icon group resource
	UpdateResource(hUpdate, RT_GROUP_ICON, MAKEINTRESOURCE(1), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		pGrpHeader, sizeof(WORD)*3+pHeader->count*sizeof(GRPICONENTRY));

	// Copy in icons
	for(int i = 0; i < pHeader->count; i++) {
		UpdateResource(hUpdate, RT_ICON, MAKEINTRESOURCE(i + 1), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
			pIcons[i], pHeader->entries[i].bytesInRes);
	}

	// Commit the changes
	EndUpdateResource(hUpdate, FALSE);

	return true;
}

// Load an icon image from a file
bool Icon::LoadIcon(LPSTR iconFile, ICONHEADER*& pHeader, ICONIMAGE**& pIcons, GRPICONHEADER*& pGrpHeader)
{
	HANDLE hFile = CreateFile(TEXT(iconFile), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE) {
		Log::Error("ERROR: Could not open icon file: %s", iconFile);
		return false;
	}

	pHeader = (ICONHEADER*) malloc(sizeof(ICONHEADER));
	DWORD bytesRead;
	ReadFile(hFile, &(pHeader->reserved), sizeof(WORD), &bytesRead, NULL);
	ReadFile(hFile, &(pHeader->type), sizeof(WORD), &bytesRead, NULL);
	ReadFile(hFile, &(pHeader->count), sizeof(WORD), &bytesRead, NULL);
	pHeader = (ICONHEADER*) realloc(pHeader, sizeof(WORD)*3+sizeof(ICONENTRY)*pHeader->count);
	ReadFile(hFile, pHeader->entries, pHeader->count*sizeof(ICONENTRY), &bytesRead, NULL);
	pIcons = (ICONIMAGE**) malloc(sizeof(ICONIMAGE*)*pHeader->count);
	for(int i = 0 ; i < pHeader->count; i++) {
		pIcons[i] = (ICONIMAGE*) malloc(pHeader->entries[i].bytesInRes);
		SetFilePointer(hFile, pHeader->entries[i].imageOffset, NULL, FILE_BEGIN);
		ReadFile(hFile, pIcons[i], pHeader->entries[i].bytesInRes, &bytesRead, NULL);
	}

	// Convert to resource format
	pGrpHeader = (GRPICONHEADER*) malloc(sizeof(WORD)*3+pHeader->count*sizeof(GRPICONENTRY));
	pGrpHeader->reserved = 0;
	pGrpHeader->type = 1;
	pGrpHeader->count = pHeader->count;
	for(int i = 0; i < pHeader->count; i++) {
		ICONENTRY* icon = &pHeader->entries[i];
		GRPICONENTRY* entry = &pGrpHeader->entries[i];
		entry->bitCount = 0;
		entry->bytesInRes = icon->bitCount;
		entry->bytesInRes2 = (WORD)icon->bytesInRes;
		entry->colourCount = icon->colorCount;
		entry->height = icon->height;
		entry->id = (WORD)(i+1);
		entry->planes = (BYTE)icon->planes;
		entry->reserved = icon->reserved;
		entry->width = icon->width;
		entry->reserved2 = 0;
	}

	// Close handles
	CloseHandle(hFile);

	return true;
}

void Icon::AddExeIcon(LPSTR commandLine)
{
	// Work out which operation
	if(strncmp(commandLine, ADD_ICON_CMD, strlen(ADD_ICON_CMD)) == 0) {
		AddIcon(commandLine);
	} else if(strncmp(commandLine, ADD_ICON_DELETE_EXE_CMD, strlen(ADD_ICON_DELETE_EXE_CMD)) == 0) {
		DeleteRandomFile(commandLine);
	} else {
		CopyToRandomAndRun(ADD_ICON_CMD);
	}
}

void Icon::AddIcon(LPSTR commandLine)
{
	// Assume the ico is named "appname.ico"
	TCHAR filename[MAX_PATH], iconfile[MAX_PATH];
	GetFilenames(commandLine, filename, iconfile);	

	// Now set the icon
	AddIcon(filename, iconfile);

	// Now delete the random file
	RunDeleteRandom(filename, ADD_ICON_DELETE_EXE_CMD);
}


bool Icon::AddIcon(LPSTR exeFile, LPSTR iconFile)
{
	// Read icon file
	ICONHEADER* pHeader;
	ICONIMAGE** pIcons;
	GRPICONHEADER* pGrpHeader;
	bool res = LoadIcon(iconFile, pHeader, pIcons, pGrpHeader);
	if(!res) {
		return false;
	}

	// Copy in resources
	HANDLE hUpdate = BeginUpdateResource(exeFile, FALSE);

	// Find next resource id
	int nextId = FindNextId((HMODULE) hUpdate);

	// Copy in icon group resource
	UpdateResource(hUpdate, RT_GROUP_ICON, MAKEINTRESOURCE(nextId++), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		pGrpHeader, sizeof(WORD)*3+pHeader->count*sizeof(GRPICONENTRY));

	// Copy in icons
	for(int i = 0; i < pHeader->count; i++) {
		UpdateResource(hUpdate, RT_ICON, MAKEINTRESOURCE(i + nextId + 1), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
			pIcons[i], pHeader->entries[i].bytesInRes);
	}

	// Commit the changes
	EndUpdateResource(hUpdate, FALSE);

	return false;
}

void Icon::RemoveExeIcons(LPSTR commandLine)
{
	// Work out which operation
	if(strncmp(commandLine, REMOVE_ICON_CMD, strlen(REMOVE_ICON_CMD)) == 0) {
		RemoveIcons(commandLine);
	} else if(strncmp(commandLine, REMOVE_ICON_DELETE_EXE_CMD, strlen(REMOVE_ICON_DELETE_EXE_CMD)) == 0) {
		DeleteRandomFile(commandLine);
	} else {
		CopyToRandomAndRun(REMOVE_ICON_CMD);
	}
}

void Icon::RemoveIcons(LPSTR commandLine)
{
	// Assume the ico is named "appname.ico"
	TCHAR filename[MAX_PATH], iconfile[MAX_PATH];
	GetFilenames(commandLine, filename, iconfile);	

	// Now set the icon
	RemoveIconResources(filename);

	// Now delete the random file
	RunDeleteRandom(filename, REMOVE_ICON_DELETE_EXE_CMD);
}

bool Icon::RemoveIconResources(LPSTR exeFile)
{
	HANDLE hUpdate = BeginUpdateResource(exeFile, FALSE);

	for(int i = 1; i < 1000; i++) {
		HRSRC hsrc = FindResource((HMODULE) hUpdate, MAKEINTRESOURCE(i), RT_GROUP_ICON);
		if(hsrc != NULL) {
			UpdateResource(hUpdate, RT_GROUP_ICON, MAKEINTRESOURCE(i), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), 0, 0);
		} 

		hsrc = FindResource((HMODULE) hUpdate, MAKEINTRESOURCE(i), RT_ICON);
		if(hsrc != NULL) {
			UpdateResource(hUpdate, RT_ICON, MAKEINTRESOURCE(i), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), 0, 0);
		} 
	}

	EndUpdateResource(hUpdate, FALSE);
	return false;
}

int Icon::FindNextId(HMODULE hModule)
{
	for(int i = 1; ; i++) {
		HRSRC hsrc = FindResource(hModule, MAKEINTRESOURCE(i), RT_GROUP_ICON);
		if(hsrc == NULL) {
			hsrc = FindResource(hModule, MAKEINTRESOURCE(i), RT_ICON);
		}
		if(hsrc == NULL)
			return i;
	}
}
