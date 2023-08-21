/**
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file LabelledProbabilitySegmentList.kt
 */

package com.qualcomm.qti.mmca.ssmeditor

import android.os.Parcel
import android.os.Parcelable

class LabelledProbabilitySegment(
    startTime: Long,
    endTime: Long,
    var probability: Int = 0,
    var label: String = ""
): Segment(startTime, endTime), Parcelable {

    constructor(parcel: Parcel) :
        this(parcel.readLong(),
            parcel.readLong(),
            parcel.readInt(),
            parcel.readString() ?: "")

    constructor(segment: LabelledProbabilitySegment) :
        this(segment.startTime, segment.endTime, segment.probability, segment.label)

    override fun canMergeWith(s: Segment) =
        when (s) {
            is LabelledProbabilitySegment -> super.canMergeWith(s) && s.probability == probability
            else -> false
        }

    override fun copy(): LabelledProbabilitySegment {
        return LabelledProbabilitySegment(this)
    }

    override fun toString(): String {
        return "${super.toString()}, prob=$probability, label=$label"
    }

    // Parcelable
    override fun writeToParcel(parcel: Parcel, flags: Int) {
        parcel.apply {
            writeLong(startTime)
            writeLong(endTime)
            writeInt(probability)
            writeString(label)
        }
    }

    override fun describeContents() = 0

    companion object CREATOR : Parcelable.Creator<LabelledProbabilitySegment> {
        override fun createFromParcel(parcel: Parcel): LabelledProbabilitySegment {
            return LabelledProbabilitySegment(parcel)
        }

        override fun newArray(size: Int): Array<LabelledProbabilitySegment?> {
            return arrayOfNulls(size)
        }
    }
}

class LabelledProbabilitySegmentList() : SegmentList<LabelledProbabilitySegment>(), Parcelable {

    private constructor(parcel: Parcel) : this() {
        segmentList = arrayListOf<LabelledProbabilitySegment>().apply {
            parcel.readList(this as List<*>, LabelledProbabilitySegment::class.java.classLoader)
        }
    }

    fun getProbabilityAtTime(time: Long) =
        findSegment(time)?.probability ?: 0

    fun getLabelAtTime(time: Long) =
        findSegment(time)?.label ?: ""

    // Parcelable
    override fun writeToParcel(dest: Parcel?, flags: Int) {
        dest?.writeList(segmentList as List<*>?)
    }

    override fun describeContents(): Int {
        return 0
    }

    companion object CREATOR : Parcelable.Creator<LabelledProbabilitySegmentList> {
        override fun createFromParcel(parcel: Parcel): LabelledProbabilitySegmentList {
            return LabelledProbabilitySegmentList(parcel)
        }

        override fun newArray(size: Int): Array<LabelledProbabilitySegmentList?> {
            return Array(size) { LabelledProbabilitySegmentList() }
        }
    }
}
