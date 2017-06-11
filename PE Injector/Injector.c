#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "WorkWithHex.h"

#define EXP 2
#define MAXPATHLEN 256

void Alignment(unsigned long *Data, unsigned long Alignment) {
    if ((*Data) % Alignment) {
        unsigned long temp = (*Data) / Alignment;
        temp = (*Data) - (temp * Alignment);
        (*Data) += Alignment - temp;
    }
}

void WriteData(LPBYTE pBase, struct output *input, unsigned long *WritePoint) {
    unsigned int j = 0;

    for (j = 0; j < input->length; j++)
        sprintf(&pBase[(*WritePoint)++], "%c", input->digit[j]);

    while (j < 4) {
        sprintf(&pBase[(*WritePoint)++], "%c", 0);
        j++;
    }
}

void SetSectionFlags(IMAGE_SECTION_HEADER* pSectHeader) {
    pSectHeader->Characteristics = pSectHeader->Characteristics | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ;
}

void InfectionSectionFlags(IMAGE_SECTION_HEADER* pSectHeader) {
    pSectHeader->Characteristics = 0x0A0000020;
}

void CompleteSection(IMAGE_NT_HEADERS *pHeader, IMAGE_SECTION_HEADER *pLastSectHeader,
    LPBYTE pBase, TCHAR *FileName) {

    IMAGE_SECTION_HEADER* pFirstSectHeader = IMAGE_FIRST_SECTION(pHeader);

    /*Cначала полу  чаем размер выравнивания (в байтах) для секций в файле и в памяти*/
    unsigned long FileAlignment = pHeader->OptionalHeader.FileAlignment;
    unsigned long SectionAlignment = pHeader->OptionalHeader.SectionAlignment;

    /*Получаем смещение в файле до первой и последней секции*/
    unsigned long StartPoint = pFirstSectHeader->VirtualAddress;
    unsigned long FinishPoint = pLastSectHeader->VirtualAddress;

    /*RVA Точки запуска программы*/
    unsigned long RVAAddress = pHeader->OptionalHeader.AddressOfEntryPoint;

    printf("\nInit Address...\nStart Address = %d\nFinish Adress = %d\nNumber of Characters = %d\n",
        StartPoint, FinishPoint, FinishPoint - StartPoint);

    printf("Start Writing Encoder Data ");
    unsigned long WritePoint = pLastSectHeader->Misc.VirtualSize + pLastSectHeader->PointerToRawData;

    /*Получаю адреса в формате Little Endian*/
    struct output *StartInHex = GetASMValue(StartPoint);
    struct output *NumberOfData = GetASMValue(20);
    struct output *EntryPoint = GetASMValue(RVAAddress - pFirstSectHeader->VirtualAddress);
    struct output *Offset = GetASMValue(pLastSectHeader->VirtualAddress - pFirstSectHeader->VirtualAddress);
    struct output *Offset2 = GetASMValue(pLastSectHeader->Misc.VirtualSize);

    /*Массив опкодов, которые будем проецировать в файл*/
    int data[] = {
        0xE8, 0x00, 0x00, 0x00, 0x00,
        0x81, 0xEA, /*+ 4 байта*/
        0xEB, 0x00,
        0x81, 0xEA, /*+ 4 байта*/
        0x31, 0xF6,
        0x01, 0xD6,
        0x81, 0xC6, /* + 4 байта*/
        0xB9, /* + 4 байта*/
        0x66, 0x8B, 0x3A,
        0x66, 0x83, 0xF7, 0x02,
        0x66, 0x89, 0x3A,
        0x42,
        0xE2, 0xF3,
        0x31, 0xD2,
        0x89, 0xF2,
        0x31, 0xF6,
        0x31, 0xFF,
        0xFF, 0xE2
    };

    int i = 18;

    for (i = 0; i < 42; i++) {
        sprintf(&pBase[WritePoint++], "%c", data[i]);
        
        if (6 == i) 
            WriteData(pBase, Offset2, &WritePoint);

        if (10 == i) 
            WriteData(pBase, Offset, &WritePoint);

        if (16 == i) 
            WriteData(pBase, EntryPoint, &WritePoint);

        if (17 == i) 
            WriteData(pBase, NumberOfData, &WritePoint);
    }

    printf("[ok]\n");

    /*Коректируем флаги в данной секции*/
    printf("Set Flags Value...");
    InfectionSectionFlags(pLastSectHeader);
    printf("[ok]\n");

    /*Адрес точки входа в программу*/
    pHeader->OptionalHeader.AddressOfEntryPoint = pLastSectHeader->VirtualAddress + pLastSectHeader->Misc.VirtualSize;

    /*Данные загрузили - меняем значения о них в заголовке секции*/
    pLastSectHeader->Misc.VirtualSize += 65;

    pLastSectHeader->SizeOfRawData = pLastSectHeader->Misc.VirtualSize;
    Alignment(&pLastSectHeader->SizeOfRawData, FileAlignment);

    /*Размер файла в памяти, включая все заголовки. Должен быть кратен SectionAlignment*/
    pHeader->OptionalHeader.SizeOfImage = pLastSectHeader->VirtualAddress + pLastSectHeader->Misc.VirtualSize;
    Alignment(&pHeader->OptionalHeader.SizeOfImage, SectionAlignment);

    /*Количество байт кода в программе*/
    pHeader->OptionalHeader.SizeOfCode += 52;
    Alignment(&pHeader->OptionalHeader.SizeOfCode, FileAlignment);

    StartPoint = pFirstSectHeader->PointerToRawData;
    FinishPoint = StartPoint + 20;
    int counter = StartPoint;

    printf("Encrypt...");
    /*Шифруем данные*/
    while (counter != FinishPoint) {
        pBase[counter] = pBase[counter] ^ 2;
        counter++;
    }
    printf("[ok]\n");
}

TCHAR *NewString() {
    char c = 0;
    int length = 0;
    int maxmemory = MAXPATHLEN;
    TCHAR *string = (TCHAR*)calloc(maxmemory, sizeof(TCHAR));

    while ('\n' != (c = getchar())) {
        string[length] = c;
        length++;

        if (length == maxmemory) {
            string = (char*)realloc(string, (EXP * maxmemory) * sizeof(TCHAR));
            maxmemory *= EXP;
        }
    }

    return string;
}

void WorkWithSections(IMAGE_NT_HEADERS *pHeader, LPBYTE pBase, TCHAR *FileName) {
    int i = 0;

    /*Получаем заголовок первой секции*/
    IMAGE_SECTION_HEADER* pSectHeader = IMAGE_FIRST_SECTION(pHeader);

    unsigned long SummaryVirtualAddress = 0;

    for (; i < pHeader->FileHeader.NumberOfSections && (pSectHeader + 1)->Misc.VirtualSize > 0; i++, pSectHeader++) {
        printf("Current Section: %s\n", pSectHeader->Name);

        if (pSectHeader->Misc.VirtualSize < pSectHeader->SizeOfRawData)
            printf("Free Place! %d bytes in %s section\n", pSectHeader->SizeOfRawData - pSectHeader->Misc.VirtualSize, pSectHeader->Name);

        SetSectionFlags(pSectHeader);
        SummaryVirtualAddress += pSectHeader->VirtualAddress;
    }

    printf("Current Section: %s\n===================\n", pSectHeader->Name);
    CompleteSection(pHeader, pSectHeader, pBase, FileName, SummaryVirtualAddress);
}

IMAGE_NT_HEADERS* GetHeader(LPBYTE pBase, TCHAR *FileName) {
    /*Проверка правильности аргументов*/
    if (pBase == NULL)
        return NULL;

    /*Получаем указатель на заголовок DOS*/
    IMAGE_DOS_HEADER* pDosHeader = (IMAGE_DOS_HEADER*)pBase;

    /*Проверяем корректность указателя*/
    if (IsBadReadPtr(pDosHeader, sizeof(IMAGE_DOS_HEADER)))
        return NULL;

    /*Сначала проверяем сигнатуру DOS*/
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return NULL;

    /*Берем указатель на заголовок PE*/
    IMAGE_NT_HEADERS* pHeader = (IMAGE_NT_HEADERS*)(pBase + pDosHeader->e_lfanew);

    /*Проверяем корректность указателя.*/
    if (IsBadReadPtr(pHeader, sizeof(IMAGE_NT_HEADERS)))
        return NULL;

    /*Наконец проверяем сигнатуру PE.*/
    if (pHeader->Signature != IMAGE_NT_SIGNATURE)
        return NULL;

    printf("Check Sections...\n");
    /*Узнаем количество секций в файле и увеличиваем их на 1 (для хранения там дешифратора)*/
    printf("Number of Sections in File: %d\n", pHeader->FileHeader.NumberOfSections);
    printf("Number of Bytes from Top of File to Begining Sections: %d\n", pHeader->OptionalHeader.SizeOfHeaders);
    WorkWithSections(pHeader, pBase, FileName);

    return pHeader;
}

LPBYTE OpenPEFile(LPCTSTR lpszFileName) {
    /*Открываем файл (получаем его дескриптор) в несовместном режиме и
    разрешаем его читать и записывать в него*/
    HANDLE hFile = CreateFile(lpszFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    /*Проверка на корректное открытие*/
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Error! File is not opened!\n");
        return NULL;
    }

    /*Создаем проецируемый файл (проекция hFile)*/
    HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
    CloseHandle(hFile);
    LPBYTE pBase = NULL;

    /*Проецируем файл в адрессное пространство вызывающего процесса c
    параметрами разрещающими чтение и запись*/
    if (hMapping != NULL) {
        pBase = (LPBYTE)MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        CloseHandle(hMapping);
    }

    return pBase;
}

int main() {
    printf("Welcome to EXEEncoder!\nPlease Enter Path to Victim: ");
    int a = 1;

    if (a = 0) {
        printf("ad");
    }
    TCHAR *FileName = NewString();
    LPCTSTR File = FileName;

    LPBYTE pBase = OpenPEFile(File);
    GetHeader(pBase, FileName);
}