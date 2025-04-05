#include "PakInterface.h"
#include <iostream>
//#include <Windows.h>

static void CopyLong(void* buf,char* sorce,int length,int pos = 0) {
	memcpy(buf, &sorce[pos], length);
}

sgf::PakInterface::PopcapPak::PopcapPak():
	mMagic(PAK_MAGIC_CONSTANT),
	mPakDecodeFileData(nullptr),
	mSize(-1),
	mVersion(0)
{

}

sgf::PakInterface::PopcapPak::PopcapPak(const std::filesystem::path& path)
{
	Open(path);
}

sgf::PakInterface::PopcapPak::~PopcapPak()
{
	delete[] mPakDecodeFileData;
}

void sgf::PakInterface::PopcapPak::DumpDecodePak() const
{
	std::ofstream out;
	out.open("dump.pak",std::ios::binary);
	out.write(mPakDecodeFileData,mSize);
	out.close();
}

void sgf::PakInterface::PopcapPak::Open(const std::filesystem::path& path)
{
	mPakFile.open(path, std::ios::binary);
	if (!mPakFile.is_open()) {
		throw PakError(PakError::ErrorType::PAK_FAILED_READING);
	}
	mPakFile.seekg(0, std::ios::end);
	mSize = mPakFile.tellg();
	if (mSize < 16)
		throw PakError(PakError::ErrorType::PAK_ABNORMAL_SIZE);
	mPakFile.seekg(0, std::ios::beg);

	mPakDecodeFileData = new char[mSize];

	mPakFile.read(mPakDecodeFileData, mSize);
	mPakFile.close();

	for (size_t i = 0; i < mSize; i++)
	{
		mPakDecodeFileData[i] ^= 0xF7;
	}

	mMagic = *(int*)&mPakDecodeFileData[0];

	if (!(mMagic == PAK_MAGIC_CONSTANT))
		throw PakError(PakError::ErrorType::PAK_MAGIC_ERROR);

	mVersion = (unsigned int)mPakDecodeFileData[4];

	if (mVersion > 0)
		throw PakError(PakError::ErrorType::PAK_UNSUPPORT_VERSION);

	int curPos = 8;//游标

	for (;;) {
		unsigned char flag = (unsigned char)mPakDecodeFileData[curPos];
		curPos++;
		if (flag == 0x80/*终止标志符*/)
			break;

		mFiles.push_back(std::make_shared<PakFile>());

		std::shared_ptr<PakFile>& file = mFiles.back();


		char nameWidth = mPakDecodeFileData[curPos];
		curPos++;

		char name[256] = { 0 };
		CopyLong(name, mPakDecodeFileData, nameWidth, curPos);
		file->mFilePath = name;
		curPos += nameWidth;

		mFileMaps[file->mFilePath] = file;

		file->mFileSize = *(int*)&mPakDecodeFileData[curPos];

		curPos += 4;

		file->mTimeStamp = *(unsigned long long*) & mPakDecodeFileData[curPos];
		curPos += 8;

	}

	int fileOffset = curPos;

	for (auto& x : mFiles)
	{
		x->mFilePtr = &mPakDecodeFileData[fileOffset];
		fileOffset += x->mFileSize;
	}
}

#ifdef _WIN32
#include <Windows.h>
#endif

void sgf::PakInterface::PopcapPak::DumpFile(const std::filesystem::path& path, const std::filesystem::path& outPath, bool usingTimeStamp)
{
	auto & f = GetPakFile(path);

	std::ofstream ofs;
	if (outPath.empty())
	{
		if(!f->mFilePath.parent_path().empty())
			std::filesystem::create_directories(f->mFilePath.parent_path());
		ofs.open(f->mFilePath, std::ios::binary);
	}
	else
	{
		if (!outPath.parent_path().empty())
			std::filesystem::create_directories(outPath.parent_path());
		ofs.open(outPath, std::ios::binary);
	}

	ofs.write((char*)f->mFilePtr, f->mFileSize);
	ofs.close();

	if (usingTimeStamp) {
#ifdef _WIN32
		HANDLE hFile = CreateFile(outPath.native().c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile == INVALID_HANDLE_VALUE || !SetFileTime(hFile, (FILETIME*)&f->mTimeStamp, (FILETIME*)&f->mTimeStamp, (FILETIME*)&f->mTimeStamp)) {
			std::cerr << "Failed to set file time. Error: " << GetLastError() << std::endl;
		}

		CloseHandle(hFile);
#endif
	}
}

void sgf::PakInterface::PopcapPak::DumpAllFiles(const std::filesystem::path& outPath,bool usingTimeStamp)
{
	for (auto& x : mFiles)
	{
		DumpFile(x->mFilePath, outPath / x->mFilePath, usingTimeStamp);
	}
}

void sgf::PakInterface::PopcapPak::CopyFileBytes(char* dst, const std::filesystem::path& path)
{
	auto & f = GetPakFile(path);
	CopyLong(dst, (char*)f->mFilePtr, f->mFileSize);
}

void sgf::PakInterface::PopcapPak::GenPakFile(const std::filesystem::path& outPath)
{
	std::ofstream ofs;
	ofs.open("buf.pak", std::ios::binary);
	if(!ofs.is_open())
		throw PakError(PakError::ErrorType::PAK_GEN_FAILED);
	int magicEncode = mMagic;
	ofs.write((char*) & magicEncode, 4);
	ofs.write((char*) & mVersion, 4);

	for (size_t i = 0; i < mFiles.size(); i++)
	{
		auto pathStr = mFiles[i]->mFilePath.generic_string();
		std::replace(pathStr.begin(), pathStr.end(), '/', '\\');

		ofs.put(0);
		ofs.put(pathStr.size());
		ofs.write(pathStr.c_str(), pathStr.size());
		ofs.write((char*) & mFiles[i]->mFileSize, 4);
		ofs.write((char*) & mFiles[i]->mTimeStamp, 8);
	}
	ofs.put(0x80);

	for (auto& x : mFiles)
	{
		ofs.write((char*)x->mFilePtr, x->mFileSize);
	}

	ofs.close();

	std::ifstream ifs;
	ifs.open("buf.pak", std::ios::binary);
	if (!ifs.is_open())
		throw PakError(PakError::ErrorType::PAK_GEN_FAILED);
	ifs.seekg(0,std::ios::end);
	int size = ifs.tellg();
	ifs.seekg(0,std::ios::beg);

	char* data = new char[size];
	ifs.read(data,size);
	ifs.close();

	for (int i = 0; i < size; i++)
	{
		data[i] ^= 0xF7;
	}

	ofs.open(outPath, std::ios::binary);
	if (!ofs.is_open())
		throw PakError(PakError::ErrorType::PAK_GEN_FAILED);
	ofs.write(data, size);

	ofs.close();
	delete[] data;
}

void sgf::PakInterface::PopcapPak::AddFile(const std::filesystem::path& path, const char* data, int size, unsigned long long timeStamp)
{
	mFiles.push_back(std::make_shared<PakFile>());
	auto& f = mFiles.back();
	f->mFilePath = path;
	f->mFilePtr = data;
	f->mFileSize = size;
	f->mTimeStamp = timeStamp;
	mFileMaps[path] = f;
}

void sgf::PakInterface::PopcapPak::RemoveFile(const std::filesystem::path& path)
{
	if (mFileMaps.erase(path)) {
		for (auto x = mFiles.begin(); x < mFiles.end(); x++)
		{
			if ((*x)->mFilePath == path)
			{
				mFiles.erase(x);//erase会导致迭代器的无效，但是直接break没问题
				break;
			}
		}
	}
	else
		throw PakError(PakError::ErrorType::PAK_INVALID_FILE);
}

const std::shared_ptr<sgf::PakInterface::PakFile>& sgf::PakInterface::PopcapPak::GetPakFile(std::filesystem::path path)
{
	try {
		return mFileMaps.at(path);
	}
	catch (std::out_of_range& e) {
		throw PakError(PakError::ErrorType::PAK_INVALID_FILE);
	}
}

sgf::PakInterface::PakError::PakError(ErrorType type):
	mErrorType(type)
{

}

const char* sgf::PakInterface::PakError::what() const
{
	return gErrorTextMap[mErrorType].c_str();
}
