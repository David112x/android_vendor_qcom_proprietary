/**
 * Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file ClassificationPostProcessor.java
 */
package com.qualcomm.qti.mmca.vpt.classification;

import android.content.Context;
import android.util.Log;

import com.qualcomm.qti.mmca.ssmeditor.FRCSegment;
import com.qualcomm.qti.mmca.ssmeditor.LabelledProbabilitySegment;
import com.qualcomm.qti.mmca.vpt.postproc.PostProcDebugInfo;
import com.qualcomm.qti.mmca.vpt.postproc.ClassificationDefinition;
import com.qualcomm.qti.mmca.vpt.postproc.ConsolidationFilter;
import com.qualcomm.qti.mmca.vpt.postproc.PostProcBatch;
import com.qualcomm.qti.mmca.vpt.postproc.LowPassFilter;
import com.qualcomm.qti.mmca.vpt.postproc.PostProcDebug;
import com.qualcomm.qti.mmca.vpt.postproc.PostProcInference;
import com.qualcomm.qti.mmca.vpt.postproc.PostProcUtils;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Vector;

public class ClassificationPostProcessor {

    public static class Configuration {
        public int windowNeighboursLeft = 1;
        public int windowNeighboursRight = 1;

        public int batchNumFrames = 32;
        public int batchStepSize = 5;

        public int lowPassFilterTapCount = 3; // non-zero implies enabled

        public List<ClassificationDefinition> classificationDefinitions = new Vector<>();

        // Debug
        public Context context = null;
        public File debugDumpFile = null;

        public void addClassDef(ClassificationDefinition def) {
            classificationDefinitions.add(def);
        }
    }

    public class PostProcResult {
        public ArrayList<FRCSegment> frcSegments = new ArrayList<>();
        public ArrayList<LabelledProbabilitySegment> lpSegments = new ArrayList<>();
    }

    private static String TAG = "PostProc";

    private HashMap<String, ClassificationDefinition> mClassDefMap = new HashMap();
    private Configuration mConfig = new Configuration();

    public ClassificationPostProcessor() {
        // constructor
        setupDefaultConfigMapIfRequired();
    }

    public ClassificationPostProcessor(Configuration config) {
        // constructor
        mConfig = config;
        addDefsToConfigMap(config.classificationDefinitions);
        setupDefaultConfigMapIfRequired();
    }

    private void addDefToConfigMap(ClassificationDefinition definition) {
        mClassDefMap.put(definition.getId(), definition);
    }

    private void addDefsToConfigMap(List<ClassificationDefinition> defs) {
        if (defs == null) {
            return;
        }

        for (ClassificationDefinition def : defs) {
            addDefToConfigMap(def);
        }
    }

    private void setupDefaultConfigMapIfRequired() {
        if (mClassDefMap.size() != 0) {
            return;
        }

        addDefsToConfigMap(getDefaultClassDefs());
    }

    public static List<ClassificationDefinition> getDefaultClassDefs() {
        List<ClassificationDefinition> defs = new Vector<>();
        defs.add(ClassificationDefinition.DEFAULT_INTERESTING);
        defs.add(ClassificationDefinition.DEFAULT_NOT_INTERESTING);
        return defs;
    }

    private String getDisplayLabelForId(String classId) {
        ClassificationDefinition definition = mClassDefMap.get(classId);
        if (definition != null) {
            return definition.getLabel();
        }
        return "";
    }

    private boolean isFrameValid(Frame f) {
        return f != null && f.isValid() && f.getVideoTimestamp() >= 0;
    }

    public PostProcResult execute(Metadata metadata) {

        List<PostProcDebugInfo> debugOverlayInfos = new ArrayList<>();
        List<PostProcDebugInfo> debugCaptureAndHolds = new ArrayList<>();
        boolean shouldWriteDebugFile =
                mConfig.context != null && mConfig.debugDumpFile != null;

        PostProcResult ppr = new PostProcResult();
        List<ClassificationBatch> batches = metadata.getBatches();
        List<PostProcBatch> postProcBatches = new ArrayList<>(batches.size());

        batches.forEach(b -> {
            PostProcBatch ppb = new PostProcBatch(b);
            ppb.calculateOriginalProbabilities();
            postProcBatches.add(ppb);
        });

        if (mConfig.lowPassFilterTapCount > 0) {
            try {
                LowPassFilter lpf = new LowPassFilter(mConfig.lowPassFilterTapCount);
                lpf.execute(postProcBatches);
                postProcBatches.forEach(b -> b.useLowPassFilteredLogits());
            } catch (Exception e) {
                e.printStackTrace();
            }
        }

        int middleFrameOffset = mConfig.batchStepSize * (mConfig.lowPassFilterTapCount / 2);
        int centerFrameOffset = mConfig.batchStepSize / 2;
        int upsampleCenterFrame = (mConfig.batchNumFrames / 2) - middleFrameOffset;
        int startIndex = upsampleCenterFrame - centerFrameOffset;
        int endIndex = upsampleCenterFrame + centerFrameOffset;
        if (mConfig.batchStepSize % 2 != 0) {
            endIndex += 1;
        }

        for (int i = 0; i < postProcBatches.size(); i++) {
            PostProcBatch centerBatch = postProcBatches.get(i);
            PostProcInference selectedClass = centerBatch.selectClass(mClassDefMap);

            if (selectedClass == null) {
                Log.e(TAG, "no selected class for i=" + i);
                continue;
            }

            int streamFrameStartIndex = (i * mConfig.batchStepSize) + startIndex;

            Frame startFrame = centerBatch.getFrameAtIndex(startIndex);
            Frame endFrame = centerBatch.getFrameAtIndex(endIndex);

            if (!isFrameValid(startFrame) || !isFrameValid(endFrame)) {
                Log.e(TAG, "invalid frame, i=" + i + ", valid{start=" + isFrameValid(startFrame) +
                        ", end=" + isFrameValid(endFrame) + "}");
                continue;
            }

            long startTimeUs = startFrame.getVideoTimestamp() / 1000;
            long endTimeUs = endFrame.getVideoTimestamp() / 1000;

            // Generate FRC segment
            if (selectedClass.shouldGenerateResult()) {
                FRCSegment s = new FRCSegment(startTimeUs, endTimeUs, FRCSegment.Interp.INTERP_2X);
                ppr.frcSegments.add(s);
                Log.d(TAG, "added segment: " + s);

                if (shouldWriteDebugFile) {
                    PostProcDebugInfo debugResult = new PostProcDebugInfo();
                    debugResult.setBatchIndex(i);
                    debugResult.setFrameIndex(streamFrameStartIndex);
                    debugResult.setLogit(selectedClass.getLogit().getLogitValue());
                    debugResult.setProbability(selectedClass.getLogit().getProbability());
                    debugResult.setVideoTsStart(startTimeUs);
                    debugResult.setVideoTsEnd(endTimeUs);
                    debugCaptureAndHolds.add(debugResult);
                }
            }

            // Generate overlay information
            {
                int prob = (int) (selectedClass.getLogit().getProbability() * 100);
                String displayLabel = getDisplayLabelForId(selectedClass.getClassId());
                LabelledProbabilitySegment s = new LabelledProbabilitySegment(
                        startTimeUs, endTimeUs, prob, displayLabel);
                ppr.lpSegments.add(s);

                if (shouldWriteDebugFile) {
                    PostProcDebugInfo debugResult = new PostProcDebugInfo();
                    debugResult.setBatchIndex(i);
                    debugResult.setFrameIndex(streamFrameStartIndex);
                    debugResult.setLogit(selectedClass.getLogit().getLogitValue());
                    debugResult.setProbability(selectedClass.getLogit().getProbability());
                    debugResult.setVideoTsStart(startTimeUs);
                    debugResult.setVideoTsEnd(endTimeUs);
                    debugOverlayInfos.add(debugResult);
                }
            }
        }

        if (shouldWriteDebugFile) {
            PostProcDebug dbg = new PostProcDebug(mConfig.context);
            dbg.dump(postProcBatches, debugCaptureAndHolds, debugOverlayInfos, mConfig.debugDumpFile);
        }

        return ppr;
    }
}
