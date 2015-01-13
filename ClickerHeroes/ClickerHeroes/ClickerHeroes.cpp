#include <Windows.h>
#include <tchar.h>
#include <iostream>
#include <time.h>
#include <iomanip>
using namespace std;

// configuration
#define CLK_15MIN (15*60*CLOCKS_PER_SEC)
#define CLK_30S (30*CLOCKS_PER_SEC)
#define CLK_70S (70*CLOCKS_PER_SEC)
#define INIT_STEP 0	// 0
//0: init; 1: wait 30s until attacking skill finish; 2: seeking treasure; 
//3: wait 70s until golden strike finish; wait 15min until dark ritual ready; 
//4: wait 15min until reload ready
#define INIT_WAIT15MIN (-CLK_15MIN)	// (-CLK_15MIN)
#define INIT_WAIT30S (-0)	// (-0)
#define INIT_WAIT70S (-0)	// (-0)
#define MAX_COUNT 30		// max number of attempt to refresh page to find a treasure

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
bool IsLoadComplete(HWND);
void UseSkill(HWND, int);
void EnableProgressMode(HWND, bool);
void Upgrade(HWND);
void Attack(HWND);

void Click(HWND, int, int);

void Timestamp()
{
	clock_t ts = clock();
	int sec = ts / CLOCKS_PER_SEC;
	tcout << setfill(_T('0')) << setw(2) << sec/3600 << _T(":") << setfill(_T('0')) << setw(2) << (sec/60)%60 << _T(":") << setfill(_T('0')) << setw(2) << sec%60 << _T(" ");
}

#define DELAY 50
#define FASTDELAY 10
#define ERROR_FOUND ((1<<29)+1)
BOOL CALLBACK FindMozillaToRefresh(HWND hwnd, LPARAM ret)
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
			Click(hwnd, 1341, 51);
			return FALSE;
		}
	}
	return TRUE;
}
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
HWND RefreshMozilla()
{
	HWND hwnd = 0;
	BOOL bRet = EnumWindows(FindMozillaToRefresh, (LPARAM)&hwnd);
	if (!bRet && GetLastError()!=ERROR_FOUND)
	{
		tcout << _T("EnumWindows(FindMozilla) Fail; GetLastError() = ") << GetLastError() << endl;
		system("pause");
		return 0;
	}
	Sleep(3000);
	return FindTargetHWND();
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
	int x = 1114;
	int y = 210;
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
	bool found = false;
	int x = 955;
	int y = 313;
	COLORREF clr = GetPixel(dc, x, y);
	found = (GetRValue(clr) == 169 && GetGValue(clr) == 110 && GetBValue(clr) == 37);
	ReleaseDC(hwnd, dc);
	return found;	// if found chest color
}
bool IsLoadComplete(HWND hwnd)
{
	// must NOT resize (i.e. 100%)
	HDC dc = GetDC(hwnd);
	RECT rect;
	GetWindowRect(hwnd, &rect);
	int x = 557;
	int y = 277;
	bool found = false;
	COLORREF clr = GetPixel(dc, x, y);
	found = (GetRValue(clr) == 254 && GetGValue(clr) == 254 && GetBValue(clr) == 254);
	ReleaseDC(hwnd, dc);
	return found;	// if found color of play button
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
	int step = INIT_STEP;
	clock_t now = clock();
	clock_t wait30s = now + INIT_WAIT30S;
	clock_t wait15min = now + INIT_WAIT15MIN;	// to ensure the first time would execute directly
	clock_t wait70s = now + INIT_WAIT70S;	// My Kleptos' level is 20, so it would last 70 sec
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
			step = 1;
			wait30s = clock();
			break;
		case 1: // wait 1, 2, 3, 7 skill finish
			for (int i = 0; i < 100; i++)
				Attack(hwnd);
			if (clock() - wait30s > CLK_30S)
			{
				Timestamp();
				tcout << _T("Time's Up, try to find treasure chest...") << endl;
				step = 2;
			}
			break;
		case 2:	// wait treasure chest
			for (int i = 0; i < 100; i++)
				Attack(hwnd);
			if (!IsTreasureChest(hwnd))
			{
				if (clock() - wait15min > CLK_15MIN*2/3)
				{
					Timestamp();
					tcout << _T("It took too long... Try refresh to speed up the seeking process...") << endl;		
					int count = 0;
					do
					{
						count++;
						tcout << count << _T(" attempt...\r");
						if (count > MAX_COUNT)
						{
							tcout << endl;
							Timestamp();
							tcout << _T("Too many attempt... Might be stuck in boss stage, so try to retreat from...") << endl;		
							// probability ~= 0.9^30 ~= 0.05, maybe stuck in the boss stage
							Click(hwnd, 557, 277);	// click play
							Sleep(5000);
							Click(hwnd, 935, 75);	// click close message
							Sleep(3000);
							EnableProgressMode(hwnd, true);
							Sleep(63 * CLOCKS_PER_SEC);		// to retreat from boss stage; wait 1 min to save
							count = 0;
						}
						hwnd = RefreshMozilla();
						do
						{
							Sleep(1000);
						}
						while (!IsLoadComplete(hwnd));
						Click(hwnd, 557, 277);	// click play
						Sleep(5000);
					}
					while (!IsTreasureChest(hwnd));
					tcout << endl;
					Click(hwnd, 935, 75);	// click close message
					Sleep(3000);
					for (int i = 0; i < 10; i++)	// click scroll bar to let the Samurai to be the target hero
					{
						Click(hwnd, 549, 620);
						Sleep(200);
					}
				}
				else
					continue;
			}
			Timestamp();
			tcout << _T("Find a treasure chest! Use skill 4, 5 and wait 70 sec...") << endl;
			UseSkill(hwnd, 4);
			UseSkill(hwnd, 5);
			step = 3;
			wait70s = clock();
			break;
		case 3:	// wait golden strike
			for (int i = 0; i < 100; i++)
				FastAttack(hwnd);
			if (clock() - wait70s > CLK_70S)
			{
				Timestamp();
				tcout << _T("Golden strike finished, upgrade heroes and wait 15 min to use Dark Ritual...") << endl;
				for (int i = 0; i < 25; i++)
					Upgrade(hwnd);
				step = 4;
			}
			break;
		case 4: // wait 15 min to use Dark Ritual
			for (int i = 0; i < 100; i++)
				Attack(hwnd);
			if (clock() - wait15min > CLK_15MIN)
			{
				Timestamp();
				tcout << _T("Time's Up, use Dark Ritual and wait 15 min to reload Dark Ritual...") << endl;
				UseSkill(hwnd, 8);
				UseSkill(hwnd, 6);
				UseSkill(hwnd, 9);
				step = 5;
				wait15min = clock();
			}
			break;
		case 5:	// wait 15 min to reload dark ritual
			for (int i = 0; i < 100; i++)
				Attack(hwnd);
			if (clock() - wait15min > CLK_15MIN)
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