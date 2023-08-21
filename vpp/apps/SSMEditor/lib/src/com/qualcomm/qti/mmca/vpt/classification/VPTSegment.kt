/**
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file VPTSegment.kt
 */
package com.qualcomm.qti.mmca.vpt.classification

import android.os.Parcel
import android.os.Parcelable
import com.qualcomm.qti.mmca.ssmeditor.FRCSegment
import com.qualcomm.qti.mmca.ssmeditor.LabelledProbabilitySegment

data class VPTSegment(
    var startTime: Long = 0,
    var endTime: Long = 0,
    var interp: FRCSegment.Interp = FRCSegment.Interp.BYPASS,
    var label: String = "",
    var probability: Int
) : Parcelable
{
    constructor(parcel: Parcel) : this(
        parcel.readLong(),
        parcel.readLong(),
        FRCSegment.Interp.values()[parcel.readInt()],
        parcel.readString() ?: "",
        parcel.readInt())

    fun asFRCSegment() =
        FRCSegment(startTime, endTime, interp)

    fun asLabelledProbabilitySegment() =
        LabelledProbabilitySegment(startTime, endTime, probability, label)

    override fun writeToParcel(parcel: Parcel, flags: Int) {
        parcel.apply {
            writeLong(startTime)
            writeLong(endTime)
            writeInt(interp.ordinal)
            writeString(label)
            writeInt(probability)
        }
    }

    override fun describeContents(): Int {
        return 0
    }

    companion object CREATOR : Parcelable.Creator<VPTSegment> {
        override fun createFromParcel(parcel: Parcel): VPTSegment {
            return VPTSegment(parcel)
        }

        override fun newArray(size: Int): Array<VPTSegment?> {
            return arrayOfNulls(size)
        }
    }
}
