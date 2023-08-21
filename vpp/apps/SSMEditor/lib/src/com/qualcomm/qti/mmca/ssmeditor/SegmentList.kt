/**
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file SegmentList.kt
 */
package com.qualcomm.qti.mmca.ssmeditor

import android.util.Log
import java.util.ArrayList
import kotlin.math.min

const val TAG: String = "SegmentList"

open class Segment(var startTime: Long, var endTime: Long) {

    constructor(s: Segment) : this(s.startTime, s.endTime)

    val isValid
        get() = when {
            startTime < 0 -> false
            endTime < 0 -> false
            endTime < startTime -> false
            else -> true
        }

    open fun canMergeWith(s: Segment) : Boolean {
        val left: Segment
        val right: Segment

        if (s.startTime < startTime) {
            left = s
            right = this
        } else {
            left = this
            right = s
        }

        return left.endTime >= right.startTime
    }

    override fun toString(): String {
        return "start=$startTime, end=$endTime"
    }

    open fun copy(): Segment {
        return Segment(this)
    }
}

@Suppress("UNCHECKED_CAST")
open class SegmentList <T : Segment>
{
    interface OnSegmentsChangedListener<T> {
        fun onSegmentsChanged(segments: List<T>)
    }

    private var segmentsChangedListeners = mutableListOf<OnSegmentsChangedListener<T>>()

    protected var segmentList = mutableListOf<T>()

    open var segments = mutableListOf<T>()
        get() = run {
            val outputSegmentList: MutableList<T> = ArrayList()
            segmentList.forEach { outputSegmentList.add(it.copy() as T) }
            outputSegmentList
        }
        protected set

    var finalTimestamp: Long = Long.MAX_VALUE
        set (value) {
            Log.d(TAG, "finalTimestamp set to $value")
            field = value

            printSegments("Segments before setting finalTimestamp")
            var elementsRemoved = segmentList.removeAll { it.startTime >= value }
            var lastTimeChanged = false
            val lastSegment = segmentList.lastOrNull()
            if (lastSegment != null) {
                val prevEndTime = lastSegment.endTime
                lastSegment.endTime = min(prevEndTime, value)
                lastTimeChanged = lastSegment.endTime != prevEndTime
            }
            printSegments("Segments after setting finalTimestamp")
            when (elementsRemoved || lastTimeChanged) {
                true -> notifyListeners()
                else -> Log.d(TAG, "no segments changed after setting timestamp")
            }
        }

    fun addSegments(segments: List<T>) {
        printSegments("Adding ${segments.size} segment(s)")
        for (segment in segments) {
            addSegment(segment, false)
        }
        notifyListeners()
        printSegments("New segments:")
    }

    fun addSegment(segment: T, shouldNotifyListeners: Boolean = true) {
        if (!segment.isValid) {
            return
        }

        val newSegment: T = segment.copy() as T

        Log.d(TAG, "adding [${segment.startTime}, ${segment.endTime}] to existing segments")

        synchronized(this) {
            var newSegmentAdded = false
            val newSegList = mutableListOf<T>()

            fun addNewSegmentIfNotAdded(_segment: T) {
                if (!newSegmentAdded) {
                    Log.d(TAG, "adding new segment, [${_segment.startTime}, ${_segment.endTime}]")
                    newSegList.add(_segment)
                    newSegmentAdded = true
                }
            }

            for (s: T in segmentList) {
                // Add all segments completely on the left of this new segment
                if (s.endTime <= newSegment.startTime) {
                    newSegList.add(s)
                }

                // Add all segments completely on the right of this new segment
                else if (newSegment.endTime <= s.startTime) {
                    // Since we're totally to the right of the new segment
                    // now, if it hasn't already been added, add it now.
                    addNewSegmentIfNotAdded(newSegment)
                    newSegList.add(s)
                }

                // New segment overlaps only the right side of old segment
                else if (s.startTime < newSegment.startTime &&
                    newSegment.startTime < s.endTime &&
                    s.endTime <= newSegment.endTime) {
                    s.endTime = newSegment.startTime
                    newSegList.add(s)
                    addNewSegmentIfNotAdded(newSegment)
                }

                // New segment overlaps the left side of the old segment
                else if (newSegment.startTime <= s.startTime &&
                    s.startTime < newSegment.endTime &&
                    newSegment.endTime < s.endTime) {
                    s.startTime = newSegment.endTime
                    addNewSegmentIfNotAdded(newSegment)
                    newSegList.add(s)
                }

                // New segments completely overlaps the old segment
                else if (newSegment.startTime <= s.startTime &&
                    s.endTime <= newSegment.endTime) {
                    addNewSegmentIfNotAdded(newSegment)
                }

                // New segment bisects an old segment
                else if (s.startTime < newSegment.startTime &&
                    newSegment.endTime < s.endTime) {
                    val rightSegment: T = s.copy() as T
                    s.endTime = newSegment.startTime

                    newSegList.add(s)
                    addNewSegmentIfNotAdded(newSegment)

                    rightSegment.startTime = newSegment.endTime
                    newSegList.add(rightSegment)
                }
            }

            // If there were no segments in the list or all existing
            // segments in the list are before the new segment
            addNewSegmentIfNotAdded(newSegment)
            segmentList = newSegList

            mergeSegments()
            if (shouldNotifyListeners) {
                notifyListeners()
            }
        }
    }

    fun removeSegments() {
        synchronized(this) {
            segmentList.clear()
        }

        notifyListeners()
    }

    fun removeSegmentAtTime(time: Long, shouldNotifyListeners: Boolean = true): T? {
        var removedSegment: T? = null
        synchronized(this) {
            printSegments("removeSegmentAtTime time=$time, current segments:")

            for ((index, segment) in segmentList.withIndex()) {
                if (segment.startTime <= time && time < segment.endTime) {
                    removedSegment = segmentList.removeAt(index)
                    mergeSegments()
                    break
                }
            }

            printSegments("new segments:")
        }

        if (removedSegment != null && shouldNotifyListeners) {
            notifyListeners()
        }

        return removedSegment
    }

    private fun mergeSegments() {
        var i = 0
        while (i < segmentList.size - 1) {

            val currSegment = segmentList[i]
            val nextSegment = segmentList[i + 1]

            when {
                currSegment.canMergeWith(nextSegment) -> {
                    currSegment.endTime = nextSegment.endTime
                    segmentList.removeAt(i + 1)
                }
                else -> i++
            }
        }
    }

    protected fun findSegment(progress: Long) : T? {
        return segmentList.find { it.startTime <= progress && progress < it.endTime }
    }

    fun printSegments(header: String? = null, sl: MutableList<T> = segmentList) {
        if (header != null) {
            Log.d(TAG, header)
        }

        synchronized(this) {
            for ((index, segment: T) in sl.withIndex()) {
                Log.d(TAG, "\t$index -> $segment")
            }
        }
    }

    // Listeners
    fun registerOnSegmentsChangedListener(listener: OnSegmentsChangedListener<T>) {
        if (!segmentsChangedListeners.contains(listener)) {
            segmentsChangedListeners.add(listener)
        }
    }

    fun unregisterOnSegmentsChangedListener(listener: OnSegmentsChangedListener<T>) {
        segmentsChangedListeners.remove(listener)
    }

    protected fun notifyListeners() {
        Log.d(TAG, "Firing segments changed listeners")
        for (l in segmentsChangedListeners) {
            l.onSegmentsChanged(segments)
        }
    }
}
