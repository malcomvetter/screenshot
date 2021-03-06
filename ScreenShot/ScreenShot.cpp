#include "pch.h"
#include <stdio.h>
#include <windows.h>
#include <gdiplus.h>
#pragma comment( lib, "gdiplus" )
#include <time.h>
#include <string>
#include <iostream>
#include <fstream>
#pragma comment(lib, "crypt32.lib")
#include <wincrypt.h>


int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) 
{
	using namespace Gdiplus;
	UINT  num = 0;
	UINT  size = 0;

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;

	GetImageEncoders(num, size, pImageCodecInfo);
	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;
		}
	}
	free(pImageCodecInfo);
	return 0;
}

std::wstring b64Encode(const BYTE* fileBytes, DWORD numFileBytes) 
{
	DWORD b64FileBufferSize;
	CryptBinaryToString(fileBytes, numFileBytes, CRYPT_STRING_BASE64, NULL, &b64FileBufferSize);

	auto b64FileBuffer = new wchar_t[b64FileBufferSize];
	CryptBinaryToString((BYTE*)fileBytes, numFileBytes, CRYPT_STRING_BASE64, b64FileBuffer, &b64FileBufferSize);

	std::wstring b64FileStr(b64FileBuffer);
	delete[] b64FileBuffer;

	return b64FileStr;
}

void screenshot() {
	using namespace Gdiplus;
	IStream* istream;
	HRESULT res = CreateStreamOnHGlobal(NULL, true, &istream);
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	{
		HDC scrdc, memdc;
		HBITMAP membit;
		scrdc = ::GetDC(0);
		int Width = GetSystemMetrics(SM_CXSCREEN);
		int Height = GetSystemMetrics(SM_CYSCREEN);
		memdc = CreateCompatibleDC(scrdc);
		membit = CreateCompatibleBitmap(scrdc, Width, Height);
		HBITMAP hOldBitmap = (HBITMAP)SelectObject(memdc, membit);
		BitBlt(memdc, 0, 0, Width, Height, scrdc, 0, 0, SRCCOPY);

		Gdiplus::Bitmap bitmap(membit, NULL);
		CLSID clsid;
		GetEncoderClsid(L"image/png", &clsid);
		//bitmap.Save(L"screen.png", &clsid, NULL); //save to file
		bitmap.Save(istream, &clsid, NULL);

		//save bitmap to clipboard
		OpenClipboard(NULL);
		EmptyClipboard();
		SetClipboardData(CF_BITMAP, membit);
		CloseClipboard();

		//move PNG to stream
		STATSTG ss;
		if (istream->Stat(&ss, STATFLAG_NONAME) == S_OK && ss.cbSize.HighPart == 0)
		{
			BYTE* buffer = new BYTE[ss.cbSize.QuadPart];
			ULONG bytesSaved = 0;
			//reset stream to beginning
			LARGE_INTEGER li;
			li.QuadPart = 0;
			istream->Seek(li, STREAM_SEEK_SET, NULL);
			//read stream into buffer
			auto result = istream->Read(buffer, ss.cbSize.QuadPart, &bytesSaved);
			//b64
			std::wstring b64 = b64Encode(buffer, ss.cbSize.LowPart);
			while (b64.find(L"\r\n") != std::wstring::npos)
			{
				b64.erase(b64.find(L"\r\n"), 2);
			}

			//do something with the b64 string ... how about writing to disk in html?
			std::wofstream myfile;
			myfile.open("screenshot.html");
			myfile << L"<html><body><img src=\"data:image/png;base64,";
			myfile << b64;
			myfile << L"\"></body></html>";
			myfile.close();
			free(buffer);
		}
		//cleanup
		istream->Release();
		DeleteObject(memdc);
		DeleteObject(membit);
		::ReleaseDC(0, scrdc);
	}
	GdiplusShutdown(gdiplusToken);
}

int main()
{
	screenshot();
	return 0;
}
