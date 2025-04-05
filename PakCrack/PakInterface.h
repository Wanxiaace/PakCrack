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
				PAK_GEN_FAILED,
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
			{PakError::ErrorType::PAK_GEN_FAILED,"PAK_GEN_FAILED"},
		};

		class PopcapPak;

		class PakFile {
		public:
			std::filesystem::path mFilePath;
			unsigned long long mTimeStamp;
			int mFileSize;
		private:
			friend class PopcapPak;

			const char* mFilePtr;//从pak数据中选取的数据头部，不能delete
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
			PopcapPak();
			PopcapPak(const std::filesystem::path& path);
			~PopcapPak();

			void DumpDecodePak() const;
			void Open(const std::filesystem::path& path);

			/// <summary>
			/// DumpFilesInPAK
			/// </summary>
			/// <param name="path"></param>
			/// <param name="outPath"></param>
			/// <param name="usingTimeStamp">will change the time of new file, it will be "false" for faster</param>
			void DumpFile(const std::filesystem::path& path, const std::filesystem::path& outPath = "",bool usingTimeStamp = false);
			void DumpAllFiles(const std::filesystem::path& outPath = "", bool usingTimeStamp = false);

			void CopyFileBytes(char* dst, const std::filesystem::path&);

			void GenPakFile(const std::filesystem::path& outPath);

			void AddFile(const std::filesystem::path& path,const char* data,int size,unsigned long long timeStamp = 0);
			void RemoveFile(const std::filesystem::path& path);

		private:
			const std::shared_ptr<sgf::PakInterface::PakFile>& GetPakFile(std::filesystem::path path);
			
		};
	}
}
#endif // !__SGF_POPCAP_PAKINTERFACE__
