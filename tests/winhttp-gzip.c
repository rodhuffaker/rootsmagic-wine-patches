#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
int main(void) {
 HINTERNET s,c,r; DWORD flags=3,got=0,code=0,n=sizeof(code); char buf[128]={0}; BOOL ok;
 s=WinHttpOpen(L"RootsMagic Wine local diagnostic",WINHTTP_ACCESS_TYPE_NO_PROXY,NULL,NULL,0);
 c=WinHttpConnect(s,L"127.0.0.1",19717,0);
 r=WinHttpOpenRequest(c,L"GET",L"/probe",NULL,NULL,NULL,0);
 ok=WinHttpSetOption(r,118,&flags,sizeof(flags)); printf("decompression_option=%d error=%lu\n",ok,ok?0:GetLastError());
 ok=WinHttpSendRequest(r,NULL,0,NULL,0,0,0); if(ok)ok=WinHttpReceiveResponse(r,NULL);
 if(!ok){printf("request_error=%lu\n",GetLastError());return 2;}
 WinHttpQueryHeaders(r,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,NULL,&code,&n,NULL);
 ok=WinHttpReadData(r,buf,sizeof(buf)-1,&got);
 printf("status=%lu read_ok=%d bytes=%lu decoded_json=%d gzip_magic=%d\n",code,ok,got,got && buf[0]=='{',got>1&&(unsigned char)buf[0]==0x1f&&(unsigned char)buf[1]==0x8b);
 WinHttpCloseHandle(r);WinHttpCloseHandle(c);WinHttpCloseHandle(s);
 return ok&&got&&buf[0]=='{'?0:1;
}
