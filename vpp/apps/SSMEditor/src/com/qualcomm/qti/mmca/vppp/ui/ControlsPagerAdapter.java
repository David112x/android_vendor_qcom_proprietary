/* ControlsPagerAdapter.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.mmca.vppp.ui;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;

import com.qualcomm.qti.mmca.view.viewpager.PagerAdapter;

import com.qualcomm.qti.mmca.vppp.db.ControlGroup;

import java.util.List;
import java.util.ArrayList;

import com.qualcomm.qti.ssmeditor.R;

/**
 * Adapter class for the ViewPager showing controls. This binds the data to the views.
 */
public class ControlsPagerAdapter extends PagerAdapter implements ControlFieldUIElement {

    private Context mContext;
    private List<ControlsPagerItem> mItems;

    /**
     * controlGroups should not be null.
     */
    public ControlsPagerAdapter(Context context, List<ControlGroup> controlGroups,
            ControlChangedListener listener) {
        this.mContext = context;

        // Set up the Pager Items
        mItems = new ArrayList<>();
        for (ControlGroup controlGroup : controlGroups) {
            mItems.add(new ControlsPagerItem(controlGroup, listener));
        }
    }

    /**
     * collection should not be null.
     */
    @Override
    public Object instantiateItem(ViewGroup collection, int position) {
        LayoutInflater inflater = LayoutInflater.from(mContext);
        ListView listView = (ListView) inflater.inflate(R.layout.controls_pager_item, collection, false);

        listView.setAdapter(mItems.get(position).getAdapter());

        collection.addView(listView);
        return listView;
    }

    @Override
    public void destroyItem(ViewGroup collection, int position, Object view) {
        collection.removeView((View) view);
    }

    @Override
    public int getCount() {
        return mItems.size();
    }

    @Override
    public boolean isViewFromObject(View view, Object object) {
        return view == object;
    }

    @Override
    public CharSequence getPageTitle(int position) {
        return mItems.get(position).getTitle();
    }

    @Override
    public void setControlChangedListener(ControlChangedListener listener) {
        for (ControlsPagerItem item : mItems) {
            item.setControlChangedListener(listener);
        }
    }
}
