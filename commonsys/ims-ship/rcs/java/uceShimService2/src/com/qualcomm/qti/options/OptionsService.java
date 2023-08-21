/*********************************************************************
 Copyright (c) 2017,2019 Qualcomm Technologies, Inc.
 All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
**********************************************************************/

package com.qualcomm.qti.options;

import com.android.ims.internal.uce.common.CapInfo;
import com.android.ims.internal.uce.common.StatusCode;
import com.android.ims.internal.uce.common.UceLong;
import com.android.ims.internal.uce.options.OptionsCapInfo;

import com.qualcomm.qti.uceservice.V2_0.CapabilityInfo;
import com.qualcomm.qti.uceservice.V2_0.OptionsCapabilityInfo;
import com.qualcomm.qti.uceservice.V2_0.UceStatusCode;
import com.qualcomm.qti.uceservice.V2_0.UceStatus;

import com.qualcomm.qti.uceShimService.RCSService;

import com.qualcomm.qti.translator.*;

import android.os.RemoteException;
import android.util.Log;
import android.os.Parcel;

import java.util.Arrays;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;
import java.util.List;

public class OptionsService extends com.android.ims.internal.uce.options.IOptionsService.Stub
{
    private final static String logTAG = "RCSService Options-Service";
    private AidlToHidlTranslator translateToHidlObj = null;
    private HidlToAidlTranslator translateToAidlObj = null;
    private HalVersionWrapper mHalWrapper = null;

    // maintatin list of HIDL Listeners
    private static List<OptionsListener> hidlListenerlist = new ArrayList<OptionsListener>();

    public OptionsService() {
        if(RCSService.mHidlService_2_2 != null) {
            mHalWrapper = new  Version2_2();
        } else if (RCSService.mHidlService_2_1 != null) {
            mHalWrapper = new  Version2_1();
        } else {
            mHalWrapper = new  Version2_0();
        }
    }
    public void setServiceHandle (int serviceHandle) {
        mHalWrapper.setServiceHandle(serviceHandle);
    }

    public StatusCode getVersion (int optionsServiceHandle) throws RemoteException {
        StatusCode retStatus = new StatusCode();
        retStatus.setStatusCode(StatusCode.UCE_NOT_SUPPORTED);
        return retStatus;
    }

    public void addHidlListener(OptionsListener hidlListener) {
        hidlListenerlist.add(hidlListener);
    }

    public StatusCode addListener(int optionsServiceHandle,
                                  com.android.ims.internal.uce.options.IOptionsListener optionsListener,
                                  UceLong optionsListenerHdl) throws RemoteException {

        StatusCode retStatus = new StatusCode();
        retStatus.setStatusCode(StatusCode.UCE_FAILURE);
        translateToAidlObj = new HidlToAidlTranslator();

        long clientHandle = optionsListenerHdl.getUceLong();
        OptionsListener hidlOptionsListener = new OptionsListener();
        addHidlListener(hidlOptionsListener);
        hidlOptionsListener.setAidlOptionsServiceListener(optionsListener);
        try {
            UceStatus pStatus = mHalWrapper.addListener(
                    optionsServiceHandle, hidlOptionsListener, clientHandle);
            retStatus.setStatusCode(translateToAidlObj.getAidlStatusCode(pStatus.status));
        }
        catch (RemoteException | RuntimeException e) {
            Log.d(logTAG, "Unexpected remote exception", e);
            throw e;
        }
        return retStatus;
    }

    public StatusCode removeListener(int optionsServiceHandle,
            UceLong optionsServiceUserData) throws RemoteException {
        StatusCode retStatus = new StatusCode();
        retStatus.setStatusCode(StatusCode.UCE_FAILURE);
        translateToAidlObj = new HidlToAidlTranslator();

        long optionsServiceListenerHdl = optionsServiceUserData.getUceLong();
        try {
            UceStatus pStatus = mHalWrapper.removeListener(
                    optionsServiceHandle, optionsServiceListenerHdl);
            retStatus.setStatusCode(translateToAidlObj.getAidlStatusCode(pStatus.status));
        }
        catch (RemoteException | RuntimeException e) {
            Log.d(logTAG, "Unexpected remote exception", e);
            throw e;
        }
        return retStatus;
    }

    public StatusCode getMyInfo(int optionsServiceHandle, int reqUserData)
            throws RemoteException {
        StatusCode retStatus = new StatusCode();
        retStatus.setStatusCode(StatusCode.UCE_FAILURE);
        translateToAidlObj = new HidlToAidlTranslator();

        try {
            UceStatus pStatus = mHalWrapper.getMyInfo(optionsServiceHandle, reqUserData);
            retStatus.setStatusCode(translateToAidlObj.getAidlStatusCode(pStatus.status));
        }
        catch (RemoteException | RuntimeException e) {
            Log.d(logTAG, "Unexpected remote exception", e);
            throw e;
        }
        return retStatus;
    }

    public StatusCode getContactCap(int optionsServiceHandle, String remoteURI,
                                    int reqUserData) throws RemoteException {
        StatusCode retStatus = new StatusCode();
        retStatus.setStatusCode(StatusCode.UCE_FAILURE);
        translateToAidlObj = new HidlToAidlTranslator();

        try {
            UceStatus pStatus = mHalWrapper
                .getContactCap(optionsServiceHandle,remoteURI, reqUserData);
            retStatus.setStatusCode(translateToAidlObj.getAidlStatusCode(pStatus.status));
        }
        catch (RemoteException | RuntimeException e) {
            Log.d(logTAG, "Unexpected remote exception", e);
            throw e;
        }

        return retStatus;
    }

    public StatusCode getContactListCap(int optionsServiceHandle,
                                        String[] remoteURIList, int reqUserData) throws RemoteException {
        StatusCode retStatus = new StatusCode();
        retStatus.setStatusCode(StatusCode.UCE_FAILURE);
        translateToAidlObj = new HidlToAidlTranslator();

        ArrayList<String> hidlRemoteURIList = new ArrayList<String>(Arrays.asList(remoteURIList));
        try {
            UceStatus pStatus = mHalWrapper
                .getContactListCap(optionsServiceHandle,hidlRemoteURIList, reqUserData);
            retStatus.setStatusCode(translateToAidlObj.getAidlStatusCode(pStatus.status));
        }
        catch (RemoteException | RuntimeException e) {
            Log.d(logTAG, "Unexpected remote exception", e);
            throw e;
        }
        return retStatus;
    }

    public StatusCode responseIncomingOptions(int optionsServiceHandle,  int tId,
                   int sipResponseCode, String reasonPhrase, OptionsCapInfo capInfo,
                   boolean bContactInBL) throws RemoteException {

        StatusCode retStatus = new StatusCode();
        retStatus.setStatusCode(StatusCode.UCE_SERVICE_UNKNOWN);
        translateToAidlObj = new HidlToAidlTranslator();
        translateToHidlObj = new AidlToHidlTranslator();

        byte contactInBLByte = (byte)(bContactInBL?1:0);
        try {
            UceStatus pStatus = mHalWrapper.responseIncomingOptions(
                optionsServiceHandle, tId, (short)sipResponseCode, reasonPhrase,
                capInfo, contactInBLByte);
            retStatus.setStatusCode(translateToAidlObj.getAidlStatusCode(pStatus.status));
        }
        catch (RemoteException | RuntimeException e) {
            Log.d(logTAG, "Unexpected remote exception", e);
            throw e;
        }
        return retStatus;
    }

    public StatusCode setMyInfo(int optionsServiceHandle, CapInfo capInfo, int reqUserData)
            throws RemoteException {

        StatusCode retStatus = new StatusCode();
        retStatus.setStatusCode(StatusCode.UCE_SERVICE_UNKNOWN);
        translateToAidlObj = new HidlToAidlTranslator();
        translateToHidlObj = new AidlToHidlTranslator();

        try {
            UceStatus pStatus = mHalWrapper.setMyInfo(optionsServiceHandle,
                                capInfo,
                                reqUserData);
            retStatus.setStatusCode(translateToAidlObj.getAidlStatusCode(pStatus.status));
        }
        catch (RemoteException | RuntimeException e) {
            Log.d(logTAG, "Unexpected remote exception", e);
            throw e;
        }

        return retStatus;
    }

    private abstract class HalVersionWrapper {

        public void setServiceHandle (int serviceHandle) {
        }

        public UceStatus addListener(int optionsServiceHandle,
                                  OptionsListener optionsListener,
                                  long clientHandle) throws RemoteException {
            return null;
        }
        public UceStatus removeListener(int optionsServiceHandle,
            long optionsServiceUserData) throws RemoteException {
            return null;
        }
        public UceStatus getMyInfo(int optionsServiceHandle, int reqUserData)
            throws RemoteException {
            return null;
        }

        public UceStatus getContactCap(int optionsServiceHandle, String remoteURI,
                                    int reqUserData) throws RemoteException {
            return null;
        }

        public UceStatus getContactListCap(int optionsServiceHandle,
            ArrayList<String> hidlRemoteURIList, int reqUserData) throws RemoteException {
            return null;
        }

        public UceStatus responseIncomingOptions(int optionsServiceHandle,  int tId,
                   int sipResponseCode, String reasonPhrase, OptionsCapInfo capInfo,
                   byte contactInBLByte) throws RemoteException {
            return null;
        }

        public UceStatus setMyInfo(int optionsServiceHandle, CapInfo capInfo, int reqUserData)
            throws RemoteException {
            return null;
        }
    };

    private class Version2_0 extends HalVersionWrapper {
        private Map<Integer, com.qualcomm.qti.uceservice.V2_0.IOptionsService> optionsHandleMap =
                          new HashMap<Integer, com.qualcomm.qti.uceservice.V2_0.IOptionsService>();
        public void setServiceHandle (int serviceHandle) {
            com.qualcomm.qti.uceservice.V2_0.IOptionsService localHidlOptionsService = null;
            try {
                localHidlOptionsService = RCSService.mHidlService.getOptionsService(serviceHandle);
            }catch(RemoteException | RuntimeException e) {
               Log.w(logTAG, "Unexpected remote exception", e);
            }
            if(localHidlOptionsService != null) {
                optionsHandleMap.put(serviceHandle, localHidlOptionsService);
            }
        }

        public UceStatus addListener(int optionsServiceHandle,
                                  OptionsListener optionsListener,
                                  long clientHandle) throws RemoteException {
            UceStatus pStatus = null;
            try {
                pStatus = optionsHandleMap.get(optionsServiceHandle)
                    .addListener((long)optionsServiceHandle, optionsListener, clientHandle);
            }
            catch (RemoteException | RuntimeException e) {
                Log.d(logTAG, "Unexpected remote exception", e);
                throw e;
            }
            return pStatus;
        }

        public UceStatus removeListener(int optionsServiceHandle,
            long optionsServiceUserData) throws RemoteException {
            UceStatus pStatus = null;
            try {
                pStatus = optionsHandleMap.get(optionsServiceHandle)
                        .removeListener(optionsServiceHandle, optionsServiceUserData);
            }
            catch (RemoteException | RuntimeException e) {
                Log.d(logTAG, "Unexpected remote exception", e);
                throw e;
            }
            return pStatus;
        }

        public UceStatus getMyInfo(int optionsServiceHandle, int reqUserData)
            throws RemoteException {
            UceStatus pStatus = null;
            try {
                pStatus = optionsHandleMap.get(optionsServiceHandle)
                        .getCapabilityInfo((long)optionsServiceHandle, (long)reqUserData);
            }
            catch (RemoteException | RuntimeException e) {
                Log.d(logTAG, "Unexpected remote exception", e);
                throw e;
            }
            return pStatus;
        }

        public UceStatus getContactCap(int optionsServiceHandle, String remoteURI,
                                    int reqUserData) throws RemoteException {
            UceStatus pStatus = null;
            try {

                pStatus = optionsHandleMap.get(optionsServiceHandle)
                        .getContactCapability((long)optionsServiceHandle, remoteURI, (long)reqUserData);
            }
            catch (RemoteException | RuntimeException e) {
                Log.d(logTAG, "Unexpected remote exception", e);
                throw e;
            }
            return pStatus;
        }

        public UceStatus getContactListCap(int optionsServiceHandle,
            ArrayList<String> hidlRemoteURIList, int reqUserData) throws RemoteException {
            UceStatus pStatus = null;
            try {
                pStatus =  optionsHandleMap.get(optionsServiceHandle)
                        .getContactListCapability((long)optionsServiceHandle,
                                                    hidlRemoteURIList, (long)reqUserData);
            }
            catch (RemoteException | RuntimeException e) {
                Log.d(logTAG, "Unexpected remote exception", e);
                throw e;
            }
            return pStatus;
        }

        public UceStatus responseIncomingOptions(int optionsServiceHandle,  int tId,
                   int sipResponseCode, String reasonPhrase, OptionsCapInfo capInfo,
                   byte contactInBLByte) throws RemoteException {
            UceStatus pStatus;
            try {
                com.qualcomm.qti.uceservice.V2_0.OptionsCapabilityInfo hidlCapInfo =
                            new com.qualcomm.qti.uceservice.V2_0.OptionsCapabilityInfo();
                hidlCapInfo = translateToHidlObj.getHidlOptCapInfo(capInfo);
                pStatus = optionsHandleMap.get(optionsServiceHandle)
                                    .responseIncomingOptions((long)optionsServiceHandle,
                                            tId, (short)sipResponseCode, reasonPhrase,
                                            hidlCapInfo, contactInBLByte);
            }
            catch (RemoteException | RuntimeException e) {
                Log.d(logTAG, "Unexpected remote exception", e);
                throw e;
            }
            return pStatus;
        }

        public UceStatus setMyInfo(int optionsServiceHandle, CapInfo capInfo, int reqUserData)
            throws RemoteException {
            UceStatus pStatus;
            try {
                com.qualcomm.qti.uceservice.V2_0.CapabilityInfo hidlCapInfo
                    = new com.qualcomm.qti.uceservice.V2_0.CapabilityInfo();
                translateToHidlObj.getHidlCapInfo(hidlCapInfo, capInfo);
                pStatus = optionsHandleMap.get(optionsServiceHandle)
                                            .setCapabilityInfo((long)optionsServiceHandle,
                                                                hidlCapInfo,
                                                                (long)reqUserData);
            } catch (RemoteException | RuntimeException e) {
                Log.d(logTAG, "Unexpected remote exception", e);
                throw e;
            }
            return pStatus;
        }
    };

    private class Version2_1 extends HalVersionWrapper {
        private Map<Integer, com.qualcomm.qti.uceservice.V2_1.IOptionsService> optionsHandleMap =
                          new HashMap<Integer, com.qualcomm.qti.uceservice.V2_1.IOptionsService>();

        public void setServiceHandle (int serviceHandle) {
            com.qualcomm.qti.uceservice.V2_1.IOptionsService localHidlOptionsService = null;
            try {
                localHidlOptionsService = RCSService.mHidlService_2_1.getOptionsService_2_1(serviceHandle);
            }catch(RemoteException | RuntimeException e) {
               Log.w(logTAG, "Unexpected remote exception", e);
            }
            if(localHidlOptionsService != null) {
                optionsHandleMap.put(serviceHandle, localHidlOptionsService);
            }
        }

        public UceStatus addListener(int optionsServiceHandle,
                                  OptionsListener optionsListener,
                                  long clientHandle) throws RemoteException {
            UceStatus pStatus = null;
            try {
                pStatus = optionsHandleMap.get(optionsServiceHandle)
                    .addListener_2_1((long)optionsServiceHandle, optionsListener, clientHandle);
            }
            catch (RemoteException | RuntimeException e) {
                Log.d(logTAG, "Unexpected remote exception", e);
                throw e;
            }
            return pStatus;
        }

        public UceStatus removeListener(int optionsServiceHandle,
            long optionsServiceUserData) throws RemoteException {
            UceStatus pStatus = null;
            try {
                pStatus = optionsHandleMap.get(optionsServiceHandle)
                        .removeListener(optionsServiceHandle, optionsServiceUserData);
            }
            catch (RemoteException | RuntimeException e) {
                Log.d(logTAG, "Unexpected remote exception", e);
                throw e;
            }
            return pStatus;
        }

        public UceStatus getMyInfo(int optionsServiceHandle, int reqUserData)
            throws RemoteException {
            UceStatus pStatus = null;
            try {
                pStatus = optionsHandleMap.get(optionsServiceHandle)
                        .getCapabilityInfo((long)optionsServiceHandle, (long)reqUserData);
            }
            catch (RemoteException | RuntimeException e) {
                Log.d(logTAG, "Unexpected remote exception", e);
                throw e;
            }
            return pStatus;
        }

        public UceStatus getContactCap(int optionsServiceHandle, String remoteURI,
                                    int reqUserData) throws RemoteException {
            UceStatus pStatus = null;
            try {

                pStatus = optionsHandleMap.get(optionsServiceHandle)
                        .getContactCapability((long)optionsServiceHandle, remoteURI, (long)reqUserData);
            }
            catch (RemoteException | RuntimeException e) {
                Log.d(logTAG, "Unexpected remote exception", e);
                throw e;
            }
            return pStatus;
        }

        public UceStatus getContactListCap(int optionsServiceHandle,
            ArrayList<String> hidlRemoteURIList, int reqUserData) throws RemoteException {
            UceStatus pStatus = null;
            try {
                pStatus =  optionsHandleMap.get(optionsServiceHandle)
                        .getContactListCapability((long)optionsServiceHandle,
                                                    hidlRemoteURIList, (long)reqUserData);
            }
            catch (RemoteException | RuntimeException e) {
                Log.d(logTAG, "Unexpected remote exception", e);
                throw e;
            }
            return pStatus;
        }

        public UceStatus responseIncomingOptions(int optionsServiceHandle,  int tId,
                   int sipResponseCode, String reasonPhrase, OptionsCapInfo capInfo,
                   byte contactInBLByte) throws RemoteException {
            UceStatus pStatus;
            try {
                com.qualcomm.qti.uceservice.V2_1.OptionsCapabilityInfo hidlCapInfo =
                            new com.qualcomm.qti.uceservice.V2_1.OptionsCapabilityInfo();
                hidlCapInfo = translateToHidlObj.getHidlOptCapInfo_2_1(capInfo);
                pStatus = optionsHandleMap.get(optionsServiceHandle)
                                    .responseIncomingOptions_2_1((long)optionsServiceHandle,
                                            tId, (short)sipResponseCode, reasonPhrase,
                                            hidlCapInfo, contactInBLByte);
            }
            catch (RemoteException | RuntimeException e) {
                Log.d(logTAG, "Unexpected remote exception", e);
                throw e;
            }
            return pStatus;
        }

        public UceStatus setMyInfo(int optionsServiceHandle, CapInfo capInfo, int reqUserData)
            throws RemoteException {
            UceStatus pStatus;
            try {
                com.qualcomm.qti.uceservice.V2_1.CapabilityInfo hidlCapInfo
                    = new com.qualcomm.qti.uceservice.V2_1.CapabilityInfo();
                translateToHidlObj.getHidlCapInfo_2_1(hidlCapInfo, capInfo);
                pStatus = optionsHandleMap.get(optionsServiceHandle)
                                            .setCapabilityInfo_2_1((long)optionsServiceHandle,
                                                                hidlCapInfo,
                                                                (long)reqUserData);
            } catch (RemoteException | RuntimeException e) {
                Log.d(logTAG, "Unexpected remote exception", e);
                throw e;
            }
            return pStatus;
        }
    };

    private class Version2_2 extends HalVersionWrapper {
        private Map<Integer, com.qualcomm.qti.uceservice.V2_2.IOptionsService> optionsHandleMap =
                          new HashMap<Integer, com.qualcomm.qti.uceservice.V2_2.IOptionsService>();

        public void setServiceHandle (int serviceHandle) {
            com.qualcomm.qti.uceservice.V2_2.IOptionsService localHidlOptionsService = null;
            try {
                localHidlOptionsService = RCSService.mHidlService_2_2.getOptionsService_2_2(serviceHandle);
            }catch(RemoteException | RuntimeException e) {
               Log.w(logTAG, "Unexpected remote exception", e);
            }
            if(localHidlOptionsService != null) {
                optionsHandleMap.put(serviceHandle, localHidlOptionsService);
            }
        }

        public UceStatus addListener(int optionsServiceHandle,
                                  OptionsListener optionsListener,
                                  long clientHandle) throws RemoteException {
            UceStatus pStatus = null;
            try {
                pStatus = optionsHandleMap.get(optionsServiceHandle)
                    .addListener_2_2((long)optionsServiceHandle, optionsListener, clientHandle);
            }
            catch (RemoteException | RuntimeException e) {
                Log.d(logTAG, "Unexpected remote exception", e);
                throw e;
            }
            return pStatus;
        }

        public UceStatus removeListener(int optionsServiceHandle,
            long optionsServiceUserData) throws RemoteException {
            UceStatus pStatus = null;
            try {
                pStatus = optionsHandleMap.get(optionsServiceHandle)
                        .removeListener(optionsServiceHandle, optionsServiceUserData);
            }
            catch (RemoteException | RuntimeException e) {
                Log.d(logTAG, "Unexpected remote exception", e);
                throw e;
            }
            return pStatus;
        }

        public UceStatus getMyInfo(int optionsServiceHandle, int reqUserData)
            throws RemoteException {
            UceStatus pStatus = null;
            try {
                pStatus = optionsHandleMap.get(optionsServiceHandle)
                        .getCapabilityInfo((long)optionsServiceHandle, (long)reqUserData);
            }
            catch (RemoteException | RuntimeException e) {
                Log.d(logTAG, "Unexpected remote exception", e);
                throw e;
            }
            return pStatus;
        }

        public UceStatus getContactCap(int optionsServiceHandle, String remoteURI,
                                    int reqUserData) throws RemoteException {
            UceStatus pStatus = null;
            try {

                pStatus = optionsHandleMap.get(optionsServiceHandle)
                        .getContactCapability((long)optionsServiceHandle, remoteURI, (long)reqUserData);
            }
            catch (RemoteException | RuntimeException e) {
                Log.d(logTAG, "Unexpected remote exception", e);
                throw e;
            }
            return pStatus;
        }

        public UceStatus getContactListCap(int optionsServiceHandle,
            ArrayList<String> hidlRemoteURIList, int reqUserData) throws RemoteException {
            UceStatus pStatus = null;
            try {
                pStatus =  optionsHandleMap.get(optionsServiceHandle)
                        .getContactListCapability((long)optionsServiceHandle,
                                                    hidlRemoteURIList, (long)reqUserData);
            }
            catch (RemoteException | RuntimeException e) {
                Log.d(logTAG, "Unexpected remote exception", e);
                throw e;
            }
            return pStatus;
        }

        public UceStatus responseIncomingOptions(int optionsServiceHandle,  int tId,
                   int sipResponseCode, String reasonPhrase, OptionsCapInfo capInfo,
                   byte contactInBLByte) throws RemoteException {
            UceStatus pStatus;
            try {
                com.qualcomm.qti.uceservice.V2_2.OptionsCapabilityInfo hidlCapInfo =
                            new com.qualcomm.qti.uceservice.V2_2.OptionsCapabilityInfo();
                hidlCapInfo = translateToHidlObj.getHidlOptCapInfo_2_2(capInfo);
                pStatus = optionsHandleMap.get(optionsServiceHandle)
                                    .responseIncomingOptions_2_2((long)optionsServiceHandle,
                                            tId, (short)sipResponseCode, reasonPhrase,
                                            hidlCapInfo, contactInBLByte);
            }
            catch (RemoteException | RuntimeException e) {
                Log.d(logTAG, "Unexpected remote exception", e);
                throw e;
            }
            return pStatus;
        }

        public UceStatus setMyInfo(int optionsServiceHandle, CapInfo capInfo, int reqUserData)
            throws RemoteException {
            UceStatus pStatus;
            try {
                com.qualcomm.qti.uceservice.V2_2.CapabilityInfo hidlCapInfo
                    = new com.qualcomm.qti.uceservice.V2_2.CapabilityInfo();
                translateToHidlObj.getHidlCapInfo_2_2(hidlCapInfo, capInfo);
                pStatus = optionsHandleMap.get(optionsServiceHandle)
                                            .setCapabilityInfo_2_2((long)optionsServiceHandle,
                                                                hidlCapInfo,
                                                                (long)reqUserData);
            } catch (RemoteException | RuntimeException e) {
                Log.d(logTAG, "Unexpected remote exception", e);
                throw e;
            }
            return pStatus;
        }
    };
}
