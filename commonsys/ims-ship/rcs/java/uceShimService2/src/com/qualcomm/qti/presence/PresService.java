/*********************************************************************
 Copyright (c) 2017,2019 Qualcomm Technologies, Inc.
 All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
**********************************************************************/

package com.qualcomm.qti.presence;

import com.android.ims.internal.uce.common.UceLong;
import com.android.ims.internal.uce.common.StatusCode;
import com.android.ims.internal.uce.presence.*;

import com.qualcomm.qti.uceservice.V2_0.UceStatus;
import com.qualcomm.qti.uceservice.V2_0.UceStatusCode;
import com.qualcomm.qti.uceservice.V2_0.PresenceCapabilityInfo;
import com.qualcomm.qti.uceservice.V2_0.PresServiceInfo;
import com.qualcomm.qti.uceservice.V2_0.RcsFeatureTag;

import com.qualcomm.qti.translator.*;

import android.os.RemoteException;
import android.os.Parcel;
import android.util.Log;

import java.util.Arrays;
import java.util.ArrayList;
import java.util.List;
import java.util.Vector;
import java.util.Map;
import java.util.HashMap;

import com.qualcomm.qti.uceShimService.RCSService;


public class PresService extends IPresenceService.Stub {

    private final static String logTAG = "QRCSService PresService";
    private AidlToHidlTranslator translateToHidlObj = null;
    private HidlToAidlTranslator translateToAidlObj = null;
    // maintatin list of HIDL Listeners
    private static List<PresListener> hidlListenerlist = new ArrayList<PresListener>();

    private HalVersionWrapper mHalWrapper = null;

    public PresService() {
        if(RCSService.mHidlService_2_2 != null) {
            mHalWrapper = new Version2_2();
        } else if(RCSService.mHidlService_2_1 != null) {
            mHalWrapper = new Version2_1();
        } else {
            mHalWrapper = new Version2_0();
        }
    }

    public void setServiceHandle (int serviceHandle) {
        mHalWrapper.setServiceHandle(serviceHandle);
    }
    public StatusCode getVersion(int pPresServiceHdl)
            throws RemoteException {

        StatusCode retStatus = new StatusCode();
        retStatus.setStatusCode(StatusCode.UCE_NOT_SUPPORTED);
        return retStatus;
    }

    public void addHidlListener(PresListener hidlListener) {
        hidlListenerlist.add(hidlListener);
    }

    public StatusCode addListener(int pPresServiceHdl,
            IPresenceListener pPresServiceListener,
            UceLong pPresServiceListenerHdl) throws RemoteException {
        StatusCode retStatus = new StatusCode();
        retStatus.setStatusCode(StatusCode.UCE_FAILURE);
        translateToAidlObj = new HidlToAidlTranslator();

        long clientHandle = pPresServiceListenerHdl.getUceLong();
        PresListener hidlPresListener = new PresListener();
        addHidlListener(hidlPresListener);
        hidlPresListener.setAidlPresListener(pPresServiceListener);

        try {
            UceStatus pStatus = mHalWrapper.addListener(pPresServiceHdl, hidlPresListener, clientHandle);
            retStatus.setStatusCode(translateToAidlObj.getAidlStatusCode(pStatus.status));
        } catch(RemoteException e) {
           Log.w(logTAG, "Unexpected remote exception", e);
           throw e;
        }
        return retStatus;
    }

    public StatusCode removeListener(int pPresServiceHdl,
            UceLong pPresServiceListenerHdl) throws RemoteException {

        StatusCode retStatus = new StatusCode();
        retStatus.setStatusCode(StatusCode.UCE_FAILURE);
        translateToAidlObj = new HidlToAidlTranslator();

        long presServiceListenerHdl = pPresServiceListenerHdl.getUceLong();
        try {
			UceStatus pStatus = mHalWrapper.removeListener(pPresServiceHdl, presServiceListenerHdl);

            retStatus.setStatusCode(translateToAidlObj.getAidlStatusCode(pStatus.status));
        } catch(RemoteException | RuntimeException e) {
           Log.w(logTAG, "Unexpected remote exception", e);
           throw e;
        }
        return retStatus;
    }
//no o/p type mentioned for reEnableService
    public StatusCode reenableService(int pPresServiceHdl,
                                      int pUserData) throws RemoteException {

        StatusCode retStatus = new StatusCode();
        retStatus.setStatusCode(StatusCode.UCE_FAILURE);
        translateToAidlObj = new HidlToAidlTranslator();
        UceStatus pStatus = null;
        try {
            pStatus = mHalWrapper.reenableService(pPresServiceHdl, pUserData);
            retStatus.setStatusCode(translateToAidlObj.getAidlStatusCode(pStatus.status));
        }
        catch( RemoteException | RuntimeException e ) {
        Log.w(logTAG, "Unexpected remote exception", e);
        throw e;
        }
        return retStatus;
    }

    public StatusCode publishMyCap(int pPresServiceHdl,
            PresCapInfo  pMyCapInfo, int pUserData) throws RemoteException {
        StatusCode retStatus = new StatusCode();
        retStatus.setStatusCode(StatusCode.UCE_FAILURE);
        translateToHidlObj = new AidlToHidlTranslator();
        translateToAidlObj = new HidlToAidlTranslator();
        try {
            UceStatus pStatus = mHalWrapper.publishMyCap(pPresServiceHdl, pMyCapInfo, pUserData);
            retStatus.setStatusCode(translateToAidlObj.getAidlStatusCode(pStatus.status));
        } catch(RemoteException | RuntimeException e) {
           Log.w(logTAG, "Unexpected remote exception", e);
           throw e;
        }
        return retStatus;
    }

    public StatusCode getContactCap(int pPresServiceHdl,
            String pRemoteURI, int pUserData) throws RemoteException {

        StatusCode retStatus = new StatusCode();
        retStatus.setStatusCode(StatusCode.UCE_FAILURE);
        translateToAidlObj = new HidlToAidlTranslator();
        try {
			UceStatus pStatus = mHalWrapper.getContactCap(pPresServiceHdl, pRemoteURI, pUserData);
            retStatus.setStatusCode(translateToAidlObj.getAidlStatusCode(pStatus.status));
        } catch(RemoteException | RuntimeException e) {
           Log.w(logTAG, "Unexpected remote exception", e);
           throw e;
        }
        return retStatus;
    }

    public StatusCode getContactListCap(int pPresServiceHdl,
            String[] pRemoteURIList, int pUserData)
            throws RemoteException {

        StatusCode retStatus = new StatusCode();
        retStatus.setStatusCode(StatusCode.UCE_FAILURE);
        translateToAidlObj = new HidlToAidlTranslator();

        ArrayList<String> stringRemoteURIList = new ArrayList<String>(Arrays.asList(pRemoteURIList));
        try {
            UceStatus pStatus= mHalWrapper.getContactListCap(pPresServiceHdl, stringRemoteURIList, pUserData);

            retStatus.setStatusCode(translateToAidlObj.getAidlStatusCode(pStatus.status));
        } catch(RemoteException | RuntimeException e) {
           Log.w(logTAG, "Unexpected remote exception", e);
           throw e;
        }
        return retStatus;
    }

    public StatusCode setNewFeatureTag(int pPresServiceHdl, String pFeatureTag,
            com.android.ims.internal.uce.presence.PresServiceInfo pServiceInfo, int pUserData)
            throws RemoteException {

        StatusCode retStatus = new StatusCode();
        retStatus.setStatusCode(StatusCode.UCE_FAILURE);
        translateToAidlObj = new HidlToAidlTranslator();
        translateToHidlObj = new AidlToHidlTranslator();

        RcsFeatureTag hidlRcsFeatureTag = new RcsFeatureTag();
        hidlRcsFeatureTag.featureTag = pFeatureTag;
        PresServiceInfo hidlPresSvcInfo = new PresServiceInfo();
        hidlPresSvcInfo = translateToHidlObj.getHidlPresSvcInfo(pServiceInfo);
        try {
            UceStatus pStatus = mHalWrapper.setNewFeatureTag(pPresServiceHdl,
                     hidlRcsFeatureTag, hidlPresSvcInfo, pUserData);
            retStatus.setStatusCode(translateToAidlObj.getAidlStatusCode(pStatus.status));
        } catch(RemoteException | RuntimeException e) {
           Log.w(logTAG, "Unexpected remote exception", e);
           throw e;
        }
        return retStatus;
    }

    private abstract class HalVersionWrapper {
        public void setServiceHandle (int serviceHandle) {

        }

        public UceStatus addListener(int pPresServiceHdl,
           PresListener hidlPresListener, long clientHandle) throws RemoteException {
               return null;
        }
        public UceStatus removeListener(int pPresServiceHdl,
            long clientHandle) throws RemoteException {
                return null;
        }

        public UceStatus reenableService(int pPresServiceHdl,
            int userData) throws RemoteException {
                return null;
        }

        public UceStatus publishMyCap(int pPresServiceHdl,
            PresCapInfo  pMyCapInfo, int pUserData)throws RemoteException {
            return null;
        }

        public UceStatus getContactCap(int pPresServiceHdl,
            String pRemoteURI, int pUserData) throws RemoteException {
                return null;
        }

        public UceStatus getContactListCap(int pPresServiceHdl,
            ArrayList<String> pRemoteURIList, int pUserData)
            throws RemoteException {
                return null;
        }

        public UceStatus setNewFeatureTag(int pPresServiceHdl, RcsFeatureTag pFeatureTag,
            PresServiceInfo pServiceInfo, int pUserData)
            throws RemoteException {
                return null;
        }
    };

    private class Version2_0 extends HalVersionWrapper {
        private Map<Integer, com.qualcomm.qti.uceservice.V2_0.IPresenceService> presHandleMap =
                            new HashMap<Integer, com.qualcomm.qti.uceservice.V2_0.IPresenceService>();
        public void setServiceHandle (int serviceHandle) {
            com.qualcomm.qti.uceservice.V2_0.IPresenceService localHidlPresenceService = null;
            try {
                localHidlPresenceService = RCSService.mHidlService.getPresenceService(serviceHandle);
            }catch(RemoteException | RuntimeException e) {
                Log.w(logTAG, "Unexpected remote exception", e);
            }
            if(localHidlPresenceService != null) {
                presHandleMap.put(serviceHandle, localHidlPresenceService);
            }
        }

        public UceStatus addListener(int pPresServiceHdl,
           PresListener hidlPresListener, long clientHandle) throws RemoteException {
            UceStatus pStatus;
            try {
            pStatus = presHandleMap.get(pPresServiceHdl)
            .addListener((long)pPresServiceHdl,
                            hidlPresListener, clientHandle);
            } catch (RemoteException | RuntimeException e) {
                throw e;
            }
            return pStatus;
        }

        public UceStatus removeListener(int pPresServiceHdl,
             long clientHandle) throws RemoteException {
            UceStatus pStatus;
            try {
              pStatus = presHandleMap.get(pPresServiceHdl)
                       .removeListener(pPresServiceHdl, clientHandle);
            } catch (RemoteException | RuntimeException e) {
                throw e;
            }
            return pStatus;
        }

        public UceStatus reenableService(int pPresServiceHdl,
             int pUserData) throws RemoteException {
            UceStatus pStatus;
            try {
                pStatus = presHandleMap.get(pPresServiceHdl).reEnableService(pPresServiceHdl, pUserData);
            } catch (RemoteException | RuntimeException e) {
                throw e;
            }
            return pStatus;
        }

        public UceStatus publishMyCap(int pPresServiceHdl,
            PresCapInfo  pMyCapInfo, int pUserData)throws RemoteException {
            UceStatus pStatus;
            try {
                com.qualcomm.qti.uceservice.V2_0.PresenceCapabilityInfo pHidlCapInfo =
                    new com.qualcomm.qti.uceservice.V2_0.PresenceCapabilityInfo();
                pHidlCapInfo = translateToHidlObj.getHidlPresCapInfo(pMyCapInfo);

                pStatus = presHandleMap.get(pPresServiceHdl)
                    .publishCapability(pPresServiceHdl,
                                    pHidlCapInfo, pUserData);
            } catch (RemoteException | RuntimeException e) {
                throw e;
            }
            return pStatus;

        }

        public UceStatus getContactCap(int pPresServiceHdl,
            String pRemoteURI, int pUserData) throws RemoteException {
            translateToAidlObj = new HidlToAidlTranslator();
            UceStatus pStatus;
            try {
                pStatus = presHandleMap.get(pPresServiceHdl)
                                    .getContactCapability(pPresServiceHdl,
                                                        pRemoteURI, pUserData);
            } catch (RemoteException | RuntimeException e) {
                throw e;
            }
            return pStatus;
        }

        public UceStatus getContactListCap(int pPresServiceHdl,
            ArrayList<String> stringRemoteURIList, int pUserData)
            throws RemoteException {
            UceStatus pStatus;
            try {
                pStatus = presHandleMap.get(pPresServiceHdl)
                                .getContactListCapability((long)pPresServiceHdl,
                                                           stringRemoteURIList,
                                                           (long) pUserData);
            } catch (RemoteException | RuntimeException e) {
                throw e;
            }
            return pStatus;
        }

        public UceStatus setNewFeatureTag(int pPresServiceHdl, RcsFeatureTag hidlRcsFeatureTag,
            PresServiceInfo hidlPresSvcInfo, int pUserData)
                throws RemoteException {
            UceStatus pStatus;
            try {
                pStatus = presHandleMap.get(pPresServiceHdl)
                            .setNewFeatureTag((long)pPresServiceHdl, hidlRcsFeatureTag,
                                hidlPresSvcInfo, (long)pUserData);
            } catch (RemoteException | RuntimeException e) {
                throw e;
            }
            return pStatus;
        }

    };

    private class Version2_1 extends HalVersionWrapper {
        private Map<Integer, com.qualcomm.qti.uceservice.V2_1.IPresenceService> presHandleMap =
                            new HashMap<Integer, com.qualcomm.qti.uceservice.V2_1.IPresenceService>();

        public void setServiceHandle (int serviceHandle) {
            com.qualcomm.qti.uceservice.V2_1.IPresenceService localHidlPresenceService = null;
            try {
                localHidlPresenceService = RCSService.mHidlService_2_1.getPresenceService_2_1(serviceHandle);
            }catch(RemoteException | RuntimeException e) {
                Log.w(logTAG, "Unexpected remote exception", e);
            }
            if(localHidlPresenceService != null) {
                presHandleMap.put(serviceHandle, localHidlPresenceService);
            }
        }

        public UceStatus addListener(int pPresServiceHdl,
           PresListener hidlPresListener, long clientHandle) throws RemoteException {
            UceStatus pStatus;
               try {
                pStatus = presHandleMap.get(pPresServiceHdl)
                                   .addListener_2_1((long)pPresServiceHdl,
                                                 hidlPresListener, clientHandle);
               }catch (RemoteException | RuntimeException e) {
                   throw e;
               }
               return pStatus;
        }

        public UceStatus removeListener(int pPresServiceHdl,
             long clientHandle) throws RemoteException {
            UceStatus pStatus;
            try {
              pStatus = presHandleMap.get(pPresServiceHdl)
                .removeListener(pPresServiceHdl, clientHandle);
            } catch (RemoteException | RuntimeException e) {
                throw e;
            }
            return pStatus;
        }

        public UceStatus reenableService(int pPresServiceHdl,
             int pUserData) throws RemoteException {
            UceStatus pStatus;
            try {
                pStatus = presHandleMap.get(pPresServiceHdl).reEnableService(pPresServiceHdl, pUserData);
            } catch (RemoteException | RuntimeException e) {
                throw e;
            }
            return pStatus;
        }

        public UceStatus publishMyCap(int pPresServiceHdl,
            PresCapInfo  pMyCapInfo, int pUserData)throws RemoteException {
            UceStatus pStatus;
            try {
                com.qualcomm.qti.uceservice.V2_1.PresenceCapabilityInfo pHidlCapInfo =
                    new com.qualcomm.qti.uceservice.V2_1.PresenceCapabilityInfo();
                pHidlCapInfo = translateToHidlObj.getHidlPresCapInfo_2_1(pMyCapInfo);

                pStatus = presHandleMap.get(pPresServiceHdl)
                    .publishCapability_2_1(pPresServiceHdl,
                                    pHidlCapInfo, pUserData);
            } catch (RemoteException | RuntimeException e) {
                throw e;
            }
            return pStatus;

        }

        public UceStatus getContactCap(int pPresServiceHdl,
            String pRemoteURI, int pUserData) throws RemoteException {
            translateToAidlObj = new HidlToAidlTranslator();
            UceStatus pStatus;
            try {
                pStatus = presHandleMap.get(pPresServiceHdl)
                                    .getContactCapability(pPresServiceHdl,
                                                        pRemoteURI, pUserData);
            } catch (RemoteException | RuntimeException e) {
                throw e;
            }
            return pStatus;
        }

        public UceStatus getContactListCap(int pPresServiceHdl,
            ArrayList<String> stringRemoteURIList, int pUserData)
            throws RemoteException {
            translateToAidlObj = new HidlToAidlTranslator();
            UceStatus pStatus;
            try {
                pStatus = presHandleMap.get(pPresServiceHdl)
                                .getContactListCapability((long)pPresServiceHdl,
                                                           stringRemoteURIList,
                                                           (long) pUserData);
            } catch (RemoteException | RuntimeException e) {
                throw e;
            }
            return pStatus;
        }

        public UceStatus setNewFeatureTag(int pPresServiceHdl, RcsFeatureTag hidlRcsFeatureTag,
            PresServiceInfo hidlPresSvcInfo, int pUserData)
                throws RemoteException {
            UceStatus pStatus;
            try {
                pStatus = presHandleMap.get(pPresServiceHdl)
                            .setNewFeatureTag((long)pPresServiceHdl, hidlRcsFeatureTag,
                                hidlPresSvcInfo, (long)pUserData);
            } catch (RemoteException | RuntimeException e) {
                throw e;
            }
            return pStatus;
        }
    };

    private class Version2_2 extends HalVersionWrapper {
        private Map<Integer, com.qualcomm.qti.uceservice.V2_2.IPresenceService> presHandleMap =
                            new HashMap<Integer, com.qualcomm.qti.uceservice.V2_2.IPresenceService>();

        public void setServiceHandle (int serviceHandle) {
            com.qualcomm.qti.uceservice.V2_2.IPresenceService localHidlPresenceService = null;
            try {
                localHidlPresenceService = RCSService.mHidlService_2_2.getPresenceService_2_2(serviceHandle);
            }catch(RemoteException | RuntimeException e) {
                Log.w(logTAG, "Unexpected remote exception", e);
            }
            if(localHidlPresenceService != null) {
                presHandleMap.put(serviceHandle, localHidlPresenceService);
            }
        }

        public UceStatus addListener(int pPresServiceHdl,
           PresListener hidlPresListener, long clientHandle)throws RemoteException {
            UceStatus pStatus = null;
               try {
                pStatus = presHandleMap.get(pPresServiceHdl)
                    .addListener_2_1((long)pPresServiceHdl,
                              hidlPresListener, clientHandle);
               } catch (RemoteException|RuntimeException e) {
                   throw e;
               }
               return pStatus;
        }
        public UceStatus removeListener(int pPresServiceHdl,
             long clientHandle) throws RemoteException {
            UceStatus pStatus;
            try {
              pStatus = presHandleMap.get(pPresServiceHdl)
                    .removeListener(pPresServiceHdl, clientHandle);
            } catch (RemoteException | RuntimeException e) {
                throw e;
            }
            return pStatus;
        }

        public UceStatus reenableService(int pPresServiceHdl,
             int pUserData) throws RemoteException {
            UceStatus pStatus;
            try {
                pStatus = presHandleMap.get(pPresServiceHdl).reEnableService(pPresServiceHdl, pUserData);
            } catch (RemoteException | RuntimeException e) {
                throw e;
            }
            return pStatus;
        }
        public UceStatus publishMyCap(int pPresServiceHdl,
            PresCapInfo  pMyCapInfo, int pUserData)throws RemoteException {
            UceStatus pStatus;
            try {
                com.qualcomm.qti.uceservice.V2_2.PresenceCapabilityInfo pHidlCapInfo =
                    new com.qualcomm.qti.uceservice.V2_2.PresenceCapabilityInfo();
                pHidlCapInfo = translateToHidlObj.getHidlPresCapInfo_2_2(pMyCapInfo);

                pStatus = presHandleMap.get(pPresServiceHdl)
                    .publishCapability_2_2(pPresServiceHdl,
                                    pHidlCapInfo, pUserData);
            } catch (RemoteException | RuntimeException e) {
                throw e;
            }
            return pStatus;
        }

        public UceStatus getContactCap(int pPresServiceHdl,
            String pRemoteURI, int pUserData) throws RemoteException {
            UceStatus pStatus;
            try {
                pStatus = presHandleMap.get(pPresServiceHdl)
                                    .getContactCapability(pPresServiceHdl,
                                                        pRemoteURI, pUserData);
            } catch (RemoteException | RuntimeException e) {
                throw e;
            }
            return pStatus;
        }

        public UceStatus getContactListCap(int pPresServiceHdl,
            ArrayList<String> stringRemoteURIList, int pUserData)
            throws RemoteException {
            translateToAidlObj = new HidlToAidlTranslator();
            UceStatus pStatus;
            try {
                pStatus = presHandleMap.get(pPresServiceHdl)
                                .getContactListCapability((long)pPresServiceHdl,
                                                           stringRemoteURIList,
                                                           (long) pUserData);
            } catch (RemoteException | RuntimeException e) {
                throw e;
            }
            return pStatus;
        }

        public UceStatus setNewFeatureTag(int pPresServiceHdl, RcsFeatureTag hidlRcsFeatureTag,
            PresServiceInfo hidlPresSvcInfo, int pUserData)
                throws RemoteException {
            UceStatus pStatus;
            try {
                pStatus = presHandleMap.get(pPresServiceHdl)
                            .setNewFeatureTag((long)pPresServiceHdl, hidlRcsFeatureTag,
                                hidlPresSvcInfo, (long)pUserData);
            } catch (RemoteException | RuntimeException e) {
                throw e;
            }
            return pStatus;
        }
    }
}
