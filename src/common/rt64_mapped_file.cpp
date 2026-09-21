//
// RT64
//

#include "rt64_mapped_file.h"

#if !defined(_WIN32)
#   include <cerrno>
#   include <cstdlib>
#   include <cstring>
#   include <cstdio>
#   if !defined(__SWITCH__)
#      include <fcntl.h>
#      include <unistd.h>
#   endif
#endif

namespace RT64 {
    MappedFile::MappedFile() {
        // Default constructor.
    }

    MappedFile::MappedFile(const std::filesystem::path &path) {
        open(path);
    }

    MappedFile::~MappedFile() {
#   if defined(_WIN32)
        if (fileView != nullptr) {
            UnmapViewOfFile(fileView);
        }
        
        if (fileMappingHandle != nullptr) {
            CloseHandle(fileMappingHandle);
        }

        if (fileHandle != nullptr) {
            CloseHandle(fileHandle);
        }
#   elif defined(__SWITCH__)
        free(fileView);
#   else
        if (fileView != MAP_FAILED) {
            munmap(fileView, fileSize);
        }

        if (fileHandle != -1) {
            close(fileHandle);
        }
#   endif
    }

    bool MappedFile::open(const std::filesystem::path &path) {
#   if defined(_WIN32)
        fileHandle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (fileHandle == INVALID_HANDLE_VALUE) {
            std::string pathStr = path.u8string();
            fprintf(stderr, "CreateFileW for %s failed with error %lu.\n", pathStr.c_str(), GetLastError());
            fileHandle = nullptr;
            return false;
        }

        if (!GetFileSizeEx(fileHandle, &fileSize)) {
            fprintf(stderr, "GetFileSizeEx failed with error %lu.\n", GetLastError());
            CloseHandle(fileHandle);
            fileHandle = nullptr;
            return false;
        }

        fileMappingHandle = CreateFileMappingW(fileHandle, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (fileMappingHandle == nullptr) {
            fprintf(stderr, "CreateFileMappingW failed with error %lu.\n", GetLastError());
            CloseHandle(fileHandle);
            fileHandle = nullptr;
            return false;
        }

        fileView = MapViewOfFile(fileMappingHandle, FILE_MAP_READ, 0, 0, 0);
        if (fileView == nullptr) {
            fprintf(stderr, "MapViewOfFile failed with error %lu.\n", GetLastError());
            CloseHandle(fileMappingHandle);
            CloseHandle(fileHandle);
            fileMappingHandle = nullptr;
            fileHandle = nullptr;
            return false;
        }

        return true;
#   elif defined(__SWITCH__)
        FILE *file = fopen(path.c_str(), "rb");
        if (file == nullptr) {
            fprintf(stderr, "fopen for %s failed with error %s.\n", path.c_str(), strerror(errno));
            return false;
        }

        fseek(file, 0, SEEK_END);
        const long fileEnd = ftell(file);
        fseek(file, 0, SEEK_SET);
        if (fileEnd <= 0) {
            fprintf(stderr, "ftell for %s failed with error %s.\n", path.c_str(), strerror(errno));
            fclose(file);
            return false;
        }

        fileView = malloc(size_t(fileEnd));
        if (fileView == nullptr) {
            fprintf(stderr, "Unable to allocate %ld bytes for %s.\n", fileEnd, path.c_str());
            fclose(file);
            return false;
        }

        fileSize = fread(fileView, 1, size_t(fileEnd), file);
        fclose(file);

        if (fileSize != size_t(fileEnd)) {
            fprintf(stderr, "fread for %s read %zu of %ld bytes.\n", path.c_str(), fileSize, fileEnd);
            free(fileView);
            fileView = nullptr;
            fileSize = 0;
            return false;
        }

        return true;
#   else
        fileHandle = ::open(path.c_str(), O_RDONLY);
        if (fileHandle == -1) {
            fprintf(stderr, "open for %s failed with error %s.\n", path.c_str(), strerror(errno));
            return false;
        }

        fileSize = lseek(fileHandle, 0, SEEK_END);
        if (fileSize == (off_t)(-1)) {
            fprintf(stderr, "lseek failed with error %s.\n", strerror(errno));
            close(fileHandle);
            fileHandle = -1;
            return false;
        }

        fileView = mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fileHandle, 0);
        if (fileView == MAP_FAILED) {
            fprintf(stderr, "mmap failed with error %s.\n", strerror(errno));
            close(fileHandle);
            fileHandle = -1;
            return false;
        }

        return true;
#   endif
    }

    bool MappedFile::isOpen() const {
#   if defined(_WIN32) || defined(__SWITCH__)
        return (fileView != nullptr);
#   else
        return (fileView != MAP_FAILED);
#   endif
    }

    uint8_t *MappedFile::data() const {
        return reinterpret_cast<uint8_t *>(fileView);
    }

    size_t MappedFile::size() const {
#   if defined(_WIN32)
        return fileSize.QuadPart;
#   else
        return static_cast<size_t>(fileSize);
#   endif
    }
};
