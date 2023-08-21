/*
* Copyright (c) 2019 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

package com.qualcomm.qti.simcontacts.activities;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.widget.Toast;

import com.android.contacts.util.PermissionsUtil;
import com.qualcomm.qti.simcontacts.R;

public class ContactMultiSelectionActivity extends AppCompatActivity  {
    private static final int REQUEST_CODE = 10000;
    private ContactSelectionFragment mContactsFragment;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (PermissionsUtil.hasPermissions(this)) {
            showUI();
        } else {
            String[] permissions = PermissionsUtil.getRequireGrantPermissions(this);
            requestPermissions(permissions, REQUEST_CODE);
        }
    }

    @Override
    public void onBackPressed() {
        if (mContactsFragment.isSearchMode()) {
            mContactsFragment.setSearchMode(false);
        } else {
            super.onBackPressed();
        }
    }

    @Override
    public void onRequestPermissionsResult(
            int requestCode,  String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (PermissionsUtil.hasPermissions(this)) {
            showUI();
        } else {
            Toast.makeText(this, R.string.missing_required_permission, Toast.LENGTH_SHORT).show();
            finish();
        }
    }

    private void showUI(){
        setContentView(R.layout.contact_selection_activity);
        mContactsFragment = (ContactSelectionFragment) getFragmentManager().
                findFragmentById(R.id.multiple_selection_fragment);
    }
}
