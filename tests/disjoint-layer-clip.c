#define COBJMACROS
// Pixel regression for FireMonkey's disjoint dirty-region layer masks.
#include <windows.h>
#include <d3d11.h>
#include <d2d1.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static void check(HRESULT hr, const char *what)
{
    if (FAILED(hr)) { printf("%s failed: %#lx\n", what, (unsigned long)hr); exit(2); }
}
static ID2D1PathGeometry *mask(ID2D1Factory *factory, const D2D1_RECT_F *rects, unsigned count)
{
    ID2D1PathGeometry *path;
    ID2D1GeometrySink *sink;
    check(ID2D1Factory_CreatePathGeometry(factory, &path), "path");
    check(ID2D1PathGeometry_Open(path, &sink), "sink");
    ID2D1GeometrySink_SetFillMode(sink, D2D1_FILL_MODE_WINDING);
    for (unsigned i = 0; i < count; ++i)
    {
        D2D1_RECT_F r = rects[i];
        D2D1_POINT_2F points[] = {{r.right,r.top},{r.right,r.bottom},{r.left,r.bottom}};
        ID2D1GeometrySink_BeginFigure(sink, ((D2D1_POINT_2F){r.left,r.top}), D2D1_FIGURE_BEGIN_FILLED);
        ID2D1GeometrySink_AddLines(sink, points, 3);
        ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    }
    check(ID2D1GeometrySink_Close(sink), "close sink");
    ID2D1GeometrySink_Release(sink);
    return path;
}
static void push(ID2D1RenderTarget *rt, ID2D1Geometry *geometry, float tx, float ty)
{
    D2D1_LAYER_PARAMETERS p = {0};
    p.contentBounds = (D2D1_RECT_F){0,0,64,64};
    p.geometricMask = geometry;
    p.maskAntialiasMode = D2D1_ANTIALIAS_MODE_ALIASED;
    p.maskTransform = (D2D1_MATRIX_3X2_F){.m11=1,.m22=1,.dx=tx,.dy=ty};
    p.opacity = 1;
    ID2D1RenderTarget_PushLayer(rt, &p, NULL);
}
int main()
{
    ID3D11Device *device;
    ID3D11DeviceContext *context;
    ID3D11Texture2D *texture, *staging;
    IDXGISurface *surface;
    ID2D1Factory *factory;
    ID2D1RenderTarget *rt;
    ID2D1SolidColorBrush *brush;
    check(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            NULL, 0, D3D11_SDK_VERSION, &device, NULL, &context), "device");
    D3D11_TEXTURE2D_DESC desc = {0};
    desc.Width = desc.Height = 64; desc.MipLevels = desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; desc.SampleDesc.Count = 1;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    check(ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture), "texture");
    desc.BindFlags = 0; desc.Usage = D3D11_USAGE_STAGING; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    check(ID3D11Device_CreateTexture2D(device, &desc, NULL, &staging), "staging");
    check(ID3D11Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface), "surface");
    check(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory), "factory");
    D2D1_RENDER_TARGET_PROPERTIES props = {0};
    props.pixelFormat = (D2D1_PIXEL_FORMAT){DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED};
    props.dpiX = props.dpiY = 96;
    check(ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory, surface, &props, &rt), "render target");
    check(ID2D1RenderTarget_CreateSolidColorBrush(rt, (&(D2D1_COLOR_F){0,0,1,1}), NULL, &brush), "brush");
    D2D1_RECT_F rects[] = {{0,0,16,16},{32,0,48,16}};
    D2D1_RECT_F inner_rect = {8,0,40,16};
    D2D1_RECT_F overlap[] = {{0,0,32,16},{16,0,48,16}};
    ID2D1PathGeometry *disjoint = mask(factory, rects, 2);
    ID2D1PathGeometry *inner = mask(factory, &inner_rect, 1);
    ID2D1PathGeometry *overlapping = mask(factory, overlap, 2);
    int failures = 0;
    for (unsigned test = 0; test < 5; ++test)
    {
        ID2D1RenderTarget_BeginDraw(rt);
        ID2D1RenderTarget_SetTransform(rt, (&(D2D1_MATRIX_3X2_F){.m11=1,.m22=1}));
        ID2D1RenderTarget_Clear(rt, (&(D2D1_COLOR_F){1,0,0,1}));
        push(rt, (ID2D1Geometry *)(test == 3 ? overlapping : disjoint), test == 4 ? 4 : 0, test == 4 ? 8 : 0);
        if (test == 2) push(rt, (ID2D1Geometry *)inner, 0, 0);
        if (test == 0 || test == 2 || test == 3)
        {
            ID2D1SolidColorBrush_SetOpacity(brush, test == 3 ? 0.5f : 1.0f);
            ID2D1RenderTarget_FillRectangle(rt, (&(D2D1_RECT_F){0,0,64,64}), (ID2D1Brush *)brush);
        }
        else ID2D1RenderTarget_Clear(rt, (&(D2D1_COLOR_F){0,0,1,1}));
        if (test == 2) ID2D1RenderTarget_PopLayer(rt);
        ID2D1RenderTarget_PopLayer(rt);
        // The clip must no longer restrict a draw after PopLayer.
        ID2D1SolidColorBrush_SetOpacity(brush, 1);
        ID2D1RenderTarget_FillRectangle(rt, (&(D2D1_RECT_F){0,48,64,64}), (ID2D1Brush *)brush);
        check(ID2D1RenderTarget_EndDraw(rt, NULL, NULL), "EndDraw");
        ID3D11DeviceContext_CopyResource(context, (ID3D11Resource *)staging, (ID3D11Resource *)texture);
        D3D11_MAPPED_SUBRESOURCE mapped;
        check(ID3D11DeviceContext_Map(context, (ID3D11Resource *)staging, 0, D3D11_MAP_READ, 0, &mapped), "readback");
        unsigned mismatches = 0;
        for (unsigned y=0; y<64; ++y) for (unsigned x=0; x<64; ++x)
        {
            bool blue;
            if (test == 3) blue = x < 48 && y < 16;
            else if (test == 4) blue = y >= 8 && y < 24 && ((x >= 4 && x < 20) || (x >= 36 && x < 52));
            else blue = y < 16 && (x < 16 || (x >= 32 && x < 48));
            if (test == 2) blue = blue && x >= 8 && x < 40;
            unsigned expected_b = blue ? (test == 3 ? 128 : 255) : 0;
            unsigned expected_r = blue ? (test == 3 ? 127 : 0) : 255;
            if (y >= 48) { expected_b = 255; expected_r = 0; }
            const unsigned char *pixel = (const unsigned char *)mapped.pData + y*mapped.RowPitch + x*4;
            if (abs((int)pixel[0]-(int)expected_b)>1 || pixel[1] || abs((int)pixel[2]-(int)expected_r)>1 || pixel[3]!=255) ++mismatches;
        }
        ID3D11DeviceContext_Unmap(context, (ID3D11Resource *)staging, 0);
        printf("case %u: %u mismatched pixels\n", test, mismatches);
        if (mismatches) ++failures;
    }
    ID2D1PathGeometry_Release(overlapping); ID2D1PathGeometry_Release(inner); ID2D1PathGeometry_Release(disjoint); ID2D1SolidColorBrush_Release(brush);
    ID2D1RenderTarget_Release(rt); ID2D1Factory_Release(factory); IDXGISurface_Release(surface); ID3D11Texture2D_Release(staging); ID3D11Texture2D_Release(texture); ID3D11DeviceContext_Release(context); ID3D11Device_Release(device);
    return failures ? 1 : 0;
}
