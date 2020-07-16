#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <ctime>
#include <cmath>
#include <vector>
#include <sstream>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

#define TIMER_ID 321
#define SafeRelease(p) if (p != NULL) p->Release(); p = NULL;
constexpr double pi = 3.141592653589793238463;
constexpr double g = 1;
constexpr float g_radius = 15.0f;
constexpr double g_airResistance = 1;// 0.999;

namespace d2d {
    int wndSizeX, wndSizeY;

    PAINTSTRUCT ps;
    ID2D1HwndRenderTarget* pRenderTarget = NULL;
    ID2D1SolidColorBrush* pBallBrush = NULL;
    ID2D1SolidColorBrush* pLineBrush = NULL;
    ID2D1Factory* pFactory = NULL;

    IDWriteTextFormat* pTextFormat = NULL;
    //IDWriteTextLayout* pTextLayout = NULL;
    IDWriteFactory* pWriteFactory = NULL;

    bool CreateDeviceIndependent(HWND hWnd) {

        if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory))) {
            DestroyWindow(hWnd);
            return true;
        }

        if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&pWriteFactory)))) {
            DestroyWindow(hWnd);
            return true;
        }

        if (FAILED(pWriteFactory->CreateTextFormat(L"Lucidia Console", NULL,
            DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"Russia", &pTextFormat))) {
            DestroyWindow(hWnd);
            return true;
        }
    }

    void CreateDeviceDependent(HWND hWnd) {
        RECT rc;
        GetClientRect(hWnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right-rc.left, rc.bottom-rc.top);
        wndSizeX = rc.right-rc.left;
        wndSizeY = rc.bottom-rc.top;

        if (FAILED(pFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hWnd, size),
            &pRenderTarget))) {
            DestroyWindow(hWnd);
            return;
        }

        if (FAILED(pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.5f, 0.5f, 0.5f), &pBallBrush))) {
            DestroyWindow(hWnd);
            return;
        }

        if (FAILED(pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.7f, 0.9f, 1.0f), &pLineBrush))) {
            DestroyWindow(hWnd);
            return;
        }
    }

    void DestroyResources() {
        SafeRelease(pLineBrush);
        SafeRelease(pBallBrush);
        SafeRelease(pRenderTarget);
    }

    void BeginDraw(HWND hWnd) {
        BeginPaint(hWnd, &ps);
        if (pRenderTarget==NULL) {
            CreateDeviceDependent(hWnd);
        }

        pRenderTarget->BeginDraw();
    }
    void EndDraw(HWND hWnd) {
        HRESULT hr = pRenderTarget->EndDraw();
        if (FAILED(hr)||hr==D2DERR_RECREATE_TARGET) {
            DestroyWindow(hWnd);
        }
        EndPaint(hWnd, &ps);
    }
}

namespace l {
    struct Pendulum {
        POINT pos, vertexPos;
        double acceleration, speed;
        double angle;
        double length;
        double mass;

        Pendulum() {
            acceleration = 0.0;
            speed = 0.0;
            angle = 0;
            length = 200;
            mass = 0.05;
        }

        void calculatePosition() {
            pos.x = vertexPos.x+(length*sin(angle));
            pos.y = vertexPos.y+(length*cos(angle));
        }

        __inline void calculateAngle() {
            speed += acceleration;
            speed *= g_airResistance;
            angle += speed;
            while (angle>2*pi)
                angle -= 2*pi;
            while (angle<-2*pi)
                angle += 2*pi;
        }

    };

    int timeStamp = 0,
        timeInterval = 16;
    Pendulum primary, secondary;

    void initialize() {
        primary.vertexPos.x = d2d::wndSizeX/2;
        primary.vertexPos.y = 2*d2d::wndSizeY/5;
        primary.calculatePosition();

        secondary.vertexPos = primary.pos;
        secondary.calculatePosition();

        timeStamp = clock();

        primary.angle = pi/2;
        secondary.angle = pi/2;
    }


    void resize() {
        primary.vertexPos.x = d2d::wndSizeX/2;
        primary.vertexPos.y = 2*d2d::wndSizeY/5;
    }

    void tick() {
        {
            double num1, num2, num3, num4, den;
#define m1 primary.mass
#define m2 secondary.mass
#define l1 primary.length
#define l2 secondary.length
#define s1 primary.speed
#define s2 secondary.speed
#define a1 primary.angle
#define a2 secondary.angle


            den = 2.0*m1+m2-m2*cos(2.0*a1-2.0*a2);

            num1 = -g*(2.0*m1+m2)*sin(a1);
            num2 = -m2*g*sin(a1-2.0*a2);
            num3 = -2.0*sin(a1-a2)*m2;
            num4 = s2*s2*l2+s1*s1*l1*cos(a1-a2);

            primary.acceleration = (num1+num2+num3*num4)/(l1*den);
            primary.acceleration = (-g*(2*m1+m2)*sin(a1)-m2*g*sin(a1-2*a2)-2*sin(a1-a2)*m2*(s2*s2*l2+s1*s1*l1*cos(a1-a2)))/
                (l1*(2*m1+m2-m2*cos(2*a1-2*a2)));

            num1 = 2.0*sin(a1-a2);
            num2 = s1*s1*l1*(m1+m2);
            num3 = g*(m1+m2)*cos(a1);
            num4 = s2*s2*l2*m2*cos(a1-a2);

            secondary.acceleration = num1*(num2+num3+num4)/(l2*den);
            secondary.acceleration = (2*sin(a1-a2)*(s1*s1*l1*(m1+m2)+g*(m1+m2)*cos(a1)+s2*s2*l2*m2*cos(a1-a2)))/
                (l2*(2*m1+m2-m2*cos(2*a1-2*a2)));
        }

        primary.calculateAngle();
        secondary.calculateAngle();

        primary.calculatePosition();
        secondary.vertexPos = primary.pos;
        secondary.calculatePosition();
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
    srand(clock());

    WNDCLASSEX wcex;
    ZeroMemory(&wcex, sizeof(wcex));
    wcex.cbSize = sizeof(wcex);
    wcex.style = HS_VERTICAL|HS_HORIZONTAL;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInst;
    wcex.hCursor = LoadCursor(hInst, IDC_ARROW);
    wcex.lpszClassName = L"CLA";
    wcex.hIcon = LoadIcon(hInst, IDI_APPLICATION);
    wcex.hIconSm = LoadIcon(hInst, IDI_APPLICATION);
    wcex.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);

    RegisterClassEx(&wcex);

    HWND hWnd = CreateWindow(L"CLA", L"Double Pendulum ", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;

    SetTimer(hWnd, TIMER_ID, 17, NULL);

    BOOL ret = 1;
    while (ret>0) {
        ret = GetMessage(&msg, NULL, 0, 0);
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    KillTimer(hWnd, TIMER_ID);

    return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static std::vector<POINT> points;
    switch (msg) {

    case WM_CREATE:

        if (d2d::CreateDeviceIndependent(hWnd))
            return 0;

        d2d::CreateDeviceDependent(hWnd);

        l::initialize();
        break;

    case WM_SIZE:
        d2d::DestroyResources();
        d2d::CreateDeviceDependent(hWnd);
        l::resize();
        points.clear();
        break;

    case WM_TIMER:
        //InvalidateRect(hWnd, 0, false);
        break;

    case WM_PAINT:
    {
        if (clock()-l::timeStamp>l::timeInterval) {
            l::timeStamp = clock();
            l::tick();
            points.push_back({ l::secondary.pos });
        }


        d2d::BeginDraw(hWnd);

        d2d::pRenderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f));

        d2d::pRenderTarget->DrawLine(D2D1::Point2F(l::primary.vertexPos.x, l::primary.vertexPos.y),
            D2D1::Point2F(l::primary.pos.x, l::primary.pos.y), d2d::pLineBrush);
        d2d::pRenderTarget->DrawLine(D2D1::Point2F(l::secondary.vertexPos.x, l::secondary.vertexPos.y),
            D2D1::Point2F(l::secondary.pos.x, l::secondary.pos.y), d2d::pLineBrush);

        d2d::pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(l::primary.pos.x, l::primary.pos.y), g_radius, g_radius), d2d::pBallBrush);
        d2d::pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(l::secondary.pos.x, l::secondary.pos.y), g_radius, g_radius), d2d::pBallBrush);
        
        for (int i = 1; i<points.size(); i++) {
            POINT p1 = points[i-1],
                p2 = points[i];
            d2d::pRenderTarget->DrawLine(D2D1::Point2F(p1.x, p1.y), D2D1::Point2F(p2.x, p2.y), d2d::pLineBrush, 3.0f);
        }

        {
            std::wstringstream sstr;

            //primary parameters
            sstr<<"first:\0";
            d2d::pRenderTarget->DrawText(sstr.str().c_str(), sstr.str().size(), d2d::pTextFormat, D2D1::RectF(0.0f, 0.0f, 300.0f, 10.0f), d2d::pLineBrush);
            sstr.str(L"");

            sstr<<WCHAR(952)<<":   "<<int(l::primary.angle*180/pi)<<'\0';
            d2d::pRenderTarget->DrawText(sstr.str().c_str(), sstr.str().size(), d2d::pTextFormat, D2D1::RectF(0.0f, 14.0f, 300.0f, 24.0f), d2d::pLineBrush);
            sstr.str(L"");

            sstr<<WCHAR(952)<<"`:  "<<int(l::primary.speed*180*60/pi)<<'\0';
            d2d::pRenderTarget->DrawText(sstr.str().c_str(), sstr.str().size(), d2d::pTextFormat, D2D1::RectF(0.0f, 28.0f, 300.0f, 38.0f), d2d::pLineBrush);
            sstr.str(L"");

            sstr<<WCHAR(952)<<"``: "<<int(l::primary.acceleration*180*60/pi)<<'\0';
            d2d::pRenderTarget->DrawText(sstr.str().c_str(), sstr.str().size(), d2d::pTextFormat, D2D1::RectF(0.0f, 42.0f, 300.0f, 52.0f), d2d::pLineBrush);
            sstr.str(L"");

            //secondary parameters
            sstr<<"second:\0";
            d2d::pRenderTarget->DrawText(sstr.str().c_str(), sstr.str().size(), d2d::pTextFormat, D2D1::RectF(0.0f, 70.0f, 300.0f, 80.0f), d2d::pLineBrush);
            sstr.str(L"");

            sstr<<WCHAR(952)<<":   "<<int(l::secondary.angle*180/pi)<<'\0';
            d2d::pRenderTarget->DrawText(sstr.str().c_str(), sstr.str().size(), d2d::pTextFormat, D2D1::RectF(0.0f, 84.0f, 300.0f, 94.0f), d2d::pLineBrush);
            sstr.str(L"");

            sstr<<WCHAR(952)<<"`:  "<<int(l::secondary.speed*180*60/pi)<<'\0';
            d2d::pRenderTarget->DrawText(sstr.str().c_str(), sstr.str().size(), d2d::pTextFormat, D2D1::RectF(0.0f, 98.0f, 300.0f, 108.0f), d2d::pLineBrush);
            sstr.str(L"");

            sstr<<WCHAR(952)<<"``: "<<int(l::secondary.acceleration*180*60/pi)<<'\0';
            d2d::pRenderTarget->DrawText(sstr.str().c_str(), sstr.str().size(), d2d::pTextFormat, D2D1::RectF(0.0f, 112.0f, 300.0f, 122.0f), d2d::pLineBrush);
            sstr.str(L"");
        }

        d2d::EndDraw(hWnd);

        InvalidateRect(hWnd, 0, false);
    }
    break;

    case WM_DESTROY:
        d2d::DestroyResources();
        SafeRelease(d2d::pTextFormat);
        SafeRelease(d2d::pWriteFactory);
        SafeRelease(d2d::pFactory);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}
