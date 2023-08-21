/**
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file LowPassFilter.kt
 */
package com.qualcomm.qti.mmca.vpt.postproc

import java.util.*

class LowPassFilter(private val tapCount: Int) {

    /**
     * Given a list of |PostProcBatch|es, this function will calculate
     * the average value of |PostProcBatch.results.rawLogit.logit|
     * and store the result in |updateBatch.results.lpfLogit.logit|. It
     * will then calculate the probability of each
     * |updateBatch.results.lpfLogit.probability|.
     */
    private fun executeWindow(batches: List<PostProcBatch>, updateBatch: PostProcBatch) {
        class Average {
            var sum = 0.0
            var count = 0
            fun calculate(): Double {
                return if (count == 0) 0.0 else sum / count
            }
        }

        val runningSums = HashMap<String, Average>()
        for (batch in batches) {
            batch.inferences.forEach { (classId: String, v: PostProcInference) ->
                if (runningSums[classId] == null) {
                    runningSums[classId] = Average()
                }

                runningSums[classId]!!.apply {
                    sum += v.originalLogit.logitValue
                    count++
                }
            }
        }

        runningSums.forEach { (key, average) ->
            updateBatch.get(key)?.lpfLogit?.logitValue = average.calculate()
        }

        updateBatch.calculateLPFProbabilities()
    }

    /**
     * Given a list of |ClassificationBatch|es, calculates new logit values
     * for each |ClassificationResult| within the batch. |tapCount| must be
     * odd, and the size of |batches| must be greater than or equal to tapCount.
     *
     * The new logit value will be calculated by taking the average of |tapCount|
     * values centered around the current center; i.e. this is a sliding window
     * operation with the middle of the sliding window getting updated with the
     * average values around it.
     *
     * This function is destructive; that is, it will modify and return the original
     * |batches|.
     */
    fun execute(batches: List<PostProcBatch>) {
        if (tapCount % 2 == 0) {
            throw Exception(
                "invalid tap count, must be odd and non-zero, tapCount=$tapCount")
        }

        val neighbours = tapCount / 2

        for ((index, windowCenterBatch) in batches.withIndex()) {
            val currentWindow: List<PostProcBatch> =
                PostProcUtils.getWindowedView(batches, index, neighbours, neighbours)
            executeWindow(currentWindow, windowCenterBatch)
        }
    }
}
