/*
 * Copyright (c) 2018, 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

package com.qualcomm.qti.internal.nrNetworkService.hidl;

import org.codeaurora.internal.DcParam;
import org.codeaurora.internal.SignalStrength;
import org.codeaurora.internal.Status;
import org.codeaurora.internal.Token;

import android.os.RemoteException;

public interface IHidlConnectionInterface {
    public void enable5g(int slotId, Token token) throws RemoteException;

    public void disable5g(int slotId, Token token) throws RemoteException;

    public void enable5gOnly(int slotId, Token token) throws RemoteException;

    public void query5gStatus(int slotId, Token token) throws RemoteException;

    public void queryNrDcParam(int slotId, Token token) throws RemoteException;

    public void queryNrBearerAllocation(int slotId, Token token) throws RemoteException;

    public void queryNrSignalStrength(int slotId, Token token) throws RemoteException;

    public void queryUpperLayerIndInfo(int slotId, Token token) throws RemoteException;

    public void query5gConfigInfo(int slotId, Token token) throws RemoteException;

    public void queryNrIconType(int slotId, Token token) throws RemoteException;

    public void enableEndc(int slotId, boolean enabled, Token token) throws RemoteException;

    public void queryEndcStatus(int slotId, Token token) throws RemoteException;

    public Token generateNextToken();

    public void registerCallback(IHidlConnectionCallback callback);

    public void unRegisterCallback(IHidlConnectionCallback callback);
}
