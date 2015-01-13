#include <Windows.h>
#include <tchar.h>
#include <iostream>
#include <time.h>
#include <iomanip>
using namespace std;

#ifdef _UNICODE
#define tcout wcout
#define tstrcmp wcscmp
#define tstrstr wcsstr
#else
#define tcout cout
#define tstrcmp strcmp
#define tstrstr strstr
#endif

bool IsProgressMode(HWND);
bool IsTreasureChest(HWND);
void UseSkill(HWND, int);
void EnableProgressMode(HWND, bool);
void Upgrade(HWND);
void Attack(HWND);

void Timestamp()
{
	clock_t ts = clock();
	int sec = ts / CLOCKS_PER_SEC;
	tcout << setfill(_T('0')) << setw(2) << sec/3600 << _T(":") << setfill(_T('0')) << setw(2) << (sec/60)%60 << _T(":") << setfill(_T('0')) << setw(2) << sec%60 << _T(" ");
}

#define DELAY 50
#define FASTDELAY 10
#define ERROR_FOUND ((1<<29)+1)
BOOL CALLBACK FindMozillaInDesktop(HWND hwnd, LPARAM ret)
{
	const int MAX_CLASSNAME = 256;
	TCHAR classname[MAX_CLASSNAME];
	auto len = RealGetWindowClass(hwnd, classname, MAX_CLASSNAME);
	if (tstrcmp(classname, _T("MozillaWindowClass")) == 0)
	{
		const int MAX_CAPTION = 256;
		TCHAR caption[MAX_CAPTION];
		len = GetWindowText(hwnd, caption, MAX_CAPTION);
		if (tstrstr(caption, _T("Clicker Heroes")))
		{
			hwnd = FindWindowEx(hwnd, 0, _T("MozillaWindowClass"), 0);
			if (hwnd)
			{
				hwnd = FindWindowEx(hwnd, 0, _T("GeckoPluginWindow"), 0);
				if (hwnd)
				{
					hwnd = FindWindowEx(hwnd, 0, _T("GeckoFPSandboxChildWindow"), 0);
					if (hwnd)
					{	
						HWND* p = (HWND*)ret;
						*p = hwnd;
						SetLastError(ERROR_FOUND);
						return FALSE;	// found; success
					}
				}
			}
		}
	}
	return TRUE;
}
HWND FindTargetHWND()
{
	HWND hwnd = 0;
	BOOL bRet = EnumWindows(FindMozillaInDesktop, (LPARAM)&hwnd);
	if (!bRet && GetLastError()!=ERROR_FOUND)
	{
		tcout << _T("EnumWindows(FindMozilla) Fail; GetLastError() = ") << GetLastError() << endl;
		system("pause");
		return 0;
	}
	return hwnd;
}

void ClickWithDelay(HWND hwnd, int x, int y, int delay)
{
	auto bRet = PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x,y));
	if (!bRet)
	{
		tcout << _T("PostMessage() fail; GetLastError() = ") << GetLastError() << endl;
		return;
	}
	Sleep(delay);
	bRet = PostMessage(hwnd, WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(x,y));
	if (!bRet)
	{
		tcout << _T("PostMessage() fail; GetLastError() = ") << GetLastError() << endl;
		return;
	}
	Sleep(delay);
}
void Click(HWND hwnd, int x, int y)
{
	ClickWithDelay(hwnd, x, y, DELAY);
}
void FastClick(HWND hwnd, int x, int y)
{
	ClickWithDelay(hwnd, x, y, FASTDELAY);
}
void KeyPress(HWND hwnd, int vk)
{
	auto bRet = PostMessage(hwnd, WM_KEYDOWN, vk, 0);
	if (!bRet)
	{
		tcout << _T("PostMessage() fail; GetLastError() = ") << GetLastError() << endl;
		return;
	}
	Sleep(DELAY);
	bRet = PostMessage(hwnd, WM_KEYUP, vk, 0);
	if (!bRet)
	{
		tcout << _T("PostMessage() fail; GetLastError() = ") << GetLastError() << endl;
		return;
	}
	Sleep(DELAY);
}
bool IsProgressMode(HWND hwnd)
{
	HDC dc = GetDC(hwnd);
	RECT rect;
	GetWindowRect(hwnd, &rect);
	//ref: window size: 1136x642, progress icon pos: (1114,210)
	int x = 1114 * ( rect.right - rect.left ) / 1136;
	int y = 210 * ( rect.bottom - rect.top ) / 642;
	bool found = false;
	for (int h = -3; h <= 3; h++)
	{
		COLORREF clr = GetPixel(dc, x, y + h);
		if (GetRValue(clr) == 255 && GetGValue(clr) == 0 && GetBValue(clr) == 0)
		{
			found = true;
			break;
		}
	}
	ReleaseDC(hwnd, dc);
	return !found;	// if found red => limit sign => not progress mode
}
bool IsTreasureChest(HWND hwnd)
{
	HDC dc = GetDC(hwnd);
	RECT rect;
	GetWindowRect(hwnd, &rect);
	//ref: window size: 1136x642, chest pos: (830,336)
	int x = 830 * ( rect.right - rect.left ) / 1136;
	int y = 336 * ( rect.bottom - rect.top ) / 642;
	bool found = false;
	COLORREF clr = GetPixel(dc, x, y);
	found = (GetRValue(clr) == 196 && GetGValue(clr) == 133 && GetBValue(clr) == 44);
	ReleaseDC(hwnd, dc);
	return found;	// if found chest color
}
void UseSkill(HWND hwnd, int i)
{
	KeyPress(hwnd, (int)'0' + i);
}
void EnableProgressMode(HWND hwnd, bool enable)
{
	bool isProgressMode = IsProgressMode(hwnd);
	if ((enable && !isProgressMode) || (!enable && isProgressMode))
		KeyPress(hwnd, (int)'A');
}
void Upgrade(HWND hwnd)
{
	//ref: window size: 1136x642, hero upgrade pos: (91,238)
	RECT rect;
	GetWindowRect(hwnd, &rect);
	int x = 91 * ( rect.right - rect.left ) / 1136;
	int y = 238 * ( rect.bottom - rect.top ) / 642;
	Click(hwnd, x, y);
}
void Attack(HWND hwnd)
{
	//ref: window size: 1136x642, monster pos: (861,133)
	//note: should avoid to aim the center in case the spark of attacking would hide the chest color
	RECT rect;
	GetWindowRect(hwnd, &rect);
	int x = 861 * ( rect.right - rect.left ) / 1136;
	int y = 133 * ( rect.bottom - rect.top ) / 642;
	Click(hwnd, x, y);
}
void FastAttack(HWND hwnd)
{
	//ref: window size: 1136x642, monster pos: (861,133)
	//note: should avoid to aim the center in case the spark of attacking would hide the chest color
	RECT rect;
	GetWindowRect(hwnd, &rect);
	int x = 861 * ( rect.right - rect.left ) / 1136;
	int y = 133 * ( rect.bottom - rect.top ) / 642;
	FastClick(hwnd, x, y);
}
int main()
{
	// Note 1: When SendMessage WM_LBUTTONDOWN to chrome, it would popon to the foreground.
	// Therefore, I choose the firefox as my internet explorer.
	// Note 2: Although it supports background auto playing,
	// the firefox window should not be minimized, otherwise the FindWindow would fail.
	// Note 3: This program is just to test the feasibility of auto play in the background,
	// so I do not intend to support complex function such as auto ascend world in the future.
	// Note 4: This program assumes that only one hero could be upgraded,
	// and it's scrolled to the top in the viewable screen.
	// Note 5: This program assumes Vaagur has been level up to maximum, and no skill is used initially.
	HWND hwnd = FindTargetHWND();
	if (hwnd == 0)
	{
		tcout << _T("Could not find the target windows!") << endl;
	}
	int step = 0;
	clock_t wait30s, wait15min;
	clock_t wait70s;	// My Kleptos' level is 20, so it would last 70 sec
	wait15min = clock() - 15 * 60 * CLOCKS_PER_SEC + 2000;	// to ensure the first time would execute directly
	while (true)
	{
		switch (step)
		{
		case 0: // use 1, 2, 3, 7 to fight
			Timestamp();
			tcout << _T("Use skill 1237 to make some progress, and wait 30 sec...") << endl;
			EnableProgressMode(hwnd, true);
			UseSkill(hwnd, 1);
			UseSkill(hwnd, 2);
			UseSkill(hwnd, 3);
			UseSkill(hwnd, 7);
			step++;
			wait30s = clock();
			break;
		case 1: // wait 1, 2, 3, 7 skill finish
			for (int i = 0; i < 100; i++)
				FastAttack(hwnd);
			if (clock() - wait30s > 30 * CLOCKS_PER_SEC)
			{
				Timestamp();
				tcout << _T("Time's Up, try to find treasure chest...") << endl;
				step++;
			}
			break;
		case 2:	// wait treasure chest
			for (int i = 0; i < 100; i++)
				Attack(hwnd);
			if (IsTreasureChest(hwnd))
			{
				Timestamp();
				tcout << _T("Find a treasure chest! Use skill 4, 5 and wait 15 min to use Dark Ritual...") << endl;
				UseSkill(hwnd, 4);
				UseSkill(hwnd, 5);
				step = 3;
				wait70s = clock();
			}
			break;
		case 3:	// use Dark Ritual
			if (clock() - wait70s > 70 * CLOCKS_PER_SEC + 2 * CLOCKS_PER_SEC)
			{
				for (int i = 0; i < 100; i++)
					Attack(hwnd);
			}
			else
			{
				for (int i = 0; i < 100; i++)
					FastAttack(hwnd);
			}
			Upgrade(hwnd);
			if (clock() - wait15min > 15 * 60 * CLOCKS_PER_SEC + 2 * CLOCKS_PER_SEC)
			{
				Timestamp();
				tcout << _T("Time's Up, use Dark Ritual and wait 15 min to reload Dark Ritual...") << endl;
				UseSkill(hwnd, 8);
				UseSkill(hwnd, 6);
				UseSkill(hwnd, 9);
				step = 4;
				wait15min = clock();
			}
			break;
		case 4:
			for (int i = 0; i < 100; i++)
				Attack(hwnd);
			if (clock() - wait15min > 15 * 60 * CLOCKS_PER_SEC + 2 * CLOCKS_PER_SEC)
			{
				Timestamp();
				tcout << _T("Time's Up, use Reload and go back to step 0") << endl;
				UseSkill(hwnd, 8);
				UseSkill(hwnd, 9);
				wait15min = clock();
				step = 0;
			}
			break;
		default:
			break;
		}
	}
	system("pause");
	return 0;
}