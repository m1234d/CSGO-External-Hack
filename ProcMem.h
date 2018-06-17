#ifndef PROCMEM_H
#define PROCMEM_H

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <string>
#include <sstream>

class ProcMem{
protected:


	HANDLE hProcess;
	DWORD dwPID, dwProtection, dwCaveAddress;


	BOOL bPOn, bIOn, bProt;

public:


	ProcMem();
	~ProcMem();
	int chArraySize(char *chArray);
	int iArraySize(int *iArray);
	bool iFind(int *iAry, int iVal);

#pragma region TEMPLATE MEMORY FUNCTIONS





	template <class cData>
	cData Read(DWORD dwAddress)
	{
		cData cRead;
		ReadProcessMemory(hProcess, (LPVOID)dwAddress, &cRead, sizeof(cData), NULL);
		return cRead;
	}


	template <class cData>
	cData Read(DWORD dwAddress, char *Offset, BOOL Type)
	{

		int iSize = iArraySize(Offset) - 1;
		dwAddress = Read<DWORD>(dwAddress);


		for (int i = 0; i < iSize; i++)
			dwAddress = Read<DWORD>(dwAddress + Offset[i]);

		if (!Type)
			return dwAddress + Offset[iSize];
		else
			return Read<cData>(dwAddress + Offset[iSize]);
	}


	template <class cData>
	void Read(DWORD dwAddress, cData Value)
	{
		ReadProcessMemory(hProcess, (LPVOID)dwAddress, &Value, sizeof(cData), NULL);
	}


	template <class cData>
	void Read(DWORD dwAddress, char *Offset, cData Value)
	{
		Read<cData>(Read<cData>(dwAddress, Offset, false), Value);
	}


	virtual void Process(char* ProcessName);
	virtual void Patch(DWORD dwAddress, char *chPatch_Bts, char *chDefault_Bts);
	virtual DWORD AOB_Scan(DWORD dwAddress, DWORD dwEnd, char *chPattern);
	virtual DWORD Module(LPSTR ModuleName);

#pragma endregion

};
#endif

