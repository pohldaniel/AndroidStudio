package com.android.webgpu;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;

public class MainActivity extends AppCompatActivity {

    WgpView wgpView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        wgpView = new WgpView(this);
        wgpView.initRenderer(getAssets());
        setContentView(wgpView);
    }

    @Override
    protected void onPause() {
        super.onPause();
        //wgpView.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        //wgpView.onResume();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }
}

