/* ChipView.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.mmca.view.chip;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.CheckBox;

import com.qualcomm.qti.ssmeditor.R;

public class ChipView extends CheckBox {

    public ChipView(Context context) {
        this(context, null);
    }

    public ChipView(Context context, AttributeSet attrs) {
        super(context, attrs, R.style.VPPPChipStyle);
    }
}
