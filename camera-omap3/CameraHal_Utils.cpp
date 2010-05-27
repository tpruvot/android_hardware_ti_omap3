/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/**
* @file CameraHal_Utils.cpp
*
* This file maps the Camera Hardware Interface to V4L2.
*
*/

#define LOG_TAG "CameraHalUtils"

#include "CameraHal.h"
#include "zoom_step.inc"

#define ASPECT_RATIO_FLAG_KEEP_ASPECT_RATIO     (1<<0)  // By default is enabled
#define ASPECT_RATIO_FLAG_SHRINK_WHOLE_SRC      (1<<1)
#define ASPECT_RATIO_FLAG_CROP_BY_PHYSICAL      (1<<2)
#define ASPECT_RATIO_FLAG_CROP_BY_SOURCE        (1<<3)
#define ASPECT_RATIO_FLAG_CROP_BY_DESTINATION   (1<<4)

#define ASPECT_RATIO_FLAG_CROP_BY_ALL           (ASPECT_RATIO_FLAG_CROP_BY_PHYSICAL| \
                                                ASPECT_RATIO_FLAG_CROP_BY_SOURCE| \
                                                ASPECT_RATIO_FLAG_CROP_BY_DESTINATION)

namespace android {

#ifdef FW3A
int CameraHal::FW3A_Create()
{
    int err = 0;

    LOG_FUNCTION_NAME

    fobj = (lib3atest_obj*)malloc(sizeof(*fobj));
    memset(fobj, 0 , sizeof(*fobj));
    if (!fobj) {
        LOGE("cannot alloc fobj");
        goto exit;
    }

    ancillary_buffer = (uint8_t *) malloc(ancillary_len);
    if (!ancillary_buffer) {
        LOGE("cannot alloc ancillary_buffer");
        goto exit;
    }
    memset(ancillary_buffer, 0, ancillary_len);

    LOGE("dlopen MMS 3a Library Enter");
  	fobj->lib.lib_handle = dlopen(LIB3AFW, RTLD_LAZY);
  	LOGE("dlopen MMS 3a Library Exit");
    if (NULL == fobj->lib.lib_handle) {
		LOGE("A dynamic linking error occurred: (%s)", dlerror());
        LOGE("ERROR - at dlopen for %s", LIB3AFW);
        goto exit;
    }

    /* get Create2A symbol */
    fobj->lib.Create2A = (int (*)(Camera2AInterface**)) dlsym(fobj->lib.lib_handle, "Create2A");
    if (NULL == fobj->lib.Create2A) {
        LOGE("ERROR - can't get dlsym \"Create2A\"");
        goto exit;
    }
 
    /* get Destroy2A symbol */
    fobj->lib.Destroy2A = (int (*)(Camera2AInterface**)) dlsym(fobj->lib.lib_handle, "Destroy2A");
    if (NULL == fobj->lib.Destroy2A) {
        LOGE("ERROR - can't get dlsym \"Destroy2A\"");
        goto exit;
    }

    /* get Init2A symbol */
    LOGE("dlsym MMS 3a Init2A Enter");
    fobj->lib.Init2A = (int (*)(Camera2AInterface*, int, uint8)) dlsym(fobj->lib.lib_handle, "Init2A");
    LOGE("dlsym MMS 3a Init2A Exit");
    if (NULL == fobj->lib.Init2A) {
        LOGE("ERROR - can't get dlsym \"Init2A\"");
        goto exit;
    }

    /* get Release2A symbol */
    LOGE("dlsym MMS 3a Release2A Enter");
    fobj->lib.Release2A = (int (*)(Camera2AInterface*)) dlsym(fobj->lib.lib_handle, "Release2A");
    LOGE("dlsym MMS 3a Release2A Exit");
    if (NULL == fobj->lib.Release2A) {
       LOGE( "ERROR - can't get dlsym \"Release2A\"");
       goto exit;
    }

    fobj->cam_dev = 0;
    /* Create 2A framework */
    err = fobj->lib.Create2A(&fobj->cam_iface_2a);
    if (err < 0) {
        LOGE("Can't Create2A");
        goto exit;
    }

    LOGD("FW3A Create - %d   fobj=%p", err, fobj);

    LOG_FUNCTION_NAME_EXIT

    return 0;

exit:
    LOGD("Can't create 3A FW");
    return -1;
}

int CameraHal::FW3A_Init()
{
    int err = 0;

    LOG_FUNCTION_NAME

    err = fobj->lib.Init2A(fobj->cam_iface_2a, fobj->cam_dev, 0);
    if (err < 0) {
        LOGE("Can't Init2A, will try to release first");
        err = fobj->lib.Release2A(fobj->cam_iface_2a);
        if (!err) {
            err = fobj->lib.Init2A(fobj->cam_iface_2a, fobj->cam_dev, 0);
            if (err < 0) {
                LOGE("Can't Init2A");

                err = fobj->lib.Destroy2A(&fobj->cam_iface_2a);
                goto exit;
            }
        }
    }

    LOG_FUNCTION_NAME_EXIT

    return 0;

exit:

    return -1;
}

int CameraHal::FW3A_Release()
{
    int ret;

    LOG_FUNCTION_NAME

    ret = fobj->lib.Release2A(fobj->cam_iface_2a);
    if (ret < 0) {
        LOGE("Cannot Release2A");
        goto exit;
    } else {
        LOGD("2A released");
    }

    LOG_FUNCTION_NAME_EXIT

    return 0;

exit:

    return -1;
}

int CameraHal::FW3A_Destroy()
{
    int ret;

    LOG_FUNCTION_NAME

    ret = fobj->lib.Destroy2A(&fobj->cam_iface_2a);
    if (ret < 0) {
        LOGE("Cannot Destroy2A");
    } else {
        LOGD("2A destroyed");
    }

   	dlclose(fobj->lib.lib_handle);
  	free(fobj);
  	free(ancillary_buffer);
    fobj = NULL;

    LOG_FUNCTION_NAME_EXIT

    return 0;
}

int CameraHal::FW3A_Start()
{
    int ret;

    LOG_FUNCTION_NAME

    if (isStart_FW3A!=0) {
        LOGE("3A FW is already started");
        return -1;
    }
    //Start 3AFW
    ret = fobj->cam_iface_2a->Start2A(fobj->cam_iface_2a->pPrivateHandle);
    if (0 > ret) {
        LOGE("Cannot Start 2A");
        return -1;
    } else {
        LOGE("3A FW Start - success");
    }

    LOG_FUNCTION_NAME_EXIT

    isStart_FW3A = 1;
    return 0;
}

int CameraHal::FW3A_Stop()
{
    int ret;

#ifdef DEBUG_LOG

    LOG_FUNCTION_NAME

#endif

    if (isStart_FW3A==0) {
        LOGE("3A FW is already stopped");
        return -1;
    }

    //Stop 3AFW
    ret = fobj->cam_iface_2a->Stop2A(fobj->cam_iface_2a->pPrivateHandle);
    if (0 > ret) {
        LOGE("Cannot Stop 3A");
        return -1;
    }

#ifdef DEBUG_LOG
     else {
        LOGE("3A FW Stop - success");
    }
#endif

    isStart_FW3A = 0;

#ifdef DEBUG_LOG
    LOG_FUNCTION_NAME_EXIT
#endif

    return 0;
}

int CameraHal::FW3A_Start_CAF()
{
    int ret;
    
    LOG_FUNCTION_NAME

    if (isStart_FW3A_CAF!=0) {
        LOGE("3A FW CAF is already started");
        return -1;
    }

    if (fobj->settings_2a.af.focus_mode != FOCUS_MODE_AF_CONTINUOUS_NORMAL) {
      FW3A_GetSettings();
      fobj->settings_2a.af.focus_mode = FOCUS_MODE_AF_CONTINUOUS_NORMAL;
      ret = FW3A_SetSettings();
      if (0 > ret) {
          LOGE("Cannot Start CAF");
          return -1;
      } else {
          LOGE("3A FW CAF Start - success");
      }
    }
    if (ret == 0) {
      ret = fobj->cam_iface_2a->StartAF(fobj->cam_iface_2a->pPrivateHandle);
    }

    isStart_FW3A_CAF = 1;

    LOG_FUNCTION_NAME_EXIT

    return 0;
}

int CameraHal::FW3A_Stop_CAF()
{
    int ret, prev = isStart_FW3A_CAF;

#ifdef DEBUG_LOG

    LOG_FUNCTION_NAME

#endif

    if (isStart_FW3A_CAF==0) {
        LOGE("3A FW CAF is already stopped");
        return prev;
    }

    ret = fobj->cam_iface_2a->StopAF(fobj->cam_iface_2a->pPrivateHandle);
    if (0 > ret) {
        LOGE("Cannot Stop CAF");
        return -1;
    }

#ifdef DEBUG_LOG
     else {
        LOGE("3A FW CAF Start - success");
    }
#endif

    isStart_FW3A_CAF = 0;

#ifdef DEBUG_LOG
    LOG_FUNCTION_NAME_EXIT
#endif

    return prev;
}

//TODO: Add mode argument
int CameraHal::FW3A_Start_AF()
{
    int ret = 0;
    
    LOG_FUNCTION_NAME

    if (isStart_FW3A_AF!=0) {
        LOGE("3A FW AF is already started");
        return -1;
    }

    if (fobj->settings_2a.af.focus_mode != FOCUS_MODE_AF_AUTO) {
      FW3A_GetSettings();
      fobj->settings_2a.af.focus_mode = FOCUS_MODE_AF_AUTO;
      ret = FW3A_SetSettings();
    }
    
    if (ret == 0) {
      ret = fobj->cam_iface_2a->StartAF(fobj->cam_iface_2a->pPrivateHandle);
    }
    if (0 > ret) {
        LOGE("Cannot Start AF");
        return -1;
    } else {
        LOGE("3A FW AF Start - success");
    }

    LOG_FUNCTION_NAME_EXIT

    isStart_FW3A_AF = 1;
    return 0;
}

int CameraHal::FW3A_Stop_AF()
{
    int ret, prev = isStart_FW3A_AF;

#ifdef DEBUG_LOG

    LOG_FUNCTION_NAME

#endif

    if (isStart_FW3A_AF == 0) {
        LOGE("3A FW AF is already stopped");
        return isStart_FW3A_AF;
    }

    //Stop 3AFW
    ret = fobj->cam_iface_2a->StopAF(fobj->cam_iface_2a->pPrivateHandle);
    if (0 > ret) {
        LOGE("Cannot Stop AF");
        return -1;
    }

#ifdef DEBUG_LOG
    else {
        LOGE("3A FW AF Stop - success");
    }
#endif

    isStart_FW3A_AF = 0;

#ifdef DEBUG_LOG
    LOG_FUNCTION_NAME_EXIT
#endif
    return prev;
}

int CameraHal::FW3A_GetSettings()
{
    int err = 0;

#ifdef DEBUG_LOG

    LOG_FUNCTION_NAME

#endif

    err = fobj->cam_iface_2a->ReadSettings(fobj->cam_iface_2a->pPrivateHandle, &fobj->settings_2a);

#ifdef DEBUG_LOG

    LOG_FUNCTION_NAME_EXIT  

#endif

    return err;
}

int CameraHal::FW3A_SetSettings()
{
    int err = 0;

    LOG_FUNCTION_NAME

    err = fobj->cam_iface_2a->WriteSettings(fobj->cam_iface_2a->pPrivateHandle, &fobj->settings_2a);
  
    LOG_FUNCTION_NAME_EXIT

    return err;
}
#endif

#ifdef IMAGE_PROCESSING_PIPELINE

int CameraHal::InitIPP(int w, int h, int fmt, int ippMode)
{
    int eError = 0;

	if( (ippMode != IPP_CromaSupression_Mode) && (ippMode != IPP_EdgeEnhancement_Mode) ){
		LOGE("Error unsupported mode=%d", ippMode);
		return -1;
	}

    pIPP.hIPP = IPP_Create();
    LOGD("IPP Handle: %p",pIPP.hIPP);

	if( !pIPP.hIPP ){
		LOGE("ERROR IPP_Create");
		return -1;
	}

    if( fmt == PIX_YUV420P)
	    pIPP.ippconfig.numberOfAlgos = 3;
	else
	    pIPP.ippconfig.numberOfAlgos = 4;

    pIPP.ippconfig.orderOfAlgos[0]=IPP_START_ID;

    if( fmt != PIX_YUV420P ){
        pIPP.ippconfig.orderOfAlgos[1]=IPP_YUVC_422iTO422p_ID;

	    if(ippMode == IPP_CromaSupression_Mode ){
		    pIPP.ippconfig.orderOfAlgos[2]=IPP_CRCBS_ID;
	    }
	    else if(ippMode == IPP_EdgeEnhancement_Mode){
		    pIPP.ippconfig.orderOfAlgos[2]=IPP_EENF_ID;
	    }

        pIPP.ippconfig.orderOfAlgos[3]=IPP_YUVC_422pTO422i_ID;
        pIPP.ippconfig.isINPLACE=INPLACE_ON;
	} else {
	    if(ippMode == IPP_CromaSupression_Mode ){
		    pIPP.ippconfig.orderOfAlgos[1]=IPP_CRCBS_ID;
	    }
	    else if(ippMode == IPP_EdgeEnhancement_Mode){
		    pIPP.ippconfig.orderOfAlgos[1]=IPP_EENF_ID;
	    }

        pIPP.ippconfig.orderOfAlgos[2]=IPP_YUVC_422pTO422i_ID;
        pIPP.ippconfig.isINPLACE=INPLACE_OFF;
	}

	pIPP.outputBufferSize = (w*h*2);

    LOGD("IPP_SetProcessingConfiguration");
    eError = IPP_SetProcessingConfiguration(pIPP.hIPP, pIPP.ippconfig);
	if(eError != 0){
		LOGE("ERROR IPP_SetProcessingConfiguration");
	}	

	if(ippMode == IPP_CromaSupression_Mode ){
		pIPP.CRCBptr.size = sizeof(IPP_CRCBSAlgoCreateParams);
		pIPP.CRCBptr.maxWidth = w;
		pIPP.CRCBptr.maxHeight = h;
		pIPP.CRCBptr.errorCode = 0;

		LOGD("IPP_SetAlgoConfig CRBS");
		eError = IPP_SetAlgoConfig(pIPP.hIPP, IPP_CRCBS_CREATEPRMS_CFGID, &(pIPP.CRCBptr));
		if(eError != 0){
			LOGE("ERROR IPP_SetAlgoConfig");
		}
	}else if(ippMode == IPP_EdgeEnhancement_Mode ){
		pIPP.EENFcreate.size = sizeof(IPP_EENFAlgoCreateParams);
		pIPP.EENFcreate.maxImageSizeH = w;
		pIPP.EENFcreate.maxImageSizeV = h;
		pIPP.EENFcreate.errorCode = 0;
		pIPP.EENFcreate.inPlace = 0;
		pIPP.EENFcreate.inputBufferSizeForInPlace = 0;

		LOGD("IPP_SetAlgoConfig EENF");
		eError = IPP_SetAlgoConfig(pIPP.hIPP, IPP_EENF_CREATEPRMS_CFGID, &(pIPP.EENFcreate));
		if(eError != 0){
			LOGE("ERROR IPP_SetAlgoConfig");
		}
	}

    pIPP.YUVCcreate.size = sizeof(IPP_YUVCAlgoCreateParams);
    pIPP.YUVCcreate.maxWidth = w;
    pIPP.YUVCcreate.maxHeight = h;
    pIPP.YUVCcreate.errorCode = 0;

    if ( fmt != PIX_YUV420P ) {
        LOGD("IPP_SetAlgoConfig: IPP_YUVC_422TO420_CREATEPRMS_CFGID");
        eError = IPP_SetAlgoConfig(pIPP.hIPP, IPP_YUVC_422TO420_CREATEPRMS_CFGID, &(pIPP.YUVCcreate));
	    if(eError != 0){
		    LOGE("ERROR IPP_SetAlgoConfig: IPP_YUVC_422TO420_CREATEPRMS_CFGID");
	    }
	}

    LOGD("IPP_SetAlgoConfig: IPP_YUVC_420TO422_CREATEPRMS_CFGID");
    eError = IPP_SetAlgoConfig(pIPP.hIPP, IPP_YUVC_420TO422_CREATEPRMS_CFGID, &(pIPP.YUVCcreate));
    if(eError != 0){
        LOGE("ERROR IPP_SetAlgoConfig: IPP_YUVC_420TO422_CREATEPRMS_CFGID");
    }

    LOGD("IPP_InitializeImagePipe");
    eError = IPP_InitializeImagePipe(pIPP.hIPP);
	if(eError != 0){
		LOGE("ERROR IPP_InitializeImagePipe");
	} else {
        mIPPInitAlgoState = true;
    }

    pIPP.iStarInArgs = (IPP_StarAlgoInArgs*)((char*)DSP_CACHE_ALIGN_MEM_ALLOC(sizeof(IPP_StarAlgoInArgs)));
    pIPP.iStarOutArgs = (IPP_StarAlgoOutArgs*)((char*)DSP_CACHE_ALIGN_MEM_ALLOC(sizeof(IPP_StarAlgoOutArgs)));

	if(ippMode == IPP_CromaSupression_Mode ){
        pIPP.iCrcbsInArgs = (IPP_CRCBSAlgoInArgs*)((char*)DSP_CACHE_ALIGN_MEM_ALLOC(sizeof(IPP_CRCBSAlgoInArgs)));
        pIPP.iCrcbsOutArgs = (IPP_CRCBSAlgoOutArgs*)((char*)DSP_CACHE_ALIGN_MEM_ALLOC(sizeof(IPP_CRCBSAlgoOutArgs)));
	}

	if(ippMode == IPP_EdgeEnhancement_Mode){
        pIPP.iEenfInArgs = (IPP_EENFAlgoInArgs*)((char*)DSP_CACHE_ALIGN_MEM_ALLOC(sizeof(IPP_EENFAlgoInArgs)));
        pIPP.iEenfOutArgs = (IPP_EENFAlgoOutArgs*)((char*)DSP_CACHE_ALIGN_MEM_ALLOC(sizeof(IPP_EENFAlgoOutArgs)));
	}

    pIPP.iYuvcInArgs1 = (IPP_YUVCAlgoInArgs*)((char*)DSP_CACHE_ALIGN_MEM_ALLOC(sizeof(IPP_YUVCAlgoInArgs)));
    pIPP.iYuvcOutArgs1 = (IPP_YUVCAlgoOutArgs*)((char*)DSP_CACHE_ALIGN_MEM_ALLOC(sizeof(IPP_YUVCAlgoOutArgs)));
    pIPP.iYuvcInArgs2 = (IPP_YUVCAlgoInArgs*)((char*)DSP_CACHE_ALIGN_MEM_ALLOC(sizeof(IPP_YUVCAlgoInArgs)));
    pIPP.iYuvcOutArgs2 = (IPP_YUVCAlgoOutArgs*)((char*)DSP_CACHE_ALIGN_MEM_ALLOC(sizeof(IPP_YUVCAlgoOutArgs)));

	if(ippMode == IPP_EdgeEnhancement_Mode){
        pIPP.dynEENF = (IPP_EENFAlgoDynamicParams*)((char*)DSP_CACHE_ALIGN_MEM_ALLOC(sizeof(IPP_EENFAlgoDynamicParams)));
	}

	if( !(pIPP.ippconfig.isINPLACE) ){
		pIPP.pIppOutputBuffer= (unsigned char*)DSP_CACHE_ALIGN_MEM_ALLOC(pIPP.outputBufferSize); // TODO make it dependent on the output format
	}
    
    return eError;
}
/*-------------------------------------------------------------------*/
/**
  * DeInitIPP()
  *
  *
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
int CameraHal::DeInitIPP(int ippMode)
{
    int eError = 0;

    if(mIPPInitAlgoState){
        LOGD("IPP_DeinitializePipe");
        eError = IPP_DeinitializePipe(pIPP.hIPP);
        LOGD("IPP_DeinitializePipe");
        if( eError != 0){
            LOGE("ERROR IPP_DeinitializePipe");
        }
        mIPPInitAlgoState = false;
    }

    LOGD("IPP_Delete");
    IPP_Delete(&(pIPP.hIPP));

    free(((char*)pIPP.iStarInArgs));
    free(((char*)pIPP.iStarOutArgs));
	
	if(ippMode == IPP_CromaSupression_Mode ){
        free(((char*)pIPP.iCrcbsInArgs));
        free(((char*)pIPP.iCrcbsOutArgs));
	}

	if(ippMode == IPP_EdgeEnhancement_Mode){
        free(((char*)pIPP.iEenfInArgs));
        free(((char*)pIPP.iEenfOutArgs));
	}

    free(((char*)pIPP.iYuvcInArgs1));
    free(((char*)pIPP.iYuvcOutArgs1));
    free(((char*)pIPP.iYuvcInArgs2));
    free(((char*)pIPP.iYuvcOutArgs2));
	
	if(ippMode == IPP_EdgeEnhancement_Mode){
        free(((char*)pIPP.dynEENF));
	}

	if(!(pIPP.ippconfig.isINPLACE)){
		free(pIPP.pIppOutputBuffer);
	}

    LOGD("Terminating IPP");
    
    return eError;
}

int CameraHal::PopulateArgsIPP(int w, int h, int fmt, int ippMode)
{
    int eError = 0;
    
    LOGD("IPP: PopulateArgs ENTER");

	//configuring size of input and output structures
    pIPP.iStarInArgs->size = sizeof(IPP_StarAlgoInArgs);
	if(ippMode == IPP_CromaSupression_Mode ){
	    pIPP.iCrcbsInArgs->size = sizeof(IPP_CRCBSAlgoInArgs);
	}
	if(ippMode == IPP_EdgeEnhancement_Mode){
	    pIPP.iEenfInArgs->size = sizeof(IPP_EENFAlgoInArgs);
	}
	
    pIPP.iYuvcInArgs1->size = sizeof(IPP_YUVCAlgoInArgs);
    pIPP.iYuvcInArgs2->size = sizeof(IPP_YUVCAlgoInArgs);

    pIPP.iStarOutArgs->size = sizeof(IPP_StarAlgoOutArgs);
	if(ippMode == IPP_CromaSupression_Mode ){
    	pIPP.iCrcbsOutArgs->size = sizeof(IPP_CRCBSAlgoOutArgs);
	}
	if(ippMode == IPP_EdgeEnhancement_Mode){
    	pIPP.iEenfOutArgs->size = sizeof(IPP_EENFAlgoOutArgs);
	}
	
    pIPP.iYuvcOutArgs1->size = sizeof(IPP_YUVCAlgoOutArgs);
    pIPP.iYuvcOutArgs2->size = sizeof(IPP_YUVCAlgoOutArgs);
    
	//Configuring specific data of algorithms
	if(ippMode == IPP_CromaSupression_Mode ){
	    pIPP.iCrcbsInArgs->inputHeight = h;
	    pIPP.iCrcbsInArgs->inputWidth = w;	
	    pIPP.iCrcbsInArgs->inputChromaFormat = IPP_YUV_420P;
	}

	if(ippMode == IPP_EdgeEnhancement_Mode){
		pIPP.iEenfInArgs->inputChromaFormat = IPP_YUV_420P;
		pIPP.iEenfInArgs->inFullWidth = w;
		pIPP.iEenfInArgs->inFullHeight = h;
		pIPP.iEenfInArgs->inOffsetV = 0;
		pIPP.iEenfInArgs->inOffsetH = 0;
		pIPP.iEenfInArgs->inputWidth = w;
		pIPP.iEenfInArgs->inputHeight = h;
		pIPP.iEenfInArgs->inPlace = 0;
		pIPP.iEenfInArgs->NFprocessing = 0;
    }

    if ( fmt == PIX_YUV422I ) {
        pIPP.iYuvcInArgs1->inputChromaFormat = IPP_YUV_422ILE;
        pIPP.iYuvcInArgs1->outputChromaFormat = IPP_YUV_420P;
        pIPP.iYuvcInArgs1->inputHeight = h;
        pIPP.iYuvcInArgs1->inputWidth = w;
    }

    pIPP.iYuvcInArgs2->inputChromaFormat = IPP_YUV_420P;
    pIPP.iYuvcInArgs2->outputChromaFormat = IPP_YUV_422ILE;
    pIPP.iYuvcInArgs2->inputHeight = h;
    pIPP.iYuvcInArgs2->inputWidth = w;

	pIPP.iStarOutArgs->extendedError= 0;
	pIPP.iYuvcOutArgs1->extendedError = 0;
	pIPP.iYuvcOutArgs2->extendedError = 0;
	if(ippMode == IPP_EdgeEnhancement_Mode)
		pIPP.iEenfOutArgs->extendedError = 0;
	if(ippMode == IPP_CromaSupression_Mode )
		pIPP.iCrcbsOutArgs->extendedError = 0;

	//Filling ipp status structure
    pIPP.starStatus.size = sizeof(IPP_StarAlgoStatus);
	if(ippMode == IPP_CromaSupression_Mode ){
    	pIPP.CRCBSStatus.size = sizeof(IPP_CRCBSAlgoStatus);
	}
	if(ippMode == IPP_EdgeEnhancement_Mode){
    	pIPP.EENFStatus.size = sizeof(IPP_EENFAlgoStatus);
	}

    pIPP.statusDesc.statusPtr[0] = &(pIPP.starStatus);
	if(ippMode == IPP_CromaSupression_Mode ){
	    pIPP.statusDesc.statusPtr[1] = &(pIPP.CRCBSStatus);
	}
	if(ippMode == IPP_EdgeEnhancement_Mode){
	    pIPP.statusDesc.statusPtr[1] = &(pIPP.EENFStatus);
	}
	
    pIPP.statusDesc.numParams = 2;
    pIPP.statusDesc.algoNum[0] = 0;
    pIPP.statusDesc.algoNum[1] = 1;
    pIPP.statusDesc.algoNum[1] = 1;

    LOGD("IPP: PopulateArgs EXIT");
    
    return eError;
}

int CameraHal::ProcessBufferIPP(void *pBuffer, long int nAllocLen, int fmt, int ippMode,
                               int EdgeEnhancementStrength, int WeakEdgeThreshold, int StrongEdgeThreshold,
                                int LowFreqLumaNoiseFilterStrength, int MidFreqLumaNoiseFilterStrength, int HighFreqLumaNoiseFilterStrength,
                                int LowFreqCbNoiseFilterStrength, int MidFreqCbNoiseFilterStrength, int HighFreqCbNoiseFilterStrength,
                                int LowFreqCrNoiseFilterStrength, int MidFreqCrNoiseFilterStrength, int HighFreqCrNoiseFilterStrength,
                                int shadingVertParam1, int shadingVertParam2, int shadingHorzParam1, int shadingHorzParam2,
                                int shadingGainScale, int shadingGainOffset, int shadingGainMaxValue,
                                int ratioDownsampleCbCr)
{
    int eError = 0;
    IPP_EENFAlgoDynamicParams IPPEENFAlgoDynamicParamsCfg =
    {
        sizeof(IPP_EENFAlgoDynamicParams),
        0,//  inPlace
        150,//  EdgeEnhancementStrength;
        100,//  WeakEdgeThreshold;
        300,//  StrongEdgeThreshold;
        30,//  LowFreqLumaNoiseFilterStrength;
        80,//  MidFreqLumaNoiseFilterStrength;
        20,//  HighFreqLumaNoiseFilterStrength;
        60,//  LowFreqCbNoiseFilterStrength;
        40,//  MidFreqCbNoiseFilterStrength;
        30,//  HighFreqCbNoiseFilterStrength;
        50,//  LowFreqCrNoiseFilterStrength;
        30,//  MidFreqCrNoiseFilterStrength;
        20,//  HighFreqCrNoiseFilterStrength;
        1,//  shadingVertParam1;
        800,//  shadingVertParam2;
        1,//  shadingHorzParam1;
        800,//  shadingHorzParam2;
        128,//  shadingGainScale;
        4096,//  shadingGainOffset;
        24576,//  shadingGainMaxValue;
        1//  ratioDownsampleCbCr;
    };//2

    LOGD("IPP_StartProcessing");
    eError = IPP_StartProcessing(pIPP.hIPP);
    if(eError != 0){
        LOGE("ERROR IPP_SetAlgoConfig");
    }

	if(ippMode == IPP_EdgeEnhancement_Mode){
       IPPEENFAlgoDynamicParamsCfg.inPlace = 0;
        NONNEG_ASSIGN(EdgeEnhancementStrength, IPPEENFAlgoDynamicParamsCfg.EdgeEnhancementStrength);
        NONNEG_ASSIGN(WeakEdgeThreshold, IPPEENFAlgoDynamicParamsCfg.WeakEdgeThreshold);
        NONNEG_ASSIGN(StrongEdgeThreshold, IPPEENFAlgoDynamicParamsCfg.StrongEdgeThreshold);
        NONNEG_ASSIGN(LowFreqLumaNoiseFilterStrength, IPPEENFAlgoDynamicParamsCfg.LowFreqLumaNoiseFilterStrength);
        NONNEG_ASSIGN(MidFreqLumaNoiseFilterStrength, IPPEENFAlgoDynamicParamsCfg.MidFreqLumaNoiseFilterStrength);
        NONNEG_ASSIGN(HighFreqLumaNoiseFilterStrength, IPPEENFAlgoDynamicParamsCfg.HighFreqLumaNoiseFilterStrength);
        NONNEG_ASSIGN(LowFreqCbNoiseFilterStrength, IPPEENFAlgoDynamicParamsCfg.LowFreqCbNoiseFilterStrength);
        NONNEG_ASSIGN(MidFreqCbNoiseFilterStrength, IPPEENFAlgoDynamicParamsCfg.MidFreqCbNoiseFilterStrength);
        NONNEG_ASSIGN(HighFreqCbNoiseFilterStrength, IPPEENFAlgoDynamicParamsCfg.HighFreqCbNoiseFilterStrength);
        NONNEG_ASSIGN(LowFreqCrNoiseFilterStrength, IPPEENFAlgoDynamicParamsCfg.LowFreqCrNoiseFilterStrength);
        NONNEG_ASSIGN(MidFreqCrNoiseFilterStrength, IPPEENFAlgoDynamicParamsCfg.MidFreqCrNoiseFilterStrength);
        NONNEG_ASSIGN(HighFreqCrNoiseFilterStrength, IPPEENFAlgoDynamicParamsCfg.HighFreqCrNoiseFilterStrength);
        NONNEG_ASSIGN(shadingVertParam1, IPPEENFAlgoDynamicParamsCfg.shadingVertParam1);
        NONNEG_ASSIGN(shadingVertParam2, IPPEENFAlgoDynamicParamsCfg.shadingVertParam2);
        NONNEG_ASSIGN(shadingHorzParam1, IPPEENFAlgoDynamicParamsCfg.shadingHorzParam1);
        NONNEG_ASSIGN(shadingHorzParam2, IPPEENFAlgoDynamicParamsCfg.shadingHorzParam2);
        NONNEG_ASSIGN(shadingGainScale, IPPEENFAlgoDynamicParamsCfg.shadingGainScale);
        NONNEG_ASSIGN(shadingGainOffset, IPPEENFAlgoDynamicParamsCfg.shadingGainOffset);
        NONNEG_ASSIGN(shadingGainMaxValue, IPPEENFAlgoDynamicParamsCfg.shadingGainMaxValue);
        NONNEG_ASSIGN(ratioDownsampleCbCr, IPPEENFAlgoDynamicParamsCfg.ratioDownsampleCbCr);

        LOGD("Set EENF Dynamics Params:");
        LOGD("\tInPlace                      = %d", (int)IPPEENFAlgoDynamicParamsCfg.inPlace);
        LOGD("\tEdge Enhancement Strength    = %d", (int)IPPEENFAlgoDynamicParamsCfg.EdgeEnhancementStrength);
        LOGD("\tWeak Edge Threshold          = %d", (int)IPPEENFAlgoDynamicParamsCfg.WeakEdgeThreshold);
        LOGD("\tStrong Edge Threshold        = %d", (int)IPPEENFAlgoDynamicParamsCfg.StrongEdgeThreshold);
        LOGD("\tLuma Noise Filter Low Freq Strength   = %d", (int)IPPEENFAlgoDynamicParamsCfg.LowFreqLumaNoiseFilterStrength );
        LOGD("\tLuma Noise Filter Mid Freq Strength   = %d", (int)IPPEENFAlgoDynamicParamsCfg.MidFreqLumaNoiseFilterStrength );
        LOGD("\tLuma Noise Filter High Freq Strength   = %d", (int)IPPEENFAlgoDynamicParamsCfg.HighFreqLumaNoiseFilterStrength );
        LOGD("\tChroma Noise Filter Low Freq Cb Strength = %d", (int)IPPEENFAlgoDynamicParamsCfg.LowFreqCbNoiseFilterStrength);
        LOGD("\tChroma Noise Filter Mid Freq Cb Strength = %d", (int)IPPEENFAlgoDynamicParamsCfg.MidFreqCbNoiseFilterStrength);
        LOGD("\tChroma Noise Filter High Freq Cb Strength = %d", (int)IPPEENFAlgoDynamicParamsCfg.HighFreqCbNoiseFilterStrength);
        LOGD("\tChroma Noise Filter Low Freq Cr Strength = %d", (int)IPPEENFAlgoDynamicParamsCfg.LowFreqCrNoiseFilterStrength);
        LOGD("\tChroma Noise Filter Mid Freq Cr Strength = %d", (int)IPPEENFAlgoDynamicParamsCfg.MidFreqCrNoiseFilterStrength);
        LOGD("\tChroma Noise Filter High Freq Cr Strength = %d", (int)IPPEENFAlgoDynamicParamsCfg.HighFreqCrNoiseFilterStrength);
        LOGD("\tShading Vert 1 = %d", (int)IPPEENFAlgoDynamicParamsCfg.shadingVertParam1);
        LOGD("\tShading Vert 2 = %d", (int)IPPEENFAlgoDynamicParamsCfg.shadingVertParam2);
        LOGD("\tShading Horz 1 = %d", (int)IPPEENFAlgoDynamicParamsCfg.shadingHorzParam1);
        LOGD("\tShading Horz 2 = %d", (int)IPPEENFAlgoDynamicParamsCfg.shadingHorzParam2);
        LOGD("\tShading Gain Scale = %d", (int)IPPEENFAlgoDynamicParamsCfg.shadingGainScale);
        LOGD("\tShading Gain Offset = %d", (int)IPPEENFAlgoDynamicParamsCfg.shadingGainOffset);
        LOGD("\tShading Gain Max Val = %d", (int)IPPEENFAlgoDynamicParamsCfg.shadingGainMaxValue);
        LOGD("\tRatio Downsample CbCr = %d", (int)IPPEENFAlgoDynamicParamsCfg.ratioDownsampleCbCr);

		    
		/*Set Dynamic Parameter*/
		memcpy(pIPP.dynEENF,
		       (void*)&IPPEENFAlgoDynamicParamsCfg,
		       sizeof(IPPEENFAlgoDynamicParamsCfg));

	
		LOGD("IPP_SetAlgoConfig");
		eError = IPP_SetAlgoConfig(pIPP.hIPP, IPP_EENF_DYNPRMS_CFGID, (void*)pIPP.dynEENF);
		if( eError != 0){
			LOGE("ERROR IPP_SetAlgoConfig");
		}
	}

    pIPP.iInputBufferDesc.numBuffers = 1;
    pIPP.iInputBufferDesc.bufPtr[0] = pBuffer;
    pIPP.iInputBufferDesc.bufSize[0] = nAllocLen;
    pIPP.iInputBufferDesc.usedSize[0] = nAllocLen;
    pIPP.iInputBufferDesc.port[0] = 0;
    pIPP.iInputBufferDesc.reuseAllowed[0] = 0;

	if(!(pIPP.ippconfig.isINPLACE)){
		pIPP.iOutputBufferDesc.numBuffers = 1;
		pIPP.iOutputBufferDesc.bufPtr[0] = pIPP.pIppOutputBuffer;						/*TODO, depend on pix format*/
		pIPP.iOutputBufferDesc.bufSize[0] = pIPP.outputBufferSize; 
		pIPP.iOutputBufferDesc.usedSize[0] = pIPP.outputBufferSize;
		pIPP.iOutputBufferDesc.port[0] = 1;
		pIPP.iOutputBufferDesc.reuseAllowed[0] = 0;
	}

    if ( fmt == PIX_YUV422I){
	    pIPP.iInputArgs.numArgs = 4;
	    pIPP.iOutputArgs.numArgs = 4;

        pIPP.iInputArgs.argsArray[0] = pIPP.iStarInArgs;
        pIPP.iInputArgs.argsArray[1] = pIPP.iYuvcInArgs1;
	    if(ippMode == IPP_CromaSupression_Mode ){
	        pIPP.iInputArgs.argsArray[2] = pIPP.iCrcbsInArgs;
	    }
	    if(ippMode == IPP_EdgeEnhancement_Mode){
		    pIPP.iInputArgs.argsArray[2] = pIPP.iEenfInArgs;
	    }
        pIPP.iInputArgs.argsArray[3] = pIPP.iYuvcInArgs2;

        pIPP.iOutputArgs.argsArray[0] = pIPP.iStarOutArgs;
        pIPP.iOutputArgs.argsArray[1] = pIPP.iYuvcOutArgs1;
        if(ippMode == IPP_CromaSupression_Mode ){
            pIPP.iOutputArgs.argsArray[2] = pIPP.iCrcbsOutArgs;
        }
        if(ippMode == IPP_EdgeEnhancement_Mode){
            pIPP.iOutputArgs.argsArray[2] = pIPP.iEenfOutArgs;
        }
        pIPP.iOutputArgs.argsArray[3] = pIPP.iYuvcOutArgs2;
	 } else {
        pIPP.iInputArgs.numArgs = 3;
        pIPP.iOutputArgs.numArgs = 3;

        pIPP.iInputArgs.argsArray[0] = pIPP.iStarInArgs;
        if(ippMode == IPP_CromaSupression_Mode ){
            pIPP.iInputArgs.argsArray[1] = pIPP.iCrcbsInArgs;
        }
        if(ippMode == IPP_EdgeEnhancement_Mode){
            pIPP.iInputArgs.argsArray[1] = pIPP.iEenfInArgs;
        }
        pIPP.iInputArgs.argsArray[2] = pIPP.iYuvcInArgs2;

        pIPP.iOutputArgs.argsArray[0] = pIPP.iStarOutArgs;
        if(ippMode == IPP_CromaSupression_Mode ){
            pIPP.iOutputArgs.argsArray[1] = pIPP.iCrcbsOutArgs;
        }
        if(ippMode == IPP_EdgeEnhancement_Mode){
            pIPP.iOutputArgs.argsArray[1] = pIPP.iEenfOutArgs;
        }
        pIPP.iOutputArgs.argsArray[2] = pIPP.iYuvcOutArgs2;
    }
   
    LOGD("IPP_ProcessImage");
	if((pIPP.ippconfig.isINPLACE)){
		eError = IPP_ProcessImage(pIPP.hIPP, &(pIPP.iInputBufferDesc), &(pIPP.iInputArgs), NULL, &(pIPP.iOutputArgs));
	}
	else{
		eError = IPP_ProcessImage(pIPP.hIPP, &(pIPP.iInputBufferDesc), &(pIPP.iInputArgs), &(pIPP.iOutputBufferDesc),&(pIPP.iOutputArgs));
	}    
    if( eError != 0){
		LOGE("ERROR IPP_ProcessImage");
	}

    LOGD("IPP_StopProcessing");
    eError = IPP_StopProcessing(pIPP.hIPP);
    if( eError != 0){
        LOGE("ERROR IPP_StopProcessing");
    }

	LOGD("IPP_ProcessImage Done");
    
    return eError;
}

#endif

int CameraHal::CorrectPreview()
{
    int dst_width, dst_height;
    struct v4l2_crop crop;
    struct v4l2_cropcap cropcap;
    int ret;
    int pos_l, pos_t, pos_w, pos_h;

#ifdef DEBUG_LOG

    LOG_FUNCTION_NAME

#endif

    mParameters.getPreviewSize(&dst_width, &dst_height);

    ret = ioctl(camera_device, VIDIOC_CROPCAP, &cropcap);
    if ( ret < 0) {
        LOGE("Error while retrieving crop capabilities");

        return EINVAL;
    }

    if (aspect_ratio_calc(cropcap.bounds.width - 8, cropcap.bounds.height - 8,
                         cropcap.pixelaspect.numerator, cropcap.pixelaspect.denominator,
                         cropcap.bounds.width - 8, cropcap.bounds.height - 8,
                         dst_width, dst_height,
                         2, 2, 1, 1,
                         (int *) &crop.c.left, (int *) &crop.c.top,
                         (int *) &crop.c.width, (int *) &crop.c.height,
                         &pos_l, &pos_t, &pos_w, &pos_h,
                         ASPECT_RATIO_FLAG_KEEP_ASPECT_RATIO|ASPECT_RATIO_FLAG_CROP_BY_SOURCE)) {

        LOGE("Error while calculating crop");

        return -1;
    }

    ret = ioctl(camera_device, VIDIOC_S_CROP, &crop);
    if (ret < 0) {
      LOGE("[%s]: ERROR VIDIOC_S_CROP failed", strerror(errno));
      return -1;
    }

    ret = ioctl(camera_device, VIDIOC_G_CROP, &crop);
    if (ret < 0) {
      LOGE("[%s]: ERROR VIDIOC_G_CROP failed", strerror(errno));
      return -1;
    }

    mInitialCrop.c.top = crop.c.top;
    mInitialCrop.c.left = crop.c.left;
    mInitialCrop.c.width = crop.c.width;
    mInitialCrop.c.height = crop.c.height;

#ifdef DEBUG_LOG

    LOGE("VIDIOC_G_CROP: top = %d, left = %d, width = %d, height = %d", crop.c.top, crop.c.left, crop.c.width, crop.c.height);

    LOG_FUNCTION_NAME_EXIT

#endif

    return NO_ERROR;
}

int CameraHal::ZoomPerform(float zoom)
{
    struct v4l2_crop crop;
    int delta_x, delta_y;
    int ret;

    LOG_FUNCTION_NAME

    memcpy( &crop, &mInitialCrop, sizeof(struct v4l2_crop));

    delta_x = crop.c.width - (crop.c.width /zoom);
    delta_y = crop.c.height - (crop.c.height/zoom);
 
    crop.c.width -= delta_x;
    crop.c.height -= delta_y;
    crop.c.left += delta_x >> 1;
    crop.c.top += delta_y >> 1;

    LOGE("Perform ZOOM: zoom_top = %d, zoom_left = %d, zoom_width = %d, zoom_height = %d", crop.c.top, crop.c.left, crop.c.width, crop.c.height);

    ret = ioctl(camera_device, VIDIOC_S_CROP, &crop);
    if (ret < 0) {
      LOGE("[%s]: ERROR VIDIOC_S_CROP failed", strerror(errno));
      return -1;
    }

    ret = ioctl(camera_device, VIDIOC_G_CROP, &crop);
    if (ret < 0) {
      LOGE("[%s]: ERROR VIDIOC_G_CROP failed", strerror(errno));
      return -1;
    }

    LOGE("Perform ZOOM G_GROP: crop_top = %d, crop_left = %d, crop_width = %d, crop_height = %d", crop.c.top, crop.c.left, crop.c.width, crop.c.height);

    LOG_FUNCTION_NAME_EXIT

    return 0;
}

/************/

void CameraHal::PPM(const char* str){

#if PPM_INSTRUMENTATION

    gettimeofday(&ppm, NULL);
    ppm.tv_sec = ppm.tv_sec - ppm_start.tv_sec;
    ppm.tv_sec = ppm.tv_sec * 1000000;
    ppm.tv_sec = ppm.tv_sec + ppm.tv_usec - ppm_start.tv_usec;
    LOGD("PPM: %s :%ld.%ld ms",str, ppm.tv_sec/1000, ppm.tv_sec%1000 );

#elif PPM_INSTRUMENTATION_ABS

    unsigned long long elapsed, absolute;
    gettimeofday(&ppm, NULL);
    elapsed = ppm.tv_sec - ppm_start.tv_sec;
    elapsed *= 1000000;
    elapsed += ppm.tv_usec - ppm_start.tv_usec;
    absolute = ppm.tv_sec;
    absolute *= 1000;
    absolute += ppm.tv_usec/1000;
	LOGD("PPM: %s :%llu.%llu ms : %llu ms",str, elapsed/1000, elapsed%1000, absolute);

 #endif

}

void CameraHal::PPM(const char* str, struct timeval* ppm_first, ...){
#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
    char temp_str[256];
    va_list args;
    va_start(args, ppm_first);
    vsprintf(temp_str, str, args);
	gettimeofday(&ppm, NULL); 
	ppm.tv_sec = ppm.tv_sec - ppm_first->tv_sec; 
	ppm.tv_sec = ppm.tv_sec * 1000000; 
	ppm.tv_sec = ppm.tv_sec + ppm.tv_usec - ppm_first->tv_usec; 
	LOGD("PPM: %s :%ld.%ld ms",temp_str, ppm.tv_sec/1000, ppm.tv_sec%1000 );
    va_end(args);
#endif
}

};



