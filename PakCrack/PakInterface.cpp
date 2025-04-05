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
		x->mFilePtr = (unsigned char*)&mPakDecodeFileData[fileOffset];
		fileOffset += x->mFileSize;
	}
}

void sgf::PakInterface::PopcapPak::DumpFile(const std::filesystem::path& path, const std::filesystem::path& outPath)
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
}

void sgf::PakInterface::PopcapPak::DumpAllFiles(const std::filesystem::path& outPath)
{
	for (auto& x : mFiles)
	{
		DumpFile(x->mFilePath, outPath / x->mFilePath);
	}
}

void sgf::PakInterface::PopcapPak::CopyFileBytes(char* dst, const std::filesystem::path& path)
{
	auto & f = GetPakFile(path);
	CopyLong(dst, (char*)f->mFilePtr, f->mFileSize);
}

void sgf::PakInterface::PopcapPak::GenPakFile()
{

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
