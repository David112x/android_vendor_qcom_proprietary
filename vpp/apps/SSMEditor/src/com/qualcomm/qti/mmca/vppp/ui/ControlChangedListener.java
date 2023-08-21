/* ControlChangedListener.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.mmca.vppp.ui;

/**
 * Interface for callbacks when a control is changed.
 */
public interface ControlChangedListener {

    void onControlChanged(ExtensionHolder data);

}
