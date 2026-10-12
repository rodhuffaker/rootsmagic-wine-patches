#define COBJMACROS
/* Pixel regression: a collinear join must not extend beyond an open path. */
#include <windows.h>
#include <d2d1.h>
#include <d3d11.h>
#include <stdio.h>
#include <stdlib.h>

static void check(HRESULT hr, const char *operation)
{
    if (FAILED(hr)) { fprintf(stderr, "%s failed: %#lx\n", operation, (unsigned long)hr); exit(2); }
}

int main(void)
{
    ID3D11Device *device;
    ID3D11DeviceContext *context;
    ID3D11Texture2D *texture, *staging;
    IDXGISurface *surface;
    ID2D1Factory *factory;
    ID2D1RenderTarget *target;
    ID2D1SolidColorBrush *brush;
    D3D11_TEXTURE2D_DESC desc = {0};
    D2D1_RENDER_TARGET_PROPERTIES props = {0};
    D2D1_POINT_2F start = {2, 16}, points[] = {{6, 16}, {8, 16}};
    int failures = 0;

    check(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            NULL, 0, D3D11_SDK_VERSION, &device, NULL, &context), "D3D11CreateDevice");
    desc.Width = desc.Height = 64;
    desc.MipLevels = desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    check(ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture), "texture");
    desc.BindFlags = 0;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    check(ID3D11Device_CreateTexture2D(device, &desc, NULL, &staging), "staging");
    check(ID3D11Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface), "surface");
    check(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory,
            NULL, (void **)&factory), "factory");
    props.pixelFormat = (D2D1_PIXEL_FORMAT){DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED};
    props.dpiX = props.dpiY = 192;
    check(ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory, surface, &props, &target), "target");
    check(ID2D1RenderTarget_CreateSolidColorBrush(target,
            (&(D2D1_COLOR_F){0, 0, 0, 1}), NULL, &brush), "brush");

    for (unsigned count = 1; count <= 2; ++count)
    {
        ID2D1PathGeometry *path;
        ID2D1GeometrySink *sink;
        D3D11_MAPPED_SUBRESOURCE mapped;
        unsigned char inside, beyond;
        check(ID2D1Factory_CreatePathGeometry(factory, &path), "path");
        check(ID2D1PathGeometry_Open(path, &sink), "sink");
        ID2D1GeometrySink_BeginFigure(sink, start, D2D1_FIGURE_BEGIN_HOLLOW);
        if (count == 1)
            ID2D1GeometrySink_AddLine(sink, points[1]);
        else
            ID2D1GeometrySink_AddLines(sink, points, count);
        ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);
        check(ID2D1GeometrySink_Close(sink), "close sink");
        ID2D1GeometrySink_Release(sink);
        ID2D1RenderTarget_BeginDraw(target);
        ID2D1RenderTarget_Clear(target, (&(D2D1_COLOR_F){0.5f, 0.5f, 0.5f, 1}));
        ID2D1RenderTarget_DrawGeometry(target, (ID2D1Geometry *)path,
                (ID2D1Brush *)brush, 1.5f, NULL);
        check(ID2D1RenderTarget_EndDraw(target, NULL, NULL), "EndDraw");
        ID3D11DeviceContext_CopyResource(context, (ID3D11Resource *)staging, (ID3D11Resource *)texture);
        check(ID3D11DeviceContext_Map(context, (ID3D11Resource *)staging, 0,
                D3D11_MAP_READ, 0, &mapped), "readback");
        inside = ((const unsigned char *)mapped.pData)[32 * mapped.RowPitch + 10 * 4];
        beyond = ((const unsigned char *)mapped.pData)[32 * mapped.RowPitch + 50 * 4];
        ID3D11DeviceContext_Unmap(context, (ID3D11Resource *)staging, 0);
        printf("%s: inside=%u beyond=%u (expected 0,128)\n",
                count == 1 ? "single segment" : "collinear join", inside, beyond);
        if (inside != 0 || beyond != 128) ++failures;
        ID2D1PathGeometry_Release(path);
    }
    ID2D1SolidColorBrush_Release(brush);
    ID2D1RenderTarget_Release(target);
    ID2D1Factory_Release(factory);
    IDXGISurface_Release(surface);
    ID3D11Texture2D_Release(staging);
    ID3D11Texture2D_Release(texture);
    ID3D11DeviceContext_Release(context);
    ID3D11Device_Release(device);
    return failures ? 1 : 0;
}
