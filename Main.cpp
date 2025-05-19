//
// 2024.9.8���� 
// IDE:						VisualStudio2022 
// C��׼:					MSVC2015 
// C++��׼:					ISO C++14 
// ��������:					Release x86 
// �ַ�����:					Unicode 
// WindowsSDK:				10 
// ����Ԥ����������(VS2022):	WIN32
//							NDEBUG
//							_WINDOWS
//							_CRT_SECURE_NO_WARNINGS
// 
//							����MSVC(����VS������? DEVCPP��û�Թ�)����������ֻ��ʹ��_s�İ�ȫ�溯��
//
#include <Windows.h> //Windows��׼��
#include <tlhelp32.h> //������ؿ�
#include <iostream>

BOOL CK = FALSE; //���ȡ������л�

//����Win�����ؼ���汾 ʹ���°�ؼ� ԭ��һ�� ���������manifest�ļ���
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

//���ݳ������ƽ������̼����ӽ����������� Լ�����������Ƶ�taskkill /f /t /im 
BOOL TerminateProcessAndChildren(const wchar_t* processName, UINT uExitCode)
{
	HANDLE hParentProcess = NULL;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); //��������
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, L"��������ʱCreateToolhelp32Snapshotʧ��!", L"   ���󱨸�", MB_ICONERROR);  //����ʧ��
		return FALSE;
	}

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hSnapshot, &pe32)) 
	{
		MessageBox(NULL, L"��������ʱProcess32Firstʧ��!", L"   ���󱨸�", MB_ICONERROR);  //����ʧ��
		CloseHandle(hSnapshot); //�رվ��
		return FALSE;
	}
	//ѡȡ����
	do
	{
		if (wcscmp(pe32.szExeFile, processName) == 0)
		{
			hParentProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);  
			break;
		}
	} while (Process32Next(hSnapshot, &pe32)); 

	CloseHandle(hSnapshot);

	if (hParentProcess == NULL)
	{
		return FALSE;
	}

	BOOL success = TRUE;  //ǰ�ڴ�����

	//����ɱ���ӽ���
	HANDLE hChildSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
	if (hChildSnapshot == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, L"��������ʱCreateToolhelp32Snapshotʧ��!", L"   ���󱨸�", MB_ICONERROR); 
		CloseHandle(hParentProcess);
		return FALSE;
	}

	PROCESSENTRY32 peChild32;
	peChild32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hChildSnapshot, &peChild32))
	{
		MessageBox(NULL, L"��������ʱProcess32Firstʧ��!", L"   ���󱨸�", MB_ICONERROR);
		CloseHandle(hChildSnapshot);
		CloseHandle(hParentProcess);
		return FALSE;
	}
	
	//���������̺��ӽ���
	do
	{
		if (peChild32.th32ParentProcessID == pe32.th32ProcessID)
		{
			HANDLE hChildProcess = OpenProcess(PROCESS_TERMINATE, FALSE, peChild32.th32ProcessID);
			if (hChildProcess != NULL)
			{
				TerminateProcess(hChildProcess, uExitCode);
				CloseHandle(hChildProcess);
			}
			success &= TerminateProcessAndChildren(pe32.szExeFile, uExitCode); 
		}
	} while (Process32Next(hChildSnapshot, &peChild32));

	CloseHandle(hChildSnapshot);
	TerminateProcess(hParentProcess, uExitCode);
	CloseHandle(hParentProcess);

	return success;  //���ؽ��
}

//���ݳ������ƹ������
BOOL SuspendProcessByName(const wchar_t* processName, int ReSuKi)
{
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, L"�������ʱCreateToolhelp32Snapshotʧ��!", L"   ���󱨸�", MB_ICONERROR);
		return false;
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hProcessSnap, &pe32))
	{
		MessageBox(NULL, L"�������ʱProcess32Firstʧ��!", L"   ���󱨸�", MB_ICONERROR);
		CloseHandle(hProcessSnap);
		return false;
	}

	bool success = false;
	do
	{
		if (std::wstring(pe32.szExeFile) == processName)
		{
			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | PROCESS_SUSPEND_RESUME, FALSE, pe32.th32ProcessID);
			if (hProcess != NULL) 
			{
				if (ReSuKi != 1145)
				{
					HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
					if (hThreadSnap != INVALID_HANDLE_VALUE)
					{
						THREADENTRY32 te32;
						te32.dwSize = sizeof(THREADENTRY32);
						if (Thread32First(hThreadSnap, &te32))
						{
							do
							{
								if (te32.th32OwnerProcessID == pe32.th32ProcessID)
								{
									HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
									if (hThread != NULL)
									{
										if (ReSuKi == 1) {
											SuspendThread(hThread);
										}
										else if (ReSuKi == 2)
										{
											ResumeThread(hThread);
										}
										CloseHandle(hThread);
									}
								}
							} while (Thread32Next(hThreadSnap, &te32));
						}
						CloseHandle(hThreadSnap);
					}
				}
				CloseHandle(hProcess);
				success = TRUE;
			}
			break;
		}
	} while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);


	/*
	if (success == TRUE)
	{
		if (s == TRUE) {
			MessageBox(NULL, L"�Ѿ�ͨ�������ȡ�������������̵ķ�ʽ����˲���!", L"   ��ʾ", MB_OK | MB_ICONINFORMATION);
		}
	}
	else
	{
		if (s == TRUE) {
			MessageBox(NULL, L"�������(������)ʧ��! �����Ǽ���������δ����!", L"   ���󱨸�", MB_OK | MB_ICONERROR);
		}
	}
	*/
	return success;
}


//------------------------------------------------------------------------------------------------
#define MAX_FILES 100
#define MAX_PATH_LENGTH 260

wchar_t exeFilesList[MAX_FILES][MAX_PATH_LENGTH + 10]; //���������ļ��ĳ���
int fileCount = 0;

void AddExeFileToList(const wchar_t* filePath) { //�ļ����Һ��� 
	if (fileCount < MAX_FILES) {
		const wchar_t* fileName = wcsrchr(filePath, L'\\');
		if (fileName != NULL) {
			fileName++; //�ƶ�����б��֮����ַ������ļ����Ŀ�ʼ��
		}
		else {
			fileName = (wchar_t*)filePath; //���û���ҵ���б�ܣ��ٶ�����·������һ���ļ���
		}

		//��������ļ����ַ���
		swprintf(exeFilesList[fileCount], MAX_PATH_LENGTH + 9, L"%s", fileName);
		fileCount++;
	}
}

void TerminateZhushouProcess() { //ɱ������
	for (int i = 0; i < fileCount; i++) {
		TerminateProcessAndChildren(exeFilesList[i], 0);
	}
}

void SearchExeInFolder(const wchar_t* folderPath) {  //Ѱ���ļ������ļ�
	WIN32_FIND_DATAW ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	wchar_t searchPath[MAX_PATH];

	swprintf(searchPath, MAX_PATH, L"%s\\*", folderPath);  //ƴ���ַ���
	hFind = FindFirstFileW(searchPath, &ffd);
	if (INVALID_HANDLE_VALUE == hFind) {
		return;
	}

	do {
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			// ����ļ���չ���Ƿ�Ϊ.exe
			wchar_t* ext = wcsrchr(ffd.cFileName, L'.');
			if (ext && _wcsicmp(ext, L".exe") == 0) {
				wchar_t fullPath[MAX_PATH];
				swprintf(fullPath, MAX_PATH, L"%s\\%s", folderPath, ffd.cFileName);
				AddExeFileToList(fullPath); //��.exe�ļ�·����ӵ��б���
			}
		}
	} while (FindNextFileW(hFind, &ffd) != 0);

	FindClose(hFind);
}

void FindAndUpdateFolder(const wchar_t findexe[]) {  //���һ�����������
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	wchar_t rootSearchPath[MAX_PATH] = L"C:\\*";

	hFind = FindFirstFile(rootSearchPath, &ffd);
	if (INVALID_HANDLE_VALUE == hFind) {
		return;
	}

	do {
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			//������ļ��У������Ƿ����"������.exe"
			wchar_t folderPath[MAX_PATH];
			swprintf(folderPath, MAX_PATH, findexe, ffd.cFileName);
			WIN32_FIND_DATA tempFfd;
			HANDLE tempFind = FindFirstFile(folderPath, &tempFfd);
			if (tempFind != INVALID_HANDLE_VALUE) {
				// �ҵ��˰���"������.exe"���ļ��У����ڱ������ļ����µ�����.exe�ļ�
				FindClose(tempFind);
				swprintf(folderPath, MAX_PATH, L"C:\\%s", ffd.cFileName);
				SearchExeInFolder(folderPath);
				break;
			}
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	FindClose(hFind);
}

void ProgramFilesJFGLZS(const wchar_t ProgramFilesPath[]) {  //����v9.9���µı������

	//v9.9���µı������ ����Program Files�ļ����´������������Ӣ���ļ��� ������һ��������Ƶı������� ������Ҫ���д���

	WIN32_FIND_DATAW findFileData;
	HANDLE hFind;
	wchar_t folderPath[MAX_PATH];
	wchar_t searchPath[MAX_PATH];
	wcscpy(folderPath, ProgramFilesPath); //Program Files �ļ���·��

	// �洢�ļ����Ķ�ά����
	wchar_t fileNames[100][MAX_PATH]; //���洢100���ļ�����ÿ���ļ������ MAX_PATH ����
	int count = 0;

	// ��������·��
	wcscpy(searchPath, folderPath);
	wcscat(searchPath, L"\\???"); //ƥ�����������ַ����ļ���

	hFind = FindFirstFileW(searchPath, &findFileData);

	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
				&& wcslen(findFileData.cFileName) == 3) {
				//������ļ������ļ���������Ϊ3��������ļ��м�������
				wcscpy(searchPath, folderPath);
				wcscat(searchPath, L"\\");
				wcscat(searchPath, findFileData.cFileName);
				wcscat(searchPath, L"\\*.exe");

				HANDLE hFile = FindFirstFileW(searchPath, &findFileData);
				if (hFile != INVALID_HANDLE_VALUE) {
					do {
						if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
							wcscpy(fileNames[count], findFileData.cFileName);
							count++;
							// ȷ�����������鷶Χ
							if (count >= 100) {
								break;
							}
						}
					} while (FindNextFileW(hFile, &findFileData) != 0 && count < 100);
					FindClose(hFile);
				}
			}
		} while (FindNextFileW(hFind, &findFileData) != 0);
		FindClose(hFind);

		//ɱ�����б�������
		for (int i = 0; i < count; i++) {
			TerminateProcessAndChildren(fileNames[i], 0);
		}
	}

}

//
#define MsgBtnSuspendJY 1
#define MsgBtnResumeJY 2
#define MsgBtnFixInternet 3
#define MsgBtnStopSJFGLZS 4
#define MsgBtnStartRes 6
#define MsgBtnStartCmd 7
#define MsgBtnDownloadPVZ 8
#define MsgBtnDownloadCS16 9
#define MsgBtnStopJY 11

HANDLE hToken, hFinalToken; //Ŀǰ��ʹ�� Ϊ����ȡwinlogon.exe�������ó�����SystemȨ������ʵ����Windows8�����ϵĴ��ڳ����ö�
//�������ھ��
HWND hWnd, hButtonSuspendJY, hButtonFixInternet, hButtonStopXSJFGLZS, hButtonStartRes, hButtonStartCMD, hButtonStartCS16, hButtonDownloadPVZ, hButtonStopJY, hGroupBoxKZ, hGroupBoxXZ, hGroupBoxKJ;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow) {  //Win32���������

	TCHAR szAppname[] = L"2024.7����޸� 25.5.20���� By:Mwz4662";
	MSG msg;
	WNDCLASS wndclass = {};  //����������
	wndclass.style = CS_HREDRAW | CS_VREDRAW; // �޸ĸ�ֵ�����
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.lpszMenuName = NULL;
	wndclass.hbrBackground = (HBRUSH)(GetStockObject(WHITE_BRUSH));
	wndclass.lpszClassName = szAppname;

	if (!RegisterClass(&wndclass)) {  //ע�ᴰ���ಢ�������
		MessageBox(NULL, L"Error", szAppname, MB_ICONERROR | MB_OK | MB_ICONINFORMATION);
	}

	//����������
	hWnd = CreateWindow(szAppname, szAppname,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT,
		465, 215,
		NULL, NULL, hInstance, NULL);

	//�����ӿؼ�
	hGroupBoxKZ = CreateWindow(
		L"BUTTON", L"���ƽ��", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 10, 10, 250, 95, hWnd, NULL, hInstance, NULL
	);
	hGroupBoxXZ = CreateWindow(
		L"BUTTON", L"���ƽ��", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 270, 10, 175, 95, hWnd, NULL, hInstance, NULL
	);
	hGroupBoxKJ = CreateWindow(
		L"BUTTON", L"��ݷ�ʽ", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 10, 110, 435, 60, hWnd, NULL, hInstance, NULL
	);

	hButtonStopJY = CreateWindow(
		L"BUTTON", L"ǿɱ����(��ɱ����)", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 30, 145, 30, hWnd, (HMENU)MsgBtnStopJY, hInstance, NULL
	);
	hButtonSuspendJY = CreateWindow(
		L"BUTTON", L"�������", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 30, 80, 30, hWnd, (HMENU)MsgBtnSuspendJY, hInstance, NULL
	);
	hButtonFixInternet = CreateWindow(
		L"BUTTON", L"���������������", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 280, 30, 155, 30, hWnd, (HMENU)MsgBtnFixInternet, hInstance, NULL
	);
	hButtonStopXSJFGLZS = CreateWindow(
		L"BUTTON", L"ǿ��ɱ��ѧ��������������", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 65, 230, 30, hWnd, (HMENU)MsgBtnStopSJFGLZS, hInstance, NULL
	);
	hButtonStartRes = CreateWindow(
		L"BUTTON", L"��Դ������", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 130, 105, 30, hWnd, (HMENU)MsgBtnStartRes, hInstance, NULL
	);
	hButtonStartCMD = CreateWindow(
		L"BUTTON", L"������ʾ��", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 130, 130, 105, 30, hWnd, (HMENU)MsgBtnStartCmd, hInstance, NULL
	);
	hButtonDownloadPVZ = CreateWindow(
		L"BUTTON", L"����PVZ", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 240, 130, 95, 30, hWnd, (HMENU)MsgBtnDownloadPVZ, hInstance, NULL
	);
	hButtonStartCS16 = CreateWindow(
		L"BUTTON", L"����Cs1.6", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 340, 130, 95, 30, hWnd, (HMENU)MsgBtnDownloadCS16, hInstance, NULL
	);

	//�����ӿؼ�����
	HFONT hFont = CreateFont(19, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GB2312_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN, L"΢���ź�");
	SendMessage(hButtonSuspendJY, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessage(hButtonFixInternet, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessage(hButtonStopXSJFGLZS, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessage(hButtonStartRes, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessage(hButtonStartCMD, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessage(hButtonDownloadPVZ, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessage(hButtonStartCS16, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessage(hButtonStopJY, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessage(hGroupBoxKJ, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessage(hGroupBoxKZ, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessage(hGroupBoxXZ, WM_SETFONT, (WPARAM)hFont, TRUE);

	ShowWindow(hWnd, iCmdShow);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, NULL, 0, 0)) {  //����ѭ��
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {  //���ڴ�����

	switch (message) {
	case WM_CREATE:
		break;
	case WM_COMMAND:  //�����ӿؼ�����
	{
		switch (LOWORD(wParam)) {
		case MsgBtnSuspendJY:
			//���
			if (CK == TRUE) {
				if (MessageBox(NULL, L"��ѡ�����Լ���Ĺ���\n���ǽ�ʦ�˽�����Ʋ�Ȼ������Ҳû�취!\n�Ƿ����?", L"   ����", MB_YESNO | MB_ICONWARNING) == IDYES) {
					if (SuspendProcessByName(L"StudentMain.exe", 2) == TRUE) {
						CK = FALSE;
						SetWindowText(hButtonSuspendJY, L"�������");
						MessageBox(NULL, L"�Ѿ�ͨ�������ȡ�������������̵ķ�ʽ����˲���!", L"   ��ʾ", MB_OK | MB_ICONINFORMATION);
					}
				}
				else {
					MessageBox(NULL, L"��ѡ����ȡ�� ������û�б�ִ��!", L"   ��ʾ", MB_OK | MB_ICONINFORMATION);
				}
			}
			else {
				if (SuspendProcessByName(L"StudentMain.exe", 1) == TRUE) {
					SetWindowText(hButtonSuspendJY, L"�ָ�����");
					CK = TRUE;
					MessageBox(NULL, L"�������(������)ʧ��! �����Ǽ���������δ����!", L"   ���󱨸�", MB_OK | MB_ICONERROR);
				}
			}
			break;
		case MsgBtnFixInternet:
			//��������������� �ر�MasterHelper��gatesrv�ٽ���TDNetFilter����
			TerminateProcessAndChildren(L"MasterHelper.exe", 0);
			TerminateProcessAndChildren(L"GATESRV.exe", 0);
			system("sc stop TDNetFilter");
			MessageBox(NULL, L"�Ѿ�ͨ���������������̼���ط��������ķ�ʽ����˲���!", L"   ��ʾ", MB_OK | MB_ICONINFORMATION);
			break;
		case MsgBtnStopSJFGLZS:
			//ɱ��ѧ��������������
			FindAndUpdateFolder(L"C:\\%s\\������.exe");
			TerminateZhushouProcess();
			FindAndUpdateFolder(L"C:\\%s\\pfn.exe");
			TerminateZhushouProcess();
			ProgramFilesJFGLZS(L"C:\\Program Files (x86)");
			ProgramFilesJFGLZS(L"C:\\Program Files");
			MessageBox(NULL, L"�Ѿ�ͨ���ݹ���Һͳ������ý����˳���������˲���!", L"   ��ʾ", MB_OK | MB_ICONINFORMATION);
			break;
		case MsgBtnStartCmd:
			//����CMD
			system("start %WinDir%\\System32\\cmd.exe /k cd %WinDir%");
			break;
		case MsgBtnStartRes:
			//��������������� ʹ����Դ����������
			if (MessageBox(NULL, L"��Դ��������ѧ�������������ֱ��Ϊ\"����в���ӽ������еĳ���\"\n��һ��ȷ�����Ѿ����й���\"�ر�ѧ��������������\"\n������Խ��ᱻѧ������������������!\n�Ƿ����?", L"   ����", MB_YESNO | MB_ICONWARNING) == IDYES) {
				system("start %WinDir%\\System32\\perfmon.exe /res");
			}
			else {
				MessageBox(NULL, L"��ѡ����ȡ�� ������û�б�ִ��!", L"   ��ʾ", MB_OK | MB_ICONINFORMATION);
			}
			break;
		case MsgBtnDownloadCS16:
			//�������CS1.6
			system("start https://cs16.info/cn/");
			MessageBox(NULL, L"����CS����ʾ:\n����ҳ��ֱ��ѡ������ ����������ѡ�񱣴�\n�м�! һ����Ҫ��������Һ�̨(���Թ���ҳ)\n���������ٶȻῳ�����0KB/s\n\nCs1.6��������������\n������������ʾ�� ����ipconfig �س�\n�ڵ������������ҵ�\"IPV4��ַ\"��ס�����һ���ַ�\n������Cs1.6 ѡ�����˵���New Game ��Mapѡ���ͼ\n\n��������\n����de_dust2����ɳ�� de_inferno����С��\n����Ҫ�˻��Ͱ�Include...�Ĺ�ȡ����\n�رն����˺� ���Gameѡ���Friendly Fire�Ĺ�ȡ����\n��ֹ���� ���Gameѡ� �ҵ�Death Camera... ѡ��Only First...\n\n��Ҷ�:�����̵�~ �ڵ����Ĵ���������\"connect ��õ��ķ���IPV4��ַ\"\n���س��ͽ�ȥ��", L"   ��ʾ", MB_OK | MB_ICONINFORMATION);
			break;
		case MsgBtnDownloadPVZ:
			//�������PVZ
			system("start https://pvz.lanzoux.com/iau42bc");
			MessageBox(NULL, L"ֱ�����ش� ѡ�����(W) ֱ��ѡ������ ȷ�� ��װ\n�ҵ������ϵ�PlatsVs...�ļ��� �� �ٴ��ļ������PlantVs...", L"   ��ʾ", MB_OK | MB_ICONINFORMATION);
			break;
		case MsgBtnStopJY:
			//ɱ������
			//2024.10.3���� ��Ҫ�ظ�ɱ�� ��ΪԶ�̽��Ŀǰ�Ѿ����� ���ﲻ������
			if (MessageBox(NULL, L"��ѡ�ǿ��ɱ������!\n��һ��ȷ�����Ѿ����й���\"�ر�ѧ��������������\"\n������Խ��ᱻѧ������������������!\n�Ƿ����?", L"   ����", MB_YESNO | MB_ICONWARNING) == IDYES) {
				if (TerminateProcessAndChildren(L"StudentMain.exe", 0) == TRUE) {
					MessageBox(NULL, L"�Ѿ�ͨ���������ý����˳���������˲���!", L"   ��ʾ", MB_OK | MB_ICONINFORMATION);
				}
				else {
					MessageBox(NULL, L"ǿ��ɱ������ʧ��! �����Ǽ���������δ����!", L"   ���󱨸�", MB_OK | MB_ICONERROR);
				}
			}
			else {
				MessageBox(NULL, L"��ѡ����ȡ�� ������û�б�ִ��!", L"   ��ʾ", MB_OK | MB_ICONINFORMATION);
			}
		}
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}
