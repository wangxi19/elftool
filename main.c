#include <elf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <list>
#include <iostream>

class ElfFile
{
public:
    explicit ElfFile(const std::string &str)
    {
        mFPath = str;
    }

    explicit ElfFile(std::string &&str)
    {
        std::swap(mFPath, str);
    }

    ElfFile() = delete;

    ~ElfFile()
    {
        if (mElfHdr)
        {
            delete mElfHdr;
            mElfHdr = nullptr;
        }

        for (auto ptr : mPHdrs)
        {
            delete ptr;
        }
        mPHdrs.clear();

        for (auto ptr : mSHdrs)
        {
            delete ptr;
        }
        mSHdrs.clear();
    }

public:
    bool Init()
    {
        int fd = open(mFPath.c_str(), O_RDONLY);
        if (fd <= 0)
        {
            std::cerr << "Fail to open file '" << mFPath << "'" << std::endl;
            return false;
        }

        mElfHdr = new Elf64_Ehdr;

        auto len = read(fd, mElfHdr, sizeof(Elf64_Ehdr));
        if (len != (ssize_t)sizeof(Elf64_Ehdr))
        {
            close(fd);
            std::cerr << "length less than ehdr" << std::endl;
            return false;
        }

        if (*(uint32_t *)&ELFMAG != *(uint32_t *)mElfHdr)
        {
            close(fd);
            std::cerr << "error of elf magic" << std::endl;
            return false;
        }

        if (mElfHdr->e_ident[EI_CLASS] != ELFCLASS64)
        {
            close(fd);
            std::cerr << "is not 64bit" << std::endl;
            return false;
        }

        if (mElfHdr->e_type < ET_REL || mElfHdr->e_type > ET_CORE)
        {
            close(fd);
            std::cerr << "error of elf type" << std::endl;
            return false;
        }

        if (mElfHdr->e_phentsize && mElfHdr->e_phentsize != sizeof(Elf64_Phdr))
        {
            close(fd);
            std::cerr << "error of phdr size" << std::endl;
            return false;
        }

        if (mElfHdr->e_shentsize && mElfHdr->e_shentsize != sizeof(Elf64_Shdr))
        {
            close(fd);
            std::cerr << "error of shdr size" << std::endl;
            return false;
        }

        // program header
        if (mElfHdr->e_phnum && -1 == lseek(fd, mElfHdr->e_phoff, SEEK_SET))
        {
            close(fd);
            std::cerr << "invalid phoff" << std::endl;
            return false;
        }
        for (auto i = 0; i < mElfHdr->e_phnum; i++)
        {
            auto phdr = new Elf64_Phdr;
            mPHdrs.push_back(phdr);
            auto len = read(fd, phdr, sizeof(Elf64_Phdr));
            if (len != (ssize_t)sizeof(Elf64_Phdr))
            {
                close(fd);
                std::cerr << "truncated phdrs" << std::endl;
                return false;
            }
        }
        // section header
        if (mElfHdr->e_shnum && -1 == lseek(fd, mElfHdr->e_shoff, SEEK_SET))
        {
            close(fd);
            std::cerr << "invalid shoff" << std::endl;
            return false;
        }
        for (auto i = 0; i < mElfHdr->e_shnum; i++)
        {
            auto shdr = new Elf64_Shdr;
            mSHdrs.push_back(shdr);
            auto len = read(fd, shdr, sizeof(Elf64_Shdr));
            if (len != (ssize_t)sizeof(Elf64_Shdr))
            {
                close(fd);
                std::cerr << "truncated shdrs" << std::endl;
                return false;
            }
        }

        close(fd);
        return true;
    }

    void Debug()
    {
        using namespace std;
        cout << "PhdrCunt: " << mPHdrs.size() << endl;
        cout << "ShdrCunt: " << mSHdrs.size() << endl;
    }

private:
    std::string mFPath;
    Elf64_Ehdr *mElfHdr{nullptr};
    std::list<Elf64_Phdr *> mPHdrs;
    std::list<Elf64_Shdr *> mSHdrs;
};

int main(int argc, char **argv)
{
    ElfFile elff(argv[1]);
    if (!elff.Init())
        return 1;
    elff.Debug();
    return 0;
}
