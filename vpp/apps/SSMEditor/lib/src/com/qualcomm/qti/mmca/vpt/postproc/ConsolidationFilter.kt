/**
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file ConsolidationFilter.kt
 */
package com.qualcomm.qti.mmca.vpt.postproc

import java.util.*

class ConsolidationFilter(
    private val consolidationThreshold: Int,
    private val classConfig: HashMap<String, ClassificationDefinition>) {

    /**
     * The point of this is basically to look at the "current" batch and its neighbours
     * and determine whether there's enough adjacent batches which have some action with
     * probability exceeding their predefined threshold. If enough neighbouring batches
     * have a classification with probability that exceeds its threshold then we can
     * consider the "current" batch to also have something interesting happening. It is up
     * to the caller to determine which batch is the interesting batch. This function will
     * just perform this function based on the list of batches in the |window|.
     * Tracks the number of times that a classification exceeds its threshold.
     */
    fun executeWindow(window: List<PostProcBatch>, updateBatch: PostProcBatch) {

        // for each class, keeps track of how many batches in this window
        // exceeds their threshold
        val exceedThresholdCounter = HashMap<String, Int>()

        for (batch in window) {
            for (classId in batch.inferences.keys) {
                val def = classConfig[classId] ?: continue // only care about defined things
                val ppr = batch.inferences[classId]!!

                var count = exceedThresholdCounter.getOrDefault(classId, 0)
                count += if (ppr.logit.probability >= def.threshold) 1 else 0
                exceedThresholdCounter[classId] = count
            }
        }

        // Now within this window, check to see how many classes exceeded their
        // own threshold, and how many times. If it crossed the threshold a sufficient
        // number of times (as defined by |consolidationThreshold| then we can say that
        // that particular class exceeded the threshold and report this back to the
        // caller.
        for (thresholdExceeded in exceedThresholdCounter.values) {
            if (thresholdExceeded >= consolidationThreshold) {
                updateBatch.hasClassExceedingThreshold = true
                break
            }
        }
    }
}
