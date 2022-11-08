// inject.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <Windows.h>
#include <stdint.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <vector>


// 导入表注入
// 计算对齐的函数，如偏移为900，对齐为1000h，返回1000h
DWORD Align(DWORD dwOffset, DWORD dwAlign)
{
	// 如果偏移小于对齐，向上取整
	if (dwOffset <= dwAlign) return dwAlign;
	// 如果偏移大于对齐且不能除尽，向上取整
	if (dwOffset % dwAlign)
	{
		return (dwOffset / dwAlign + 1) * dwAlign;
	}
	// 如果能除尽，直接返回offset
	return dwOffset;
}

// FOA 转 RVA
DWORD FoaToRva(LPVOID pFileBuffer, DWORD dwFoa)
{
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pFileBuffer;
	PIMAGE_FILE_HEADER pPEHeader = (PIMAGE_FILE_HEADER)(pDosHeader->e_lfanew + (DWORD)pFileBuffer + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionHeader = (PIMAGE_OPTIONAL_HEADER32)((DWORD)pPEHeader + sizeof(IMAGE_FILE_HEADER));
	PIMAGE_SECTION_HEADER pSectionHeader = \
		(PIMAGE_SECTION_HEADER)((DWORD)pOptionHeader + pPEHeader->SizeOfOptionalHeader);

	// RVA在文件头中或者文件对齐==内存对齐时，RVA==FOA  错！第一句是对的，第二句是错的
	if (dwFoa < pOptionHeader->SizeOfHeaders)
	{
		return dwFoa;
	}

	// 遍历节表，确定偏移属于哪一个节	
	for (int i = 0; i < pPEHeader->NumberOfSections; i++)
	{
		if (dwFoa >= pSectionHeader[i].PointerToRawData && \
			dwFoa < pSectionHeader[i].PointerToRawData + pSectionHeader[i].SizeOfRawData)
		{
			int offset = dwFoa - pSectionHeader[i].PointerToRawData;
			return pSectionHeader[i].VirtualAddress + offset;
		}
	}
	printf("找不到FOA %x 对应的 RVA，转换失败\n", dwFoa);
	return 0;
}

// RVA 转 FOA
DWORD RvaToFoa(LPVOID pFileBuffer, DWORD dwRva)
{
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pFileBuffer;
	PIMAGE_FILE_HEADER pPEHeader = (PIMAGE_FILE_HEADER)(pDosHeader->e_lfanew + (DWORD)pFileBuffer + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionHeader = (PIMAGE_OPTIONAL_HEADER32)((DWORD)pPEHeader + sizeof(IMAGE_FILE_HEADER));
	PIMAGE_SECTION_HEADER pSectionHeader = \
		(PIMAGE_SECTION_HEADER)((DWORD)pOptionHeader + pPEHeader->SizeOfOptionalHeader);

	// RVA在文件头中或者文件对齐==内存对齐时，RVA==FOA  错！第一句是对的，第二句是错的
	if (dwRva < pOptionHeader->SizeOfHeaders)
	{
		return dwRva;
	}

	// 遍历节表，确定偏移属于哪一个节	
	for (int i = 0; i < pPEHeader->NumberOfSections; i++)
	{
		if (dwRva >= pSectionHeader[i].VirtualAddress && \
			dwRva < pSectionHeader[i].VirtualAddress + pSectionHeader[i].Misc.VirtualSize)
		{
			int offset = dwRva - pSectionHeader[i].VirtualAddress;
			return pSectionHeader[i].PointerToRawData + offset;
		}
	}
	printf("找不到RVA %x 对应的 FOA，转换失败\n", dwRva);
	return 0;
}

// 移动NT头和节表到DOS STUB，该函数在新增节时节表空间不足的情况下调用；返回地址减小值
DWORD MoveNTHeaderAndSectionHeadersToDosStub(LPVOID pFileBuffer)
{
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pFileBuffer;
	PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)((DWORD)pDosHeader + pDosHeader->e_lfanew);
	PIMAGE_FILE_HEADER pPEHeader = (PIMAGE_FILE_HEADER)((DWORD)pDosHeader + pDosHeader->e_lfanew + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionHeader = (PIMAGE_OPTIONAL_HEADER32)((DWORD)pPEHeader + sizeof(IMAGE_FILE_HEADER));
	PIMAGE_SECTION_HEADER pSectionHeader = \
		(PIMAGE_SECTION_HEADER)((DWORD)pOptionHeader + pPEHeader->SizeOfOptionalHeader);

	LPVOID pDst = (LPVOID)((DWORD)pDosHeader + sizeof(IMAGE_DOS_HEADER)); // NT头插入点
	DWORD dwRet = (DWORD)pNTHeader - (DWORD)pDst; // 返回地址减小的值
	DWORD dwSize = 4 + sizeof(IMAGE_FILE_HEADER) + pPEHeader->SizeOfOptionalHeader + \
		sizeof(IMAGE_SECTION_HEADER) * pPEHeader->NumberOfSections; // 移动的字节数
	LPVOID pSrc = malloc(dwSize);
	if (pSrc == NULL)
	{
		printf("分配内存失败\n");
		return 0;
	}
	memcpy(pSrc, (LPVOID)pNTHeader, dwSize); // 保存要复制的数据
	memset((LPVOID)pNTHeader, 0, dwSize); // 清空原数据
	memcpy(pDst, pSrc, dwSize); // 移动数据
	free(pSrc);
	pDosHeader->e_lfanew = sizeof(IMAGE_DOS_HEADER); // 更新 e_lfanew

	return dwRet;
}

// 新增一个大小为 newSectionSize 的代码节
// dwFileBufferSize 是原来的文件大小
// 返回新缓冲区的大小，失败返回0
DWORD AddCodeSection(LPVOID pFileBuffer, LPVOID* pNewFileBuffer, DWORD dwFileBufferSize, DWORD dwNewSectionSize)
{
	// 复制一份 pFileBuffer，不要修改原来的数据
	LPVOID pFileBuffer2 = malloc(dwFileBufferSize);
	memcpy(pFileBuffer2, pFileBuffer, dwFileBufferSize);
	pFileBuffer = pFileBuffer2;

	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pFileBuffer;
	PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)((DWORD)pDosHeader + pDosHeader->e_lfanew);
	PIMAGE_FILE_HEADER pPEHeader = (PIMAGE_FILE_HEADER)((DWORD)pDosHeader + pDosHeader->e_lfanew + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionHeader = (PIMAGE_OPTIONAL_HEADER32)((DWORD)pPEHeader + sizeof(IMAGE_FILE_HEADER));
	PIMAGE_SECTION_HEADER pSectionHeader = \
		(PIMAGE_SECTION_HEADER)((DWORD)pOptionHeader + pPEHeader->SizeOfOptionalHeader);

	PWORD pNumberOfSections = &(pPEHeader->NumberOfSections); // 节的数量
	PIMAGE_SECTION_HEADER pLastSectionHeader = pSectionHeader + *pNumberOfSections - 1; // 最后一个节表
	PIMAGE_SECTION_HEADER pNewSectionHeader = pSectionHeader + *pNumberOfSections; // 新节表插入点
	DWORD newFileBufferSize = 0; // 新文件的大小

	// 判断最后一个节表后面是否有空闲的80字节
	if (80 > (DWORD)pFileBuffer + pOptionHeader->SizeOfHeaders - (DWORD)pNewSectionHeader)
	{
		std::cout << (DWORD)pFileBuffer + pOptionHeader->SizeOfHeaders - (DWORD)pNewSectionHeader << std::endl;
		printf("没有足够的80字节插入新节表\n");
		free(pFileBuffer2);
		return 0;
	}

	// 判断空闲的80字节是否全为0，如果不是，则把整个NT头往上挪覆盖dos stub以空出空间插入节表
	for (int i = 0; i < 80; i++)
	{
		if (((PBYTE)pNewSectionHeader)[i] != 0)
		{
			DWORD dwRet = MoveNTHeaderAndSectionHeadersToDosStub(pFileBuffer);
			printf("节表空间不足，NT头和节表向低地址移动了 %d 字节\n", dwRet);
			if (dwRet < 80)
			{
				printf("移动后仍没有足够的80字节空间插入新节表\n");
				free(pFileBuffer2);
				return 0;
			}
			// 更新指针
			pNTHeader = (PIMAGE_NT_HEADERS)((DWORD)pDosHeader + pDosHeader->e_lfanew);
			pPEHeader = (PIMAGE_FILE_HEADER)((DWORD)pDosHeader + pDosHeader->e_lfanew + 4);
			pOptionHeader = (PIMAGE_OPTIONAL_HEADER32)((DWORD)pPEHeader + sizeof(IMAGE_FILE_HEADER));
			pSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD)pOptionHeader + pPEHeader->SizeOfOptionalHeader);
			pNumberOfSections = &(pPEHeader->NumberOfSections); // 节的数量
			pLastSectionHeader = pSectionHeader + *pNumberOfSections - 1; // 最后一个节表
			pNewSectionHeader = pSectionHeader + *pNumberOfSections; // 新节表插入点
			break;
		}
	}

	// 定义一个 IMAGE_SECTION_HEADER 结构，计算里面的属性
	IMAGE_SECTION_HEADER newSectionHeader;
	memcpy(newSectionHeader.Name, ".newsec", 8);
	newSectionHeader.Misc.VirtualSize = Align(dwNewSectionSize, pOptionHeader->SectionAlignment);
	newSectionHeader.VirtualAddress = pLastSectionHeader->VirtualAddress + \
		Align(pLastSectionHeader->Misc.VirtualSize, pOptionHeader->SectionAlignment);
	newSectionHeader.SizeOfRawData = Align(dwNewSectionSize, pOptionHeader->FileAlignment);
	newSectionHeader.PointerToRawData = pLastSectionHeader->PointerToRawData + pLastSectionHeader->SizeOfRawData;
	newSectionHeader.PointerToRelocations = 0;
	newSectionHeader.PointerToLinenumbers = 0;
	newSectionHeader.NumberOfRelocations = 0;
	newSectionHeader.NumberOfLinenumbers = 0;
	newSectionHeader.Characteristics = 0x60000020;

	// pNewFileBuffer 分配内存，把 pFileBuffer 复制过去，后面的修改都在 pNewFileBuffer 进行
	*pNewFileBuffer = malloc(dwFileBufferSize + newSectionHeader.SizeOfRawData);
	memcpy(*pNewFileBuffer, pFileBuffer, dwFileBufferSize);
	memset((LPVOID)((DWORD)*pNewFileBuffer + dwFileBufferSize), 0, newSectionHeader.SizeOfRawData); // 新增节数据清0

	// 更新指针，指向新内存	
	pDosHeader = (PIMAGE_DOS_HEADER)*pNewFileBuffer;
	pNTHeader = (PIMAGE_NT_HEADERS)((DWORD)pDosHeader + pDosHeader->e_lfanew);
	pPEHeader = (PIMAGE_FILE_HEADER)((DWORD)pDosHeader + pDosHeader->e_lfanew + 4);
	pOptionHeader = (PIMAGE_OPTIONAL_HEADER32)((DWORD)pPEHeader + sizeof(IMAGE_FILE_HEADER));
	pSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD)pOptionHeader + pPEHeader->SizeOfOptionalHeader);
	pNumberOfSections = &(pPEHeader->NumberOfSections);
	pLastSectionHeader = pSectionHeader + *pNumberOfSections - 1;
	pNewSectionHeader = pSectionHeader + *pNumberOfSections;

	// 节的数量+1，SizeOfImage是内存中拉伸后的大小
	*pNumberOfSections += 1;
	pOptionHeader->SizeOfImage += Align(newSectionHeader.Misc.VirtualSize, pOptionHeader->SectionAlignment);

	// 拷贝 newSectionHeader
	memcpy(pNewSectionHeader, &newSectionHeader, sizeof(newSectionHeader));

	//printf("插入成功\n");
	free(pFileBuffer2);
	return dwFileBufferSize + newSectionHeader.SizeOfRawData;
}

// 导入表注入demo，通过修改导入表，将 InjectDll.dll 添加到导入表
// DLL只有一个导出函数 ExportFunction，保证至少有一个导出函数DLL才会被加载
// DLL的主函数在加载和分离时会弹窗
DWORD ImportTableInjectDemo(LPVOID pFileBuffer, LPVOID* pNewFileBuffer, DWORD dwFileSize, char *dllPath)
{
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pFileBuffer;
	PIMAGE_FILE_HEADER pPEHeader = (PIMAGE_FILE_HEADER)(pDosHeader->e_lfanew + (DWORD)pDosHeader + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionHeader = (PIMAGE_OPTIONAL_HEADER32)((DWORD)pPEHeader + sizeof(IMAGE_FILE_HEADER));
	PIMAGE_SECTION_HEADER pSectionHeader = \
		(PIMAGE_SECTION_HEADER)((DWORD)pOptionHeader + pPEHeader->SizeOfOptionalHeader);

	PIMAGE_IMPORT_DESCRIPTOR pImportTable = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD)pFileBuffer + \
		RvaToFoa(pFileBuffer, pOptionHeader->DataDirectory[1].VirtualAddress));

	// 计算新增节的大小
	// 新增节存储的内容有：原来的所有导入表，新导入表，新INT, IAT, 模块名，一个_IMAGE_IMPORT_BY_NAME
	// 上述容器按0为结束标记的，也要包含结束标记
	DWORD dwNewSectionSize = 0;
	DWORD dwNumberOfDll = 0;
	while (pImportTable->OriginalFirstThunk || pImportTable->FirstThunk)
	{
		//printf("%s\n", (LPCSTR)(RvaToFoa(pFileBuffer, pImportTable->Name) + (DWORD)pFileBuffer));
		dwNumberOfDll++;
		pImportTable++;
	}
	dwNewSectionSize += (dwNumberOfDll + 2) * sizeof(IMAGE_IMPORT_DESCRIPTOR); // 原有的和新添加的导入表，以及结束标记
	dwNewSectionSize += 16; // 这里包括一个INT,一个IAT和两个结束标记
	dwNewSectionSize += strlen(dllPath) + 1; // 模块名
	dwNewSectionSize += 2 + strlen("ExportFunction") + 1; // _IMAGE_IMPORT_BY_NAME，包括Hint和函数名
	DWORD dwNewFileSize = AddCodeSection(pFileBuffer, pNewFileBuffer, dwFileSize, dwNewSectionSize);

	pDosHeader = (PIMAGE_DOS_HEADER)*pNewFileBuffer;
	pPEHeader = (PIMAGE_FILE_HEADER)(pDosHeader->e_lfanew + (DWORD)pDosHeader + 4);
	pOptionHeader = (PIMAGE_OPTIONAL_HEADER32)((DWORD)pPEHeader + sizeof(IMAGE_FILE_HEADER));
	pSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD)pOptionHeader + pPEHeader->SizeOfOptionalHeader);
	pImportTable = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD)*pNewFileBuffer + \
		RvaToFoa(*pNewFileBuffer, pOptionHeader->DataDirectory[1].VirtualAddress));

	// 设置新增节属性
	pSectionHeader[pPEHeader->NumberOfSections - 1].Characteristics = 0xC0000040; // 可读写，含已初始化数据

	// 定义指针指向新增节首字节
	LPVOID pNewSec = (LPVOID)((DWORD)*pNewFileBuffer + pSectionHeader[pPEHeader->NumberOfSections - 1].PointerToRawData);
	LPVOID pInsert = pNewSec;
	// 复制原有的导入表
	memcpy(pInsert, pImportTable, dwNumberOfDll * sizeof(IMAGE_IMPORT_DESCRIPTOR));
	// 设置新导入表的时间戳，ForwarderChain
	pImportTable = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD)pInsert + dwNumberOfDll * sizeof(IMAGE_IMPORT_DESCRIPTOR));
	pImportTable->TimeDateStamp = 0; // 表示不使用绑定导入
	pImportTable->ForwarderChain = -1;
	// 设置导入表结束标记
	pInsert = (LPVOID)((DWORD)pImportTable + sizeof(IMAGE_IMPORT_DESCRIPTOR)); // 指向结束标记
	memset(pInsert, 0, sizeof(IMAGE_IMPORT_DESCRIPTOR));
	// 指定INT表插入点
	pInsert = (LPVOID)((DWORD)pInsert + sizeof(IMAGE_IMPORT_DESCRIPTOR)); // 现在指向INT表
	PIMAGE_THUNK_DATA pINT = (PIMAGE_THUNK_DATA)pInsert; // 定义指向INT表的指针
	// 设置INT结束标志
	memset(pINT + 1, 0, sizeof(IMAGE_THUNK_DATA));
	// 设置 IMPORT_BY_NAME，INT表和IAT表共同指向这块内存
	PIMAGE_IMPORT_BY_NAME pImportByName = (PIMAGE_IMPORT_BY_NAME)((DWORD)pINT + 8); // IMPORT_BY_NAME插入点
	pImportByName->Hint = 0; // 设置没有用的导出序号
	strcpy((char*)(pImportByName->Name), "ExportFunction"); // 设置函数名
	// INT表的值是 IMPORT_BY_NAME 的RVA
	*((PDWORD)pINT) = FoaToRva(*pNewFileBuffer, (DWORD)pImportByName - (DWORD)*pNewFileBuffer); // 设置INT的值
	// 设置IAT表
	pInsert = (LPVOID)((DWORD)pImportByName + 2 + strlen("ExportFunction") + 1); // 指向IAT表	
	PIMAGE_THUNK_DATA  pIAT = (PIMAGE_THUNK_DATA)pInsert; // IAT插入点
	memcpy(pIAT, pINT, 8);
	// 分配模块名的内存，并完成导入表剩余属性的赋值
	pInsert = (LPVOID)((DWORD)pInsert + 8);
	strcpy((char*)pInsert, dllPath);
	// 设置导入表属性
	pImportTable->FirstThunk = FoaToRva(*pNewFileBuffer, (DWORD)pINT - (DWORD)*pNewFileBuffer);
	pImportTable->OriginalFirstThunk = FoaToRva(*pNewFileBuffer, (DWORD)pIAT - (DWORD)*pNewFileBuffer);
	pImportTable->Name = FoaToRva(*pNewFileBuffer, (DWORD)pInsert - (DWORD)*pNewFileBuffer);
	// 更新目录项中导入表的位置
	pOptionHeader->DataDirectory[1].VirtualAddress = FoaToRva(*pNewFileBuffer, ((DWORD)pNewSec - (DWORD)*pNewFileBuffer));
	return dwNewFileSize;
}


void importTableInject(char *dst, char *dllPath)
{
	FILE* fp = fopen(dst, "rb");
	fseek(fp, 0, SEEK_END);
	int len = ftell(fp);
	unsigned char* buff = (unsigned char*)malloc((len + 200) * sizeof(char));
	unsigned char* newbuff = (unsigned char*)malloc((len + 200) * sizeof(char));
	rewind(fp);
	int l = fread(buff, 1, len, fp);
	ImportTableInjectDemo((void*)buff, (void**)&newbuff, len, dllPath);
	fp = freopen("FlappyBird_hook.exe", "wb", fp);
	fwrite(newbuff, 1, len + 200, fp);
	fclose(fp);
}

// 远程线程注入

//获取进程name的ID   
DWORD getPid(LPTSTR name)
{
	HANDLE hProcSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);//获取进程快照句柄   
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);
	BOOL flag = Process32First(hProcSnap, &pe32);//获取列表的第一个进程   
	while (flag)
	{
		if (!_tcscmp(pe32.szExeFile, name))
		{
			CloseHandle(hProcSnap);
			return pe32.th32ProcessID;//pid   
		}
		flag = Process32Next(hProcSnap, &pe32);//获取下一个进程   
	}
	CloseHandle(hProcSnap);
	return 0;
}

// 远程线程注入
bool remoteInjectDll(LPTSTR dst, LPCTSTR  szDllPath)
{
	DWORD dwPID = getPid(dst);
	HANDLE hProcess = NULL, hThread = NULL;
	HMODULE hMod = NULL;
	LPVOID pRemoteBuf = NULL;
	DWORD dwBufSize = (DWORD)(_tcslen(szDllPath) + 1) * sizeof(TCHAR);
	LPTHREAD_START_ROUTINE pThreadProc;
	// Open target process to inject dll
	if (!(hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID)))
	{
		_tprintf(L"Fail to open process %d ! [%d]\n", dwPID, GetLastError());
		return FALSE;
	}
	// Allocate memory in the remote process big enough for the DLL path name
	pRemoteBuf = VirtualAllocEx(hProcess, NULL, dwBufSize, MEM_COMMIT, PAGE_READWRITE);
	// Write the DLL path name to the space allocated in the target process
	WriteProcessMemory(hProcess, pRemoteBuf, (LPVOID)szDllPath, dwBufSize, NULL);
	// Find the address of LoadLibrary in target process(same to this process)
	hMod = GetModuleHandle(L"kernel32.dll");
	pThreadProc = (LPTHREAD_START_ROUTINE)GetProcAddress(hMod, "LoadLibraryW");
	// Create a remote thread in target process
	hThread = CreateRemoteThread(hProcess, NULL, 0, pThreadProc, pRemoteBuf, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
	CloseHandle(hProcess);
	return TRUE;
}

// 消息钩子注入
int setWindowHookEx_inject(WCHAR *dllPath)
{
	HWND hwnd = FindWindow(NULL, L"FlappyBird Configuration");
	if (hwnd == NULL)
	{
		std::cout << "FindWindow failed" << std::endl;
		return 0;
	}
	DWORD pid = NULL;
	DWORD tid = GetWindowThreadProcessId(hwnd, &pid);
	if (tid == NULL)
	{
		std::cout << "GetWindowThreadProcessId failed" << std::endl;
		return 0;
	}
	HMODULE dll = LoadLibraryEx(dllPath, NULL, DONT_RESOLVE_DLL_REFERENCES);
	if (dll == NULL)
	{
		std::cout << "LoadLibraryEx failed" << std::endl;
		return 0;
	}
	HOOKPROC addr = (HOOKPROC)GetProcAddress(dll, "DllInject");
	if (addr == NULL)
	{
		std::cout << "GetProcAddress failed" << std::endl;
		return 0;
	}
	HHOOK handle = SetWindowsHookEx(WH_GETMESSAGE, addr, dll, tid);
	if (handle == NULL)
	{
		std::cout << "SetWindowsHookEx failed" << std::endl;
		return 0;
	}
	PostThreadMessage(tid, WM_NULL, 0, 0);
	std::cout << "SetWindowsHookEx success" << std::endl;
	std::cout << "Press any key to exit" << std::endl;
	getchar();
	BOOL unhook = UnhookWindowsHookEx(handle);
	if (unhook == FALSE)
	{
		std::cout << "UnhookWindowsHookEx failed" << std::endl;
		return 0;
	}
	return 1;
}	

std::vector<DWORD> getTids(DWORD pid)
{
	std::vector<DWORD> tids;
	HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hThreadSnap == INVALID_HANDLE_VALUE)
	{
		std::cout << "CreateToolhelp32Snapshot failed" << std::endl;
		return tids;
	}
	THREADENTRY32 te32;
	te32.dwSize = sizeof(THREADENTRY32);
	if (!Thread32First(hThreadSnap, &te32))
	{
		std::cout << "Thread32First failed" << std::endl;
		CloseHandle(hThreadSnap);
		return tids;
	}
	do
	{
		if (te32.th32OwnerProcessID == pid)
		{
			tids.push_back(te32.th32ThreadID);
		}
	} while (Thread32Next(hThreadSnap, &te32));
	CloseHandle(hThreadSnap);
	return tids;
}


// APC 注入
void apc_inject(WCHAR* dst, WCHAR* dllPath)
{
	DWORD pid = getPid(dst);
	if (pid == 0)
	{
		std::cout << "getPid failed" << std::endl;
		return;
	}
	std::vector<DWORD> tids = getTids(pid);
	if (tids.size() == 0)
	{
		std::cout << "getTids failed" << std::endl;
		return;
	}
	
	HANDLE hProcess = OpenProcess(PROCESS_VM_WRITE|PROCESS_VM_OPERATION, FALSE, pid);
	auto p = VirtualAllocEx(hProcess, NULL, 1 << 12, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	WriteProcessMemory(hProcess, p, dllPath, 2 * wcslen(dllPath) + 1, NULL);
	for (auto tid : tids)
	{
		HANDLE hThread = OpenThread(THREAD_SET_CONTEXT, FALSE, tid);
		if (hThread)
		{
			QueueUserAPC((PAPCFUNC)GetProcAddress(GetModuleHandle(L"kernel32"), "LoadLibraryW"), hThread, (ULONG_PTR)p);
		}
	}
	VirtualFreeEx(hProcess, p, 0, MEM_RELEASE);
}


int main()
{
	char dst[] = "FlappyBird.exe", dllPath[] = "test.dll";
	TCHAR dst_t[] = L"FlappyBird.exe";
	WCHAR dllPath_t[] = L"C:\\Users\\22057\\Documents\\study\\class\\gamesecurity\\homework2\\inject\\x64\\Debug\\test.dll";
	// importTableInject(dst, dllPath);
	//remoteInjectDll(dst_t, dllPath_t);
	// setWindowHookEx_inject(dllPath_t);
	apc_inject(dst_t, dllPath_t);
	system("pause");
	return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
