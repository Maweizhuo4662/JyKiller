//
// 2024.9.8更新 
// IDE:						VisualStudio2022 
// C标准:					MSVC2015 
// C++标准:					ISO C++14 
// 导出设置:					Release x86 
// 字符编码:					Unicode 
// WindowsSDK:				10 
// 编译预处理器定义(VS2022):	WIN32
//							NDEBUG
//							_WINDOWS
//							_CRT_SECURE_NO_WARNINGS
// 
//							↑用MSVC(或者VS的问题? DEVCPP真没试过)不添加这个就只能使用_s的安全版函数
//
#include <Windows.h> //Windows标准库
#include <tlhelp32.h> //挂起相关库
#include <iostream>

BOOL CK = FALSE; //解控取消解控切换

//更改Win公共控件库版本 使用新版控件 原理一样 不像另外搞manifest文件了
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

//根据程序名称结束进程及其子进程无视限制 约等于无视限制的taskkill /f /t /im 
BOOL TerminateProcessAndChildren(const wchar_t* processName, UINT uExitCode)
{
	HANDLE hParentProcess = NULL;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); //创建快照
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, L"结束进程时CreateToolhelp32Snapshot失败!", L"   错误报告", MB_ICONERROR);  //处理失败
		return FALSE;
	}

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hSnapshot, &pe32)) 
	{
		MessageBox(NULL, L"结束进程时Process32First失败!", L"   错误报告", MB_ICONERROR);  //处理失败
		CloseHandle(hSnapshot); //关闭句柄
		return FALSE;
	}
	//选取进程
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

	BOOL success = TRUE;  //前期处理结果

	//查找杀死子进程
	HANDLE hChildSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
	if (hChildSnapshot == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, L"结束进程时CreateToolhelp32Snapshot失败!", L"   错误报告", MB_ICONERROR); 
		CloseHandle(hParentProcess);
		return FALSE;
	}

	PROCESSENTRY32 peChild32;
	peChild32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hChildSnapshot, &peChild32))
	{
		MessageBox(NULL, L"结束进程时Process32First失败!", L"   错误报告", MB_ICONERROR);
		CloseHandle(hChildSnapshot);
		CloseHandle(hParentProcess);
		return FALSE;
	}
	
	//结束父进程和子进程
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

	return success;  //返回结果
}

//根据程序名称挂起程序
BOOL SuspendProcessByName(const wchar_t* processName, int ReSuKi)
{
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, L"挂起进程时CreateToolhelp32Snapshot失败!", L"   错误报告", MB_ICONERROR);
		return false;
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hProcessSnap, &pe32))
	{
		MessageBox(NULL, L"挂起进程时Process32First失败!", L"   错误报告", MB_ICONERROR);
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
			MessageBox(NULL, L"已经通过挂起或取消挂起极域主进程的方式完成了操作!", L"   提示", MB_OK | MB_ICONINFORMATION);
		}
	}
	else
	{
		if (s == TRUE) {
			MessageBox(NULL, L"解除控屏(挂起极域)失败! 可能是极域主进程未启动!", L"   错误报告", MB_OK | MB_ICONERROR);
		}
	}
	*/
	return success;
}


//------------------------------------------------------------------------------------------------
#define MAX_FILES 100
#define MAX_PATH_LENGTH 260

wchar_t exeFilesList[MAX_FILES][MAX_PATH_LENGTH + 10]; //留出查找文件的长度
int fileCount = 0;

void AddExeFileToList(const wchar_t* filePath) { //文件查找函数 
	if (fileCount < MAX_FILES) {
		const wchar_t* fileName = wcsrchr(filePath, L'\\');
		if (fileName != NULL) {
			fileName++; //移动到反斜杠之后的字符（即文件名的开始）
		}
		else {
			fileName = (wchar_t*)filePath; //如果没有找到反斜杠，假定整个路径就是一个文件名
		}

		//构造查找文件名字符串
		swprintf(exeFilesList[fileCount], MAX_PATH_LENGTH + 9, L"%s", fileName);
		fileCount++;
	}
}

void TerminateZhushouProcess() { //杀死进程
	for (int i = 0; i < fileCount; i++) {
		TerminateProcessAndChildren(exeFilesList[i], 0);
	}
}

void SearchExeInFolder(const wchar_t* folderPath) {  //寻找文件夹内文件
	WIN32_FIND_DATAW ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	wchar_t searchPath[MAX_PATH];

	swprintf(searchPath, MAX_PATH, L"%s\\*", folderPath);  //拼接字符串
	hFind = FindFirstFileW(searchPath, &ffd);
	if (INVALID_HANDLE_VALUE == hFind) {
		return;
	}

	do {
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			// 检查文件扩展名是否为.exe
			wchar_t* ext = wcsrchr(ffd.cFileName, L'.');
			if (ext && _wcsicmp(ext, L".exe") == 0) {
				wchar_t fullPath[MAX_PATH];
				swprintf(fullPath, MAX_PATH, L"%s\\%s", folderPath, ffd.cFileName);
				AddExeFileToList(fullPath); //将.exe文件路径添加到列表中
			}
		}
	} while (FindNextFileW(hFind, &ffd) != 0);

	FindClose(hFind);
}

void FindAndUpdateFolder(const wchar_t findexe[]) {  //查找机房管理助手
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	wchar_t rootSearchPath[MAX_PATH] = L"C:\\*";

	hFind = FindFirstFile(rootSearchPath, &ffd);
	if (INVALID_HANDLE_VALUE == hFind) {
		return;
	}

	do {
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			//如果是文件夹，则检查是否包含"更新器.exe"
			wchar_t folderPath[MAX_PATH];
			swprintf(folderPath, MAX_PATH, findexe, ffd.cFileName);
			WIN32_FIND_DATA tempFfd;
			HANDLE tempFind = FindFirstFile(folderPath, &tempFfd);
			if (tempFind != INVALID_HANDLE_VALUE) {
				// 找到了包含"更新器.exe"的文件夹，现在遍历该文件夹下的所有.exe文件
				FindClose(tempFind);
				swprintf(folderPath, MAX_PATH, L"C:\\%s", ffd.cFileName);
				SearchExeInFolder(folderPath);
				break;
			}
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	FindClose(hFind);
}

void ProgramFilesJFGLZS(const wchar_t ProgramFilesPath[]) {  //处理v9.9更新的保活机制

	//v9.9更新的保活机制 会在Program Files文件夹下创建随机的三字英文文件夹 里面有一个随机名称的保护程序 这里需要进行处理

	WIN32_FIND_DATAW findFileData;
	HANDLE hFind;
	wchar_t folderPath[MAX_PATH];
	wchar_t searchPath[MAX_PATH];
	wcscpy(folderPath, ProgramFilesPath); //Program Files 文件夹路径

	// 存储文件名的二维数组
	wchar_t fileNames[100][MAX_PATH]; //最多存储100个文件名，每个文件名最多 MAX_PATH 长度
	int count = 0;

	// 构造搜索路径
	wcscpy(searchPath, folderPath);
	wcscat(searchPath, L"\\???"); //匹配所有三个字符的文件夹

	hFind = FindFirstFileW(searchPath, &findFileData);

	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
				&& wcslen(findFileData.cFileName) == 3) {
				//如果是文件夹且文件夹名长度为3，则进入文件夹继续搜索
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
							// 确保不超出数组范围
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

		//杀死所有保护程序
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

HANDLE hToken, hFinalToken; //目前不使用 为了窃取winlogon.exe的令牌让程序以System权限启动实现在Windows8及以上的窗口超级置顶
//创建窗口句柄
HWND hWnd, hButtonSuspendJY, hButtonFixInternet, hButtonStopXSJFGLZS, hButtonStartRes, hButtonStartCMD, hButtonStartCS16, hButtonDownloadPVZ, hButtonStopJY, hGroupBoxKZ, hGroupBoxXZ, hGroupBoxKJ;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow) {  //Win32主程序入口

	TCHAR szAppname[] = L"2024.7最后修改 25.5.20编译 By:Mwz4662";
	MSG msg;
	WNDCLASS wndclass = {};  //创建窗口类
	wndclass.style = CS_HREDRAW | CS_VREDRAW; // 修改赋值运算符
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.lpszMenuName = NULL;
	wndclass.hbrBackground = (HBRUSH)(GetStockObject(WHITE_BRUSH));
	wndclass.lpszClassName = szAppname;

	if (!RegisterClass(&wndclass)) {  //注册窗口类并处理错误
		MessageBox(NULL, L"Error", szAppname, MB_ICONERROR | MB_OK | MB_ICONINFORMATION);
	}

	//创建主窗口
	hWnd = CreateWindow(szAppname, szAppname,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT,
		465, 215,
		NULL, NULL, hInstance, NULL);

	//创建子控件
	hGroupBoxKZ = CreateWindow(
		L"BUTTON", L"控制解除", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 10, 10, 250, 95, hWnd, NULL, hInstance, NULL
	);
	hGroupBoxXZ = CreateWindow(
		L"BUTTON", L"限制解除", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 270, 10, 175, 95, hWnd, NULL, hInstance, NULL
	);
	hGroupBoxKJ = CreateWindow(
		L"BUTTON", L"快捷方式", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 10, 110, 435, 60, hWnd, NULL, hInstance, NULL
	);

	hButtonStopJY = CreateWindow(
		L"BUTTON", L"强杀极域(先杀助手)", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 30, 145, 30, hWnd, (HMENU)MsgBtnStopJY, hInstance, NULL
	);
	hButtonSuspendJY = CreateWindow(
		L"BUTTON", L"解除控屏", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 30, 80, 30, hWnd, (HMENU)MsgBtnSuspendJY, hInstance, NULL
	);
	hButtonFixInternet = CreateWindow(
		L"BUTTON", L"解除极域网络限制", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 280, 30, 155, 30, hWnd, (HMENU)MsgBtnFixInternet, hInstance, NULL
	);
	hButtonStopXSJFGLZS = CreateWindow(
		L"BUTTON", L"强制杀掉学生机房管理助手", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 65, 230, 30, hWnd, (HMENU)MsgBtnStopSJFGLZS, hInstance, NULL
	);
	hButtonStartRes = CreateWindow(
		L"BUTTON", L"资源监视器", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 130, 105, 30, hWnd, (HMENU)MsgBtnStartRes, hInstance, NULL
	);
	hButtonStartCMD = CreateWindow(
		L"BUTTON", L"命令提示符", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 130, 130, 105, 30, hWnd, (HMENU)MsgBtnStartCmd, hInstance, NULL
	);
	hButtonDownloadPVZ = CreateWindow(
		L"BUTTON", L"下载PVZ", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 240, 130, 95, 30, hWnd, (HMENU)MsgBtnDownloadPVZ, hInstance, NULL
	);
	hButtonStartCS16 = CreateWindow(
		L"BUTTON", L"下载Cs1.6", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 340, 130, 95, 30, hWnd, (HMENU)MsgBtnDownloadCS16, hInstance, NULL
	);

	//更改子控件字体
	HFONT hFont = CreateFont(19, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GB2312_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN, L"微软雅黑");
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
	while (GetMessage(&msg, NULL, 0, 0)) {  //启动循环
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {  //窗口处理函数

	switch (message) {
	case WM_CREATE:
		break;
	case WM_COMMAND:  //进行子控件处理
	{
		switch (LOWORD(wParam)) {
		case MsgBtnSuspendJY:
			//解控
			if (CK == TRUE) {
				if (MessageBox(NULL, L"此选项将解除对极域的挂起\n除非教师端解除控制不然本程序也没办法!\n是否继续?", L"   警告", MB_YESNO | MB_ICONWARNING) == IDYES) {
					if (SuspendProcessByName(L"StudentMain.exe", 2) == TRUE) {
						CK = FALSE;
						SetWindowText(hButtonSuspendJY, L"解除控屏");
						MessageBox(NULL, L"已经通过挂起或取消挂起极域主进程的方式完成了操作!", L"   提示", MB_OK | MB_ICONINFORMATION);
					}
				}
				else {
					MessageBox(NULL, L"你选择了取消 操作并没有被执行!", L"   提示", MB_OK | MB_ICONINFORMATION);
				}
			}
			else {
				if (SuspendProcessByName(L"StudentMain.exe", 1) == TRUE) {
					SetWindowText(hButtonSuspendJY, L"恢复控屏");
					CK = TRUE;
					MessageBox(NULL, L"解除控屏(挂起极域)失败! 可能是极域主进程未启动!", L"   错误报告", MB_OK | MB_ICONERROR);
				}
			}
			break;
		case MsgBtnFixInternet:
			//解除极域网络限制 关闭MasterHelper和gatesrv再结束TDNetFilter服务
			TerminateProcessAndChildren(L"MasterHelper.exe", 0);
			TerminateProcessAndChildren(L"GATESRV.exe", 0);
			system("sc stop TDNetFilter");
			MessageBox(NULL, L"已经通过结束极域辅助进程及相关服务驱动的方式完成了操作!", L"   提示", MB_OK | MB_ICONINFORMATION);
			break;
		case MsgBtnStopSJFGLZS:
			//杀死学生机房管理助手
			FindAndUpdateFolder(L"C:\\%s\\更新器.exe");
			TerminateZhushouProcess();
			FindAndUpdateFolder(L"C:\\%s\\pfn.exe");
			TerminateZhushouProcess();
			ProgramFilesJFGLZS(L"C:\\Program Files (x86)");
			ProgramFilesJFGLZS(L"C:\\Program Files");
			MessageBox(NULL, L"已经通过递归查找和程序内置进程退出函数完成了操作!", L"   提示", MB_OK | MB_ICONINFORMATION);
			break;
		case MsgBtnStartCmd:
			//启动CMD
			system("start %WinDir%\\System32\\cmd.exe /k cd %WinDir%");
			break;
		case MsgBtnStartRes:
			//任务管理器被禁用 使用资源监视器代替
			if (MessageBox(NULL, L"资源监视器被学生机房管理助手标记为\"会威胁电子教室运行的程序\"\n请一定确保你已经运行过了\"关闭学生机房管理助手\"\n否则电脑将会被学生机房管理助手锁定!\n是否继续?", L"   警告", MB_YESNO | MB_ICONWARNING) == IDYES) {
				system("start %WinDir%\\System32\\perfmon.exe /res");
			}
			else {
				MessageBox(NULL, L"你选择了取消 操作并没有被执行!", L"   提示", MB_OK | MB_ICONINFORMATION);
			}
			break;
		case MsgBtnDownloadCS16:
			//快捷下载CS1.6
			system("start https://cs16.info/cn/");
			MessageBox(NULL, L"关于CS的提示:\n打开网页后直接选择下载 弹出窗口里选择保存\n切记! 一定不要把浏览器挂后台(可以关网页)\n否则下载速度会砍半或者0KB/s\n\nCs1.6局域网联机方法\n房主打开命令提示符 输入ipconfig 回车\n在弹出的文字中找到\"IPV4地址\"记住后面的一串字符\n房主打开Cs1.6 选择主菜单的New Game 点Map选择地图\n\n房主设置\n比如de_dust2就是沙二 de_inferno就是小镇\n不需要人机就把Include...的勾取消掉\n关闭队友伤害 点击Game选项卡把Friendly Fire的勾取消掉\n防止窥屏 点击Game选项卡 找到Death Camera... 选择Only First...\n\n玩家端:按键盘的~ 在弹出的窗口里输入\"connect 你得到的房主IPV4地址\"\n最后回车就进去了", L"   提示", MB_OK | MB_ICONINFORMATION);
			break;
		case MsgBtnDownloadPVZ:
			//快捷下载PVZ
			system("start https://pvz.lanzoux.com/iau42bc");
			MessageBox(NULL, L"直接下载打开 选择浏览(W) 直接选择桌面 确定 安装\n找到桌面上的PlatsVs...文件夹 打开 再打开文件夹里的PlantVs...", L"   提示", MB_OK | MB_ICONINFORMATION);
			break;
		case MsgBtnStopJY:
			//杀死极域
			//2024.10.3更新 需要重复杀死 因为远程解控目前已经更新 这里不做更新
			if (MessageBox(NULL, L"此选项将强制杀掉极域!\n请一定确保你已经运行过了\"关闭学生机房管理助手\"\n否则电脑将会被学生机房管理助手锁定!\n是否继续?", L"   警告", MB_YESNO | MB_ICONWARNING) == IDYES) {
				if (TerminateProcessAndChildren(L"StudentMain.exe", 0) == TRUE) {
					MessageBox(NULL, L"已经通过程序内置进程退出函数完成了操作!", L"   提示", MB_OK | MB_ICONINFORMATION);
				}
				else {
					MessageBox(NULL, L"强制杀掉极域失败! 可能是极域主进程未启动!", L"   错误报告", MB_OK | MB_ICONERROR);
				}
			}
			else {
				MessageBox(NULL, L"你选择了取消 操作并没有被执行!", L"   提示", MB_OK | MB_ICONINFORMATION);
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
