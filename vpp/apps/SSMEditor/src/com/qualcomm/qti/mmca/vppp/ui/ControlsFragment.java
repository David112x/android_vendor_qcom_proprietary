/* ControlsFragment.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.mmca.vppp.ui;

import android.app.Fragment;
import android.content.Context;
import android.os.Bundle;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.qualcomm.qti.ssmeditor.R;
import com.qualcomm.qti.mmca.vppp.db.Preset;

/**
 * Fragment to display the controls of a preset at the bottom of the player.
 */
public class ControlsFragment extends Fragment {

    public static final String TAG = "ControlsFragment";
    private static final String ARG_VM = "param_preset";

    private ControlsFragmentViewModel mViewModel;

    private ControlsView mControlsView;

    private ControlChangedListener mListener;

    public ControlsFragment() {
        // empty public constructor as required
    }

    /**
     * @param preset The preset whose controls the Fragment should display. Should not be null.
     * @return A new instance of fragment ControlsFragment.
     */
    public static ControlsFragment newInstance(Preset preset) {
        ControlsFragment fragment = new ControlsFragment();
        Bundle args = new Bundle();
        args.putParcelable(ARG_VM, new ControlsFragmentViewModel(preset, 0));
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // Load the viewModel
        if (savedInstanceState != null) {
            mViewModel = (ControlsFragmentViewModel) savedInstanceState.getParcelable(ARG_VM);
        } else if (getArguments() != null) {
            mViewModel = (ControlsFragmentViewModel) getArguments().getParcelable(ARG_VM);
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        View view = inflater.inflate(R.layout.fragment_controls, container, false);

        mControlsView = view.findViewById(R.id.controls_sheet);
        setupControlsView();

        return view;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof ControlChangedListener) {
            mListener = (ControlChangedListener) context;
        } else {
            throw new RuntimeException(context.toString()
                    + " must implement ControlChangedListener");
        }
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putParcelable(ARG_VM, mViewModel);
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mListener = null;
    }

    private void setupControlsView() {
        mControlsView.configure(mViewModel.getPreset(), mListener);
        mControlsView.setOnPageChangedListener(new ControlsView.OnPageChangedListener() {
            @Override
            public void onPageChanged(int newPage) {
                if (newPage != -1) {
                    // Happens right after configuration change so we ignore this while
                    // restoring state.
                    mViewModel.setPageIndex(newPage);
                }
            }
        });
        mControlsView.setPage(mViewModel.getPageIndex());
    }

    /**
     * @param preset Should not be null.
     */
    public void updateData(Preset preset){
        mViewModel.setPreset(preset);
        setupControlsView();
        mControlsView.setPage(0);
    }
}

