/**
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file FRCSegmentList.kt
 */
package com.qualcomm.qti.mmca.ssmeditor

import android.media.MediaFormat
import android.os.Bundle
import android.os.Parcel
import android.os.Parcelable
import android.util.Log
import com.qualcomm.qti.mmca.decoder.MediaCodecParams

class FRCSegment: Segment, MediaCodecParams, Parcelable {

    enum class Interp(val mediaCodecString: String) {
        BYPASS("FRC_INTERP_1X"),
        INTERP_1X("FRC_INTERP_1X"),
        INTERP_2X("FRC_INTERP_2X"),
        INTERP_4X("FRC_INTERP_4X"),
    }

    var interpFactor: Interp = Interp.BYPASS

    val isBypass
        get() = interpFactor == Interp.BYPASS

    constructor(sTime: Long, eTime: Long, i: Interp) : super(sTime, eTime) {
        interpFactor = i
    }

    constructor(parcel: Parcel) :
        super(parcel.readLong(), parcel.readLong()) {
        interpFactor = Interp.values()[parcel.readInt()]
    }

    override fun canMergeWith(s: Segment) =
        when (s) {
            is FRCSegment -> super.canMergeWith(s) && s.interpFactor == interpFactor
            else -> false
        }

    override fun copy(): FRCSegment {
        return FRCSegment(startTime, endTime, interpFactor)
    }

    override fun toString(): String {
        return "${super.toString()}, interp=$interpFactor"
    }

    override fun pack(mediaFormat: MediaFormat): MediaFormat {
        return mediaFormat
    }

    override fun pack(bundle: Bundle): Bundle {
        bundle.apply {
            putString(MediaCodecParams.VPP_MODE, "HQV_MODE_MANUAL")
            putInt(MediaCodecParams.VPP_FRC_TS_START, startTime.toInt())
            putInt(MediaCodecParams.VPP_FRC_FRM_CPY_FB, 1)
            putInt(MediaCodecParams.VPP_FRC_FRM_CPY_IN, 0)
            putString(MediaCodecParams.VPP_FRC_MODE, "FRC_MODE_SLOMO")
            putString(MediaCodecParams.VPP_FRC_LVL, "FRC_LEVEL_HIGH")
            putString(MediaCodecParams.VPP_FRC_INTERP, interpFactor.mediaCodecString)
        }
        return bundle
    }

    // Parcelable
    override fun writeToParcel(dest: Parcel, flags: Int) {
        dest.apply {
            writeLong(startTime)
            writeLong(endTime)
            writeInt(interpFactor.ordinal)
        }
    }

    override fun describeContents() = 0

    companion object CREATOR : Parcelable.Creator<FRCSegment> {
        override fun createFromParcel(parcel: Parcel): FRCSegment {
            return FRCSegment(parcel)
        }

        override fun newArray(size: Int): Array<FRCSegment?> {
            return arrayOfNulls(size)
        }
    }
}

class FRCSegmentList() : SegmentList<FRCSegment>(), Parcelable {

    private constructor(parcel: Parcel) : this() {
        segmentList = arrayListOf<FRCSegment>().apply {
            parcel.readList(this as List<*>, FRCSegment::class.java.classLoader)
        }
    }

    override var segments: MutableList<FRCSegment> = mutableListOf()
        get() = run {
            // Returns a list of segments containing no gaps. Any gaps are
            // replaced with BYPASS segments.
            var prevEndTime: Long = 0
            val outputSegments: MutableList<FRCSegment> = mutableListOf()

            fun addBypass(start: Long, end: Long) {
                outputSegments.add(FRCSegment(start, end, FRCSegment.Interp.BYPASS))
            }

            for (segment in segmentList) {
                if (segment.startTime != prevEndTime) {
                    addBypass(prevEndTime, segment.startTime)
                }
                outputSegments.add(segment.copy())
                prevEndTime = segment.endTime
            }

            if (prevEndTime != finalTimestamp) {
                addBypass(prevEndTime, finalTimestamp)
            }

            outputSegments
        }

    fun editSegment(time: Long,
                    newStartTime: Long,
                    newEndTime: Long) {
        synchronized(this) {
            removeSegmentAtTime(time, false)?.apply {
                if (newStartTime != newEndTime) {
                    // If newStartTime == newEndTime, this means the segment should
                    // only be deleted, as a 0 length segment makes no sense.
                    addSegment(FRCSegment(newStartTime, newEndTime, interpFactor), false)
                }

                // Only fire the callback listeners once the entire operation is done,
                // since this operation should look atomic externally.
                notifyListeners()
            } ?: let {
                Log.e(TAG, "editSegment failed, no existing segment w/ time=$time")
            }
        }
    }

    fun getInterpFactorAtTime(time: Long) =
        findSegment(time)?.interpFactor ?: FRCSegment.Interp.BYPASS

    fun isInSlowZone(time: Long) =
        when (findSegment(time)?.interpFactor) {
            FRCSegment.Interp.INTERP_2X,
            FRCSegment.Interp.INTERP_4X -> true
            else -> false
        }

    override fun writeToParcel(dest: Parcel?, flags: Int) {
        dest?.writeList(segmentList as List<*>?)
    }

    override fun describeContents(): Int {
        return 0
    }

    companion object CREATOR : Parcelable.Creator<FRCSegmentList> {
        override fun createFromParcel(parcel: Parcel): FRCSegmentList {
            return FRCSegmentList(parcel)
        }

        override fun newArray(size: Int): Array<FRCSegmentList?> {
            return Array(size) { FRCSegmentList() }
        }
    }
}
