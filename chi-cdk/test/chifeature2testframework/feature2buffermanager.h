////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  feature2buffermanager.h
/// @brief Declarations for feature2 buffer manager.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef FEATURE2BUFFERMANAGER_H
#define FEATURE2BUFFERMANAGER_H

#include "genericbuffermanager.h"
#include "chimodule.h"

class Feature2BufferManager : public GenericBufferManager
{

public:

    Feature2BufferManager()
    {
        m_pChiModule = ChiModule::GetInstance();
    }
    ~Feature2BufferManager() {};

    int GetStatusFromBuffer(GenericBuffer* pBuffer);

private:

    ChiModule* m_pChiModule;

    int GenerateOutputBuffer(GenericBuffer* pRequiredBuffer, GenericStream* pOutputStream);
    int GenerateInputBuffer(GenericBuffer* pRequiredBuffer, GenericStream* pInputStream, const char* filename);
    int SaveImageToFile(GenericBuffer* pResultbuffer, std::string filename);

    /* Buffer Allocation Helpers */
    GenericBuffer* CreateGenericBuffer();
    void DestroyGenericBuffer(GenericBuffer* pBuffer);

    /* Fence Helpers */
    CHIFENCEINFO CreateFence(bool isValid = true);
    void DestroyFence(CHIFENCEINFO fenceInfo);

    /* Stream Getters */
    int GetTypeFromStream(GenericStream* pStream);
    uint32_t GetMaxBuffersFromStream(GenericStream* pStream);

    /* Buffer Getters */
    void* GetNativeBufferFromGeneric(GenericBuffer* pBuffer);
    NativeChiBufferHandle GetHandleFromBuffer(GenericBuffer* pBuffer);
    int GetReleaseFenceFromBuffer(GenericBuffer* pBuffer);
    void SetAcquireFenceInBuffer(GenericBuffer* pBuffer, int setVal);
    uint32_t GetMaxBuffersFromBuffer(GenericBuffer* pBuffer);

    /// Do not allow the copy constructor or assignment operator
    Feature2BufferManager(const GenericBufferManager&) = delete;
    Feature2BufferManager& operator= (const GenericBufferManager&) = delete;
};

#endif // FEATURE2BUFFERMANAGER_H
