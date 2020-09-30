#include <Windows.h>
#include <gdiplus.h>
#include <atlimage.h>
#include <string>
#include <commctrl.h>
#pragma comment(lib, "gdiplus.lib")
using namespace std;

#define SINGLE_STEP_PIXELS 3

#define DIRECTION int
#define LEFT 0
#define UP 4
#define RIGHT 8
#define DOWN 12

#define RECTANGLE 100
#define PAINT 200

const int IDM_Rectangle = 0;
const int IDM_Picture = 1;
const int IDM_Cut = 2;
const int rect_size = 50;
const int x_chor = 100;
const int y_chor = 100;
int status;

RECT movableRect = { x_chor, y_chor, x_chor + 50, y_chor+50 };
RECT clientRect;




const HBRUSH WINDOW_BACKGROUND_BRUSH = CreateSolidBrush(RGB(100, 149, 237));
const HBRUSH MOVABLE_RECT_BRUSH = CreateSolidBrush(RGB(221, 160, 221));

HBITMAP movableBitmap;

// declarations
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void dragMovableRect(DIRECTION direction, int offset);
int getAllowedOffset(DIRECTION direction, int offset);
void correctChordsMouse();
void drawMovableBitmap(HDC hdc);
HBITMAP pngFileToHbitmap(WCHAR * pngFilePath);
void CaptureAnImage(HWND hWnd,string filename);
void saveToFile(HBITMAP hBitmap, string filename);
void changeMenuState(HMENU hMenu, int status);
void saveFile(HWND hWnd);

#define MOVABLE_IMAGE_PATH L"C:\\Users\\Владелец\\source\\repos\\OSISP_1_new\\OSISP_1_new\\face.png"




INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nCmdShow) {
	HWND mainWindow; //window descriptor
	WNDCLASSEX windowClass; //message structure
	MSG receivedMessage; //window class structure

	LPCSTR const className = "ClassName";
	LPCSTR windowName = "SPRITE";

	//Fill windowClass structure
	windowClass.cbSize = sizeof(windowClass);
	windowClass.style = 0;
	windowClass.lpfnWndProc = (WNDPROC)WindowProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = hInstance;
	windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground = WINDOW_BACKGROUND_BRUSH;
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = className;
	windowClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	//Register class
	if (!RegisterClassEx(&windowClass)) {
		const wchar_t* const CLASS_REG_ERROR = L"Cannot register class";
		MessageBoxW(NULL, CLASS_REG_ERROR, NULL, MB_OK);
		return 0;
	}

	//Create window
	mainWindow = CreateWindowEx(
		0, className,
		windowName,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE, //window style
		500, 200, // left heigh 
		500, 450, // width and heigh
		NULL,     //link to parent window
		NULL,     //use window menu class
		hInstance,//lint to present app
		NULL      //use as lParam in WM_CREATE
	);

	if (!mainWindow) {
		const wchar_t* const WND_CREATE_ERROR = L"Cannot create window";
		MessageBoxW(NULL, WND_CREATE_ERROR, NULL, MB_OK);
		return 0;
	}

	//Show current window
	ShowWindow(mainWindow, nCmdShow); //show

	//message loop until the application is closed
	while (GetMessage(&receivedMessage, NULL, 0, 0)) {
		TranslateMessage(&receivedMessage);
		DispatchMessage(&receivedMessage);
	}

	return receivedMessage.wParam;
}


LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	PAINTSTRUCT paintStruct;
	HDC hdc;
	int wheelOffset;
	HMENU hMenu,hStatus;
	hMenu = CreateMenu();
	hStatus = CreateMenu();

	switch (uMsg) {
	case WM_CREATE:
		AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hStatus, "Status");
		AppendMenu(hMenu, MF_POPUP, IDM_Cut, "Shortcut");
		AppendMenu(hMenu, MF_SEPARATOR, NULL, NULL);
		AppendMenu(hStatus, MF_BYCOMMAND | MF_CHECKED, IDM_Rectangle, "Rectangle");
		AppendMenu(hStatus, MF_BYCOMMAND | MF_UNCHECKED, IDM_Picture, "Picture");
		SetMenu(hWnd, hMenu);
		movableBitmap = pngFileToHbitmap((WCHAR*)MOVABLE_IMAGE_PATH);
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &paintStruct);
		GetClientRect(hWnd, &clientRect);
		FillRect(hdc, &clientRect, WINDOW_BACKGROUND_BRUSH);
		if (status == PAINT) {
			drawMovableBitmap(hdc);
		}
		else
			FillRect(hdc, &movableRect, MOVABLE_RECT_BRUSH);

		EndPaint(hWnd, &paintStruct);

		break;
	case WM_COMMAND:

		switch (LOWORD(wParam)){
		case IDM_Rectangle:	{
			status = RECTANGLE;
			hStatus = GetSubMenu(GetMenu(hWnd), 0);
			changeMenuState(hStatus, status);
		}; break;

		case IDM_Picture:	{
			status = PAINT;
			hStatus = GetSubMenu(GetMenu(hWnd), 0);
			changeMenuState(hStatus, status);
		}; break;
		case IDM_Cut:																			///////////////////////////////////
			saveFile(hWnd);
		}; break;

	case WM_MENUSELECT:
		switch (LOWORD(wParam)) {
		case IDM_Rectangle:
			SendMessage(hWnd, SB_SETTEXT, SBT_NOBORDERS, (LPARAM)"0");
			break;
		case IDM_Picture:
			SendMessage(hWnd, SB_SETTEXT, SBT_NOBORDERS, (LPARAM)"1");
			break;
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_MOUSEWHEEL:
		GET_KEYSTATE_WPARAM(wParam) == MK_SHIFT ?
			GET_WHEEL_DELTA_WPARAM(wParam) > 0 ?  dragMovableRect(RIGHT, SINGLE_STEP_PIXELS) : dragMovableRect(LEFT, SINGLE_STEP_PIXELS):
			GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? dragMovableRect(UP, SINGLE_STEP_PIXELS) : dragMovableRect(DOWN, SINGLE_STEP_PIXELS);
		InvalidateRect(hWnd, &clientRect, false);
		break;

	case WM_KEYDOWN:
		switch (wParam) {
		case VK_LEFT:
			dragMovableRect(LEFT, SINGLE_STEP_PIXELS);
			break;
		case VK_RIGHT:
			dragMovableRect(RIGHT, SINGLE_STEP_PIXELS);
			break;
		case VK_UP:
			dragMovableRect(UP, SINGLE_STEP_PIXELS);
			break;
		case VK_DOWN:
			dragMovableRect(DOWN, SINGLE_STEP_PIXELS);
			break;
		}

		InvalidateRect(hWnd, &clientRect, false);
		break;
	case WM_MOUSEMOVE: {
		int mouseXPos = LOWORD(lParam) - (rect_size / 2);
		int mouseYPos = HIWORD(lParam) - (rect_size / 2);

		if ((mouseXPos > 0)
			&& (mouseXPos < clientRect.right - rect_size)
			&& (mouseYPos > 0)
			&& (mouseYPos < clientRect.bottom - rect_size)) {

			 if (movableRect.top <= mouseYPos)  dragMovableRect(DOWN, mouseYPos - movableRect.top);
			 if (movableRect.left >= mouseXPos) dragMovableRect(LEFT, movableRect.left - mouseXPos);
			 if (movableRect.top >= mouseYPos)  dragMovableRect(UP, movableRect.top - mouseYPos);
			 if (movableRect.left <= mouseXPos)	dragMovableRect(RIGHT, mouseXPos - movableRect.left);
		}

		correctChordsMouse();
	}
	InvalidateRect(hWnd, &clientRect, false);
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

void dragMovableRect(DIRECTION direction, int offset) {

	offset = getAllowedOffset(direction, offset);

	switch (direction) {
	case UP:
		OffsetRect(&movableRect, 0, -offset);
		break;
	case DOWN:
		OffsetRect(&movableRect, 0, offset);
		break;
	case LEFT:
		OffsetRect(&movableRect, -offset, 0);
		break;
	case RIGHT:
		OffsetRect(&movableRect, offset, 0);
		break;
	}
}

int getAllowedOffset(DIRECTION direction, int offset) {
	LONG firstPosition;
	LONG secondPosition;

	switch (direction) {
	case UP:
		firstPosition = movableRect.top - offset;
		secondPosition = clientRect.top;
		break;
	case DOWN:
		firstPosition = clientRect.bottom;
		secondPosition = movableRect.bottom + offset;
		break;
	case LEFT:
		firstPosition = movableRect.left - offset;
		secondPosition = clientRect.left;
		break;
	case RIGHT:
		firstPosition = clientRect.right;
		secondPosition = movableRect.right + offset;
		break;
	}

	if ((secondPosition - firstPosition) > -1) {
		return -5 * offset;
	}

	return offset;
}

void correctChordsMouse() {
	if (movableRect.left <= clientRect.left+3 ) dragMovableRect(RIGHT, 20);
	if (movableRect.top <= clientRect.top + 3) dragMovableRect(DOWN, 20);
	if (movableRect.right >= clientRect.right - 3) dragMovableRect(LEFT, 20);
	if (movableRect.bottom >= clientRect.bottom - 3) dragMovableRect(UP, 20);
}

void drawMovableBitmap(HDC hdc) {
	HBITMAP hbm, receivedBitmap;

	HDC tempDC;
	BITMAP bitmap;
	POINT  bitmapSize, zeroPoint;

	tempDC = CreateCompatibleDC(hdc);

	receivedBitmap = (HBITMAP)SelectObject(tempDC, movableBitmap);

	if (receivedBitmap) {

		SetMapMode(tempDC, GetMapMode(hdc));
		GetObject(movableBitmap, sizeof(BITMAP), (LPSTR)&bitmap);

		bitmapSize.x = bitmap.bmWidth;
		bitmapSize.y = bitmap.bmHeight;
		DPtoLP(hdc, &bitmapSize, 1);

		zeroPoint.x = 0;
		zeroPoint.y = 0;

		DPtoLP(tempDC, &zeroPoint, 1);
		BitBlt(hdc, movableRect.left, movableRect.top, bitmapSize.x, bitmapSize.y, tempDC, zeroPoint.x, zeroPoint.y, SRCCOPY);
		SelectObject(tempDC, receivedBitmap);
	}

	DeleteDC(tempDC);
}

HBITMAP pngFileToHbitmap(WCHAR* pngFilePath) {
	Gdiplus::GdiplusStartupInput gdiPlusStartupIntpu;
	ULONG_PTR gdiPlusToken;
	GdiplusStartup(&gdiPlusToken, &gdiPlusStartupIntpu, NULL);

	HBITMAP convertedBitmap = NULL;
	Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(pngFilePath, false);
	if (bitmap) {
		bitmap->GetHBITMAP(RGB(237, 149, 100), &convertedBitmap);
		delete bitmap;
	}
	Gdiplus::GdiplusShutdown(gdiPlusToken);
	return convertedBitmap;
}

void CaptureAnImage(HWND hwnd, string filename) {
	// get the device context of the screen
	HDC hScreenDC = GetDC(hwnd);
	// and a device context to put it in
	HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

	int width = clientRect.right;
	int height = clientRect.bottom;

	// maybe worth checking these are positive values
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);

	// get a new bitmap
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

	BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);
	hBitmap = (HBITMAP)SelectObject(hMemoryDC, hOldBitmap);

	// clean up
	DeleteDC(hMemoryDC);
	DeleteDC(hScreenDC);

	saveToFile(hBitmap, filename);
}

void saveToFile(HBITMAP hBitmap, string filename) {
	CImage image;
	image.Attach(hBitmap);
	image.Save(filename.c_str());
}

void changeMenuState(HMENU hMenu, int status) {
	CheckMenuItem(hMenu, IDM_Rectangle, MF_BYCOMMAND | (status == RECTANGLE ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, IDM_Picture, MF_BYCOMMAND | (status == PAINT ? MF_CHECKED : MF_UNCHECKED));
}

void saveFile(HWND hWnd) {
	TCHAR szFilters[] = ("JPG File (*.jpg)\0*.jpg\0\0");
	TCHAR szFilePathName[MAX_PATH] = ("");
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = szFilters;
	ofn.lpstrFile = szFilePathName;  // This will hold the file name
	ofn.lpstrDefExt = ("jpg");
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = ("Save File");
	ofn.Flags = OFN_OVERWRITEPROMPT;

	// Open the file save dialog, and choose the file name
	GetSaveFileName(&ofn);
	CaptureAnImage(hWnd, ofn.lpstrFile);
}
