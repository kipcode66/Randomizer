/**	@file seedselector.cpp
 *  @brief Selection tool to select the desired data-GCI
 *
 *	@author AECX
 *	@bug No known bugs.
 */
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cinttypes>

#ifdef DVD
#include "gc_wii/dvd.h"
#elif defined PLATFORM_WII
#include "gc_wii/nand.h"
#else
#include "gc_wii/card.h"
#endif

#include "rando/seedlist.h"
#include "main.h"
#include "rando/data.h"
#include "rando/seed.h"
#include "tools.h"
#include "memory.h"
#include "cxx.h"
#include "gc_wii/OSInterrupt.h"
#include "tp/m_do_printf.h"

#define MIN(x, y) ((x) > (y) ? (y) : (x))

// TODO Move this to libtp_rel
#ifdef PLATFORM_WII
extern "C" const char* nandGetHomeDir();
#endif

namespace mod::rando
{
    SeedList::SeedList()
    {
        using namespace libtp;

        // Store minimal data so we don't have to open the files again later
        // Allocate the memory to the back of the heap to avoid fragmentation
        // Align to uint8_t, as it is the largest variable type used in the MinSeedInfo struct
        MinSeedInfo* minSeedInfoBuffer = new (-sizeof(uint8_t)) MinSeedInfo[SEED_MAX_ENTRIES];

        // Make sure m_numSeeds is set to 0 before checking for seeds
        m_numSeeds = 0;

        // Get a list of all seeds available
#ifdef DVD
        getSeedFiles("/mod/seed", minSeedInfoBuffer);
#elif defined PLATFORM_WII
        char dirBuf[96];
        snprintf(dirBuf, sizeof(dirBuf), "%s", nandGetHomeDir());
        libtp::tp::m_Do_printf::OSReport("nand home dir + seed: \"%s\"\n", dirBuf);

        getSeedFiles(dirBuf, minSeedInfoBuffer);
#else
        // The memory card should already be mounted
        getSeedFiles(CARD_SLOT_A, minSeedInfoBuffer);
#endif
        uint32_t numSeeds = m_numSeeds;
        if (numSeeds > 0)
        {
            // Align to uint8_t, as it is the largest variable type used in the MinSeedInfo struct
            m_minSeedInfo = new (sizeof(uint8_t)) MinSeedInfo[numSeeds];

            // Copy all of the seeds' data to the main buffer
            memcpy(m_minSeedInfo, minSeedInfoBuffer, sizeof(MinSeedInfo) * numSeeds);
        }

        getConsole() << static_cast<int32_t>(numSeeds) << " seed(s) available.\n";

        // Cleanup
        delete[] minSeedInfoBuffer;
    }

    SeedList::~SeedList()
    {
        delete[] m_minSeedInfo;
    }

#ifdef DVD
    void SeedList::getSeedFiles(const char* seedDirectory, MinSeedInfo* minSeedInfoBuffer)
    {
        using namespace libtp::gc_wii::dvd;

        DVDDirectory dir;
        DVDDirectoryEntry entry;
        char filePath[96];
#elif defined PLATFORM_WII
    void SeedList::getSeedFiles(const char* seedDirectory, MinSeedInfo* minSeedInfoBuffer)
    {
        using namespace libtp::gc_wii::nand;

        char filePath[96];
#else
    // This function assumes that the memory card is already mounted
    void SeedList::getSeedFiles(int32_t chan, MinSeedInfo* minSeedInfoBuffer)
    {
        using namespace libtp::gc_wii::card;

        CARDStat stat;
        int32_t result;
#endif
        Header header;
        const char* currentFileName;

        // Starting index
        uint32_t index = 0;

        // Interrupts are required to be enabled for CARD/DVD functions to work properly
        bool enable = libtp::gc_wii::os_interrupt::OSEnableInterrupts();

#ifdef DVD
        // Try to open the directory that has the seeds
        if (!DVDOpenDir(seedDirectory, &dir))
        {
            return;
        }
#endif

#ifdef PLATFORM_WII
        // Get the total number of files/folders in the folder
        uint32_t numFiles;
        if (NANDReadDir(seedDirectory, nullptr, &numFiles) != NAND_RESULT_READY)
        {
            return;
        }

        numFiles = MIN(numFiles, SEED_MAX_ENTRIES);

        // Allocate memory for the list
        // Allocate the memory to the back of the heap to avoid fragmentation
        char* fileList = new (-0x20) char[numFiles * (NAND_FILENAME_MAX + 1)];

        // Get the list of files/folders
        if (NANDReadDir(seedDirectory, fileList, &numFiles) != NAND_RESULT_READY)
        {
            return;
        }

        libtp::tp::m_Do_printf::OSReport("nand seed file(s) count: %d\n", numFiles);

        uint8_t cursor = 0;
        for (uint32_t i = 0; i < numFiles; ++i)
        {
#else
        // Loop through all possible files
        for (int32_t i = 0; i < SEED_MAX_ENTRIES; i++)
        {
#endif
#ifdef DVD
            // Loop through the files in the directory
            // DVDReadDir will return false once all files have been looped through
            if (!DVDReadDir(&dir, &entry))
            {
                break;
            }

            // Currently only supporting a single folder
            if (entry.is_directory)
            {
                // Entry is a directory
                continue;
            }

            // Try to open the file and get the header data
            // entry does not contain the full file path
            currentFileName = entry.fileName;
            snprintf(filePath, sizeof(filePath), "/mod/seed/%s", currentFileName);

            // Try to open the file and get the header data
            if (DVD_STATE_END != libtp::tools::readFile(filePath, sizeof(header), 0, &header))
            {
#elif defined PLATFORM_WII
            // Loop through the files in the directory
            size_t filenameLength = strlen(fileList + cursor);

            snprintf(filePath, sizeof(filePath), "%s", fileList + cursor);
            cursor += filenameLength + 1;
            currentFileName = filePath;

            if (NAND_RESULT_READY != libtp::tools::readNAND(filePath, sizeof(header), 0, &header))
            {
#else
            // Try to get the status of an arbitrary file slot
            result = CARDGetStatus(chan, i, &stat);
            if (result != CARD_RESULT_READY)
            {
                // No file is in the current slot
                continue;
            }

            // If the file is for the vanilla game save or the rando's REL save, then skip it
            currentFileName = stat.fileName;
            if ((strcmp(currentFileName, "gczelda2") == 0) || (strcmp(currentFileName, "Custom REL File") == 0))
            {
                continue;
            }

            // Try to open the file and get the header data
            if (CARD_RESULT_READY != libtp::tools::readGCIMounted(chan, currentFileName, sizeof(header), 0, &header, true))
            {
#endif
                // The file could not be opened
                continue;
            }

            // Make sure the seed version is supported
            const uint32_t currentSeedVersion = header.version;

            // The major and minor seed versions use 2 bytes each, so merge both into a single 4 byte variable
            constexpr uint32_t supportedSeedVersion = (_VERSION_MAJOR << 16) | _VERSION_MINOR;

            if (currentSeedVersion == supportedSeedVersion)
            {
                MinSeedInfo* currentMinSeedInfo = &minSeedInfoBuffer[index];

                // Copy the seed version
                currentMinSeedInfo->version = currentSeedVersion;

                // Copy the total number of bytes in the GCI
                currentMinSeedInfo->totalSize = header.totalSize;

                // Copy the file index
                currentMinSeedInfo->fileIndex = static_cast<uint8_t>(index);

                // Copy the filename to the buffer and make sure it is NULL terminated
                char* fileName = currentMinSeedInfo->fileName;
                strncpy(fileName, currentFileName, CARD_FILENAME_MAX);
                fileName[CARD_FILENAME_MAX] = '\0';

                // Done with the current file, so increment the index
                index++;
            }
            else
            {
                getConsole() << "ERROR - UNSUPPORTED SEED: " << currentFileName << "\n";
            }
        }

#ifdef DVD
        // Close the directory that has the seeds
        DVDCloseDir(&dir);
#elif defined(PLATFORM_WII)
        delete fileList;
#endif
        // Restore interrupts
        libtp::gc_wii::os_interrupt::OSRestoreInterrupts(enable);

        // Update m_numSeeds
        m_numSeeds = static_cast<uint8_t>(index);
    }
} // namespace mod::rando