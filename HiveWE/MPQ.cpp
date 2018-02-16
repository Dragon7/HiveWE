#include "stdafx.h"

namespace mpq {
	File::~File() {
		close();
	}

	std::vector<uint8_t> File::read() {
		uint32_t size = SFileGetFileSize(handle, nullptr);
		std::vector<uint8_t> buffer(size);

		unsigned long bytes_read;
		bool success = SFileReadFile(handle, &buffer[0], size, &bytes_read, nullptr);
		if (!success) {
			std::cout << "Failed to read file: " << GetLastError() << std::endl;
		}
		return buffer;
	}

	void File::close() {
		SFileCloseFile(handle);
	}

	// MPQ

	MPQ::MPQ(const fs::path& path, unsigned long flags) {
		open(path, flags);
	}

	MPQ::MPQ(File archive, unsigned long flags) {
		open(archive, flags);
	}

	MPQ::~MPQ() {
		close();
	}

	void MPQ::open(const fs::path& path, unsigned long flags) {
		bool opened = SFileOpenArchive(path.c_str(), 0, flags, &handle);
		if (!opened) {
			std::wcout << "Error opening " << path << " with error:" << GetLastError() << std::endl;
		}
	}
	
	void MPQ::open(File& archive, unsigned long flags) {
		const std::vector<uint8_t> buffer = archive.read();

		std::ofstream output("Data/Temporary/temp.mpq", std::ofstream::binary);
		if (!output) {
			std::cout << "Opening/Creating failed for-: Data/Temporary/temp.mpq" << std::endl;
		}

		output.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(uint8_t));
		output.close();

		open(L"Data/Temporary/temp.mpq", flags);
	}

	File MPQ::file_open(const fs::path& path) {
		File file;
		bool opened = SFileOpenFileEx(handle, path.string().c_str(), 0, &file.handle);
		if (!opened) {
			std::cout << "Error opening file " << path << " with error: " << GetLastError() << std::endl;
		}
		return file;
	}

	bool MPQ::file_exists(const fs::path& path) {
		return SFileHasFile(handle, path.string().c_str());
	}

	void MPQ::close() {
		SFileCloseArchive(handle);
		handle = nullptr;
	}
}