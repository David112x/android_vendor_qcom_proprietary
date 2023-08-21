/**
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file PostProc.kt
 */
package com.qualcomm.qti.mmca.vpt.postproc

import android.content.Context
import android.util.Log
import com.qualcomm.qti.mmca.vpt.classification.ClassificationBatch
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.io.FileWriter
import kotlin.math.exp

val TAG = "PostProc"

/**
 * Defines the parameters for a particular class to be used in the post processing
 * pipeline.
 *
 * id           String which identifies the class
 * label        Human readable representation of this class
 * threshold    The threshold that needs to be met in order for this class to be
 *              deemed 'interesting'
 * displayable  Flag indicating whether this class should be used in thresholding.
 *              This is necessary as post proc has no notion of interesting or
 *              not-interesting. When selecting the interesting class, if no classes
 *              exceed their predefined threshold, then the class with the highest
 *              probability is returned. When this is set to false for a class, that
 *              class is precluded from being chosen.
 */
class ClassificationDefinition(
    var id: String,
    var label: String,
    var threshold: Double,
    var displayable: Boolean) {

    companion object {
        const val THRESHOLD_MAX = 1.0
        const val DEFAULT_INTERESTING_THRESHOLD = 0.70

        lateinit var DEFAULT_INTERESTING: ClassificationDefinition
        lateinit var DEFAULT_NOT_INTERESTING: ClassificationDefinition

        init {
            DEFAULT_INTERESTING = getDefaultInteresting()
            DEFAULT_NOT_INTERESTING =
                ClassificationDefinition("0", "NOT_INTERESTING", THRESHOLD_MAX, false)
        }

        @JvmStatic
        fun getDefaultInteresting() =
            ClassificationDefinition("1", "INTERESTING", DEFAULT_INTERESTING_THRESHOLD, true)
    }
}

class PostProcInference {
    class Logit {
        var logitValue: Double = 0.0
        var probability: Double = 0.0
    }

    var classId: String = ""
    var originalLogit = Logit()
    var lpfLogit = Logit()

    // used to determine which logit value should be used
    var logit = originalLogit

    var generateResult = false
    fun shouldGenerateResult() = generateResult
}

/**
 * Represents a batch of logits that's being used in post proc. Adds additional
 * fields to the standard ClassificationBatch, such as intermediate data that needs
 * to be stored as a particular batch is being processed by each module / stage in
 * the post processing pipeline. This class is intended to be used internally by
 * post proc.
 *
 * E.g.
 *   - LPF knows that it needs to update PostProcResult.lpfLogit
 *   - Consolidation knows that it needs to update PostProcBatch.hasClassExceedingThreshold
 *   - Caller of LPF needs to update PostProcResult.logit to point to either originalLogit
 *     or lpfLogit...
 *   - Other blocks need to interpret PostProcResult.logit to make its decision...
 */
class PostProcBatch(
    private val originalBatch: ClassificationBatch) {

    val inferences = HashMap<String, PostProcInference>()

    // consolidation filter results - do we want to track exactly which class exceeded
    // its threshold and by how much? If so we should probably put it in PostProcResult...
    // maybe store a list of sorted classes (keys) which exceed the threshold? It's not
    // particularly useful at this point...
    var hasClassExceedingThreshold = false

    init {
        var unknown = 0
        for (b in originalBatch.classificationResults) {

            val ppi = PostProcInference()
            ppi.originalLogit.logitValue = b.logit.toDouble()
            if (b.classId != null) {
                ppi.classId = b.classId
            } else {
                ppi.classId = "unknown_${unknown}"
                unknown++
            }

            inferences.putIfAbsent(ppi.classId, ppi)?.also {
                Log.e(TAG, "batch already contains: ${ppi.classId}")
            }
        }
    }

    fun calculateProbabilities(logits: List<PostProcInference.Logit>) {
        var sumExp = 0.0
        var exponentials = ArrayList<Double>(logits.size)

        logits.forEach { i -> exponentials.add(exp(i.logitValue)) }

        for (e in exponentials) {
            sumExp += e
        }

        for ((i, logit) in logits.withIndex()) {
            logit.probability = exponentials.get(i) / sumExp
        }
    }

    fun calculateOriginalProbabilities() {
        val logits = ArrayList<PostProcInference.Logit>(inferences.size)
        inferences.forEach { (_, v) -> logits.add(v.originalLogit) }
        calculateProbabilities(logits)
    }

    fun calculateLPFProbabilities() {
        val logits = ArrayList<PostProcInference.Logit>(inferences.size)
        inferences.forEach { (_, v) -> logits.add(v.lpfLogit) }
        calculateProbabilities(logits)
    }

    fun useLowPassFilteredLogits() {
        inferences.forEach { (_, v) -> v.logit = v.lpfLogit }
    }

    /**
     * This function takes in a HashMap<String, ClassificationDefinition> and
     * compares each PostProcResult's probability with the corresponding
     * definition's threshold. If multiple PostProcResults have probability
     * exceeding their threshold then the highest is selected. Only classes
     * where ClassificationDefinition.displayable = true are considered.
     *
     * If no classes exceed their threshold, then the highest probability with
     * displayable = true is returned.
     */
    fun selectClass(defs: HashMap<String, ClassificationDefinition>): PostProcInference? {
        var maxProb = 0.0
        var maxResult: PostProcInference? = null

        inferences.forEach { (_classId, _result) ->
            run {
                val def = defs[_classId]

                val prob = _result.logit.probability
                val thresh = def?.threshold ?: 0.0

                if (def?.displayable == true) {
                    if (prob > maxProb) {
                        maxProb = prob
                        maxResult = _result
                    }

                    if (prob > thresh) {
                        _result.generateResult = true
                    }
                }
            }
        }

        return maxResult
    }

    fun get(key: String) = inferences.get(key)

    fun getFrameAtIndex(index: Int) = originalBatch.frames.getOrNull(index)
}

class PostProcUtils {
    companion object {
        /**
         * Returns a list of elements from |batches| with |left| number of
         * elements, followed by the element at |centerIndex| followed by
         * |right| number of elements from the right of |centerIndex|. If
         * |left| or |right| are outside the range of |batches| then the
         * 0th and nth element are repeated in order to pad the resultant
         * list with the required size.
         * 0 <= |centerIndex| < batches.size()
         */
        @JvmStatic
        fun <T> getWindowedView(
            batches: List<T>, centerIndex: Int, numLeft: Int, numRight: Int): List<T> {

            if (centerIndex < 0 || centerIndex >= batches.size) {
                throw Exception("center index out of bounds, centerIndex=" +
                    centerIndex + ", list size=" + batches.size)
            }

            val windowedList: MutableList<T> = ArrayList(numLeft + numRight + 1)

            var padLeft = 0
            var padRight = 0

            var indexLeft = centerIndex - numLeft
            var indexRight = centerIndex + numRight

            if (indexLeft < 0) {
                padLeft = -indexLeft
                indexLeft = 0
            }

            if (indexRight >= batches.size) { // +1 since index is 0 based, and size() is not
                padRight = indexRight - batches.size + 1
                indexRight = batches.size - 1
            }

            for (i in 0 until padLeft) {
                windowedList.add(batches[0])
            }

            // +1 as subList is left-inclusive, right-exclusive
            windowedList.addAll(batches.subList(indexLeft, indexRight + 1))

            for (i in 0 until padRight) {
                windowedList.add(batches[batches.size - 1])
            }

            return windowedList
        }
    }
}

class PostProcDebugInfo {
    var batchIndex: Int = -1
    var frameIndex: Int = -1
    var videoTsStart: Long = -1
    var videoTsEnd: Long = -1
    var probability: Double = Double.NaN
    var logit: Double = Double.NaN
    var label: String = ""

    fun asJSONObject(): JSONObject {
        return JSONObject().apply {
            put("batch_index", batchIndex)
            put("frame_index", frameIndex)
            put("vid_ts_start", videoTsStart)
            put("vid_ts_end", videoTsEnd)
            put("probability", probability)
            put("logit", logit)
            put("label", label)
        }
    }
}

class PostProcDebug(val ctx: Context) {

    fun convertPostProcBatchesToJSON(batches: List<PostProcBatch>): JSONArray {
        val allResults = JSONArray()
        for ((index, windowCenterBatch) in batches.withIndex()) {
            val batch = JSONObject()
            val results = JSONArray()
            batch.put("batch_index", index)
            windowCenterBatch.inferences.forEach { (classId: String, r: PostProcInference) ->
                results.put(JSONObject().apply {
                    put("id", classId)
                    put("original_logit", r.originalLogit.logitValue)
                    put("original_softmax", r.originalLogit.probability)

                    put("lpf_logit", r.lpfLogit.logitValue)
                    put("lpf_softmax", r.lpfLogit.probability)
                })
            }
            batch.put("results", results)
            allResults.put(batch)
        }

        return allResults
    }

    fun dump(batches: List<PostProcBatch>,
             cahr: List<PostProcDebugInfo>,
             overlayInfos: List<PostProcDebugInfo>,
             debugFile: File) {

        // lpf and raw logits
        val logits = convertPostProcBatchesToJSON(batches)

        // capture and hold
        val cah = JSONArray()
        cahr.forEach { cah.put(it.asJSONObject()) }

        // overlay info
        val olay = JSONArray()
        overlayInfos.forEach { olay.put(it.asJSONObject()) }

        val rootObject = JSONObject().apply {
            put("logits", logits)
            put("cahr", cah)
            put("overlay", olay)
        }

        FileWriter(debugFile).apply {
            write(rootObject.toString(4))
            close()
        }
    }
}
