#include <stdlib.h>
#include <locale.h>  
#include <stdio.h>
#include <windows.h>
#include <tchar.h>


void ErrorMes(LPTSTR lpszFunction) 
{ 
   

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 
    wprintf(L"%s failed with error %d: %s", 
        lpszFunction, dw, lpMsgBuf);     

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);

}


BOOL FindFileByClaster(TCHAR* volume,LONGLONG cluster){

    HANDLE hDevice = CreateFile(volume,      // drive to open
        GENERIC_READ ,                       // access to the drive
        FILE_SHARE_READ | FILE_SHARE_WRITE,  // share mode
        NULL,                                // default security attributes
        OPEN_EXISTING,                       // disposition
        FILE_FLAG_BACKUP_SEMANTICS,          // file attributes
        NULL);

    if(hDevice == INVALID_HANDLE_VALUE)
    {   
          ErrorMes(L"CreateFile");
          return FALSE;
    }

    
    LOOKUP_STREAM_FROM_CLUSTER_INPUT input={0};
    input.NumberOfClusters = 1;
    input.Cluster[0].QuadPart = cluster;        

    //buffer
    BYTE output[5000]={};
    DWORD dwRes=0;
    LOOKUP_STREAM_FROM_CLUSTER_OUTPUT result={0};   

    
    BOOL bRes = DeviceIoControl( (HANDLE)       hDevice,    
                 FSCTL_LOOKUP_STREAM_FROM_CLUSTER, 
                 (LPVOID)       &input,         
                 (DWORD)        sizeof(input),     
                 (LPVOID)       output,        
                 (DWORD)        5000,    
                 (LPDWORD)      &dwRes,    
                 NULL );    

    if(bRes == FALSE){      
          ErrorMes(L"DeviceIoControl");
          return FALSE;
    }

    //переносим результат из массива в структуру LOOKUP_STREAM_FROM_CLUSTER_OUTPUT
    memcpy(&result,output,sizeof(LOOKUP_STREAM_FROM_CLUSTER_OUTPUT));

    if(result.NumberOfMatches == 0){
        wprintf( L"Файл не найден\n");
        return FALSE;
    }   

    wprintf( L"Информация о файле\n");

    //переходим к адресу первой структуры LOOKUP_STREAM_FROM_CLUSTER_ENTRY
    BYTE* p = (BYTE*)output + result.Offset;
    LOOKUP_STREAM_FROM_CLUSTER_ENTRY* pentry = (LOOKUP_STREAM_FROM_CLUSTER_ENTRY*)p;    

    
    wprintf( L"Flags: 0x%x ",(UINT)pentry->Flags);

    if((pentry->Flags & LOOKUP_STREAM_FROM_CLUSTER_ENTRY_FLAG_PAGE_FILE) > 0) wprintf(L"(Pagefile)");
    else if((pentry->Flags & LOOKUP_STREAM_FROM_CLUSTER_ENTRY_FLAG_FS_SYSTEM_FILE) > 0)  wprintf(L"(Internal filesystem file)");
    else if((pentry->Flags & LOOKUP_STREAM_FROM_CLUSTER_ENTRY_FLAG_TXF_SYSTEM_FILE) > 0) wprintf(L"(Internal TXF file)");
    else wprintf(L"(Normal file)"); 

    wprintf( L"\nFile: %s\n",pentry->FileName); 
    return TRUE;
}

int _tmain(int argc, _TCHAR* argv[])
{
    setlocale(LC_ALL,"Russian");

    LONGLONG inp=0;
    wprintf( L"Введите номер кластера: \n");
    scanf("%llu",&inp);

    FindFileByClaster(L"\\\\.\\C:",inp);        

    system("PAUSE");
    return 0;
}
