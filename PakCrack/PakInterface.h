#ifndef __SGF_POPCAP_PAKINTERFACE__
#define __SGF_POPCAP_PAKINTERFACE__

#include <fstream>
#include <filesystem>
#include <vector>
#include <unordered_map>

#define PAK_MAGIC_CONSTANT 0xBAC04AC0

namespace sgf {
	namespace PakInterface {
		class PakError : public std::exception {
		public:
			enum ErrorType {
				PAK_FAILED_READING,
				PAK_ABNORMAL_SIZE,
				PAK_UNSUPPORT_VERSION,
				PAK_MAGIC_ERROR,
				PAK_INVALID_FILE,
			};
		public:
			ErrorType mErrorType;

		public:
			PakError(ErrorType type);

		public:
			virtual const char* what() const override;
		};

		static std::unordered_map<PakError::ErrorType, std::string>
			gErrorTextMap =
		{
			{PakError::ErrorType::PAK_FAILED_READING,"PAK_FAILED_READING"},
			{PakError::ErrorType::PAK_ABNORMAL_SIZE,"PAK_ABNORMAL_SIZE"},
			{PakError::ErrorType::PAK_UNSUPPORT_VERSION,"PAK_UNSUPPORT_VERSION"},
			{PakError::ErrorType::PAK_MAGIC_ERROR,"PAK_MAGIC_ERROR"},
			{PakError::ErrorType::PAK_INVALID_FILE,"PAK_INVALID_FILE"},
		};

		class PopcapPak;

		class PakFile {
		public:
			std::filesystem::path mFilePath;
			unsigned long long mTimeStamp;
			int mFileSize;
		private:
			friend class PopcapPak;

			unsigned char* mFilePtr;//从pak数据中选取的数据头部，不能delete
		};

		class PopcapPak {
		private:
			std::ifstream mPakFile;
			std::vector<std::shared_ptr<PakFile>> mFiles;
			std::unordered_map<std::filesystem::path, std::shared_ptr<PakFile>> mFileMaps;

		public:
			
			char* mPakDecodeFileData;
			int mSize;
			unsigned int mMagic;
			unsigned int mVersion;

		public:
			PopcapPak(std::filesystem::path path);
			~PopcapPak();

			void DumpDecodePak() const;

			void DumpFile(std::filesystem::path path, std::filesystem::path outPath = "");
			void DumpAllFiles(std::filesystem::path outPath = "");

			void CopyFileBytes(char* dst, std::filesystem::path path);

		private:
			const std::shared_ptr<sgf::PakInterface::PakFile>& GetPakFile(std::filesystem::path path);
			
		};
	}
}
#endif // !__SGF_POPCAP_PAKINTERFACE__
